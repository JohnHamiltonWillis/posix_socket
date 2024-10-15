/* This file makes use of Doxygen syntax for generating documentation. */
/******************************************************************************/
/**
 * @file tcp_server.cpp
 * @brief File containing methods for communicating with TCP clients.
 *
 * This file provides TCPClient member definitions for communicating with
 * TCP clients.
 *
 * @author willisjh
 * @date 22 March 2022
 *
 * @bug No known bugs.
 *
 * @addtogroup TCPServer
 * @{
 *
 ******************************************************************************/


/******************************** Include Files *******************************/
#include "common/socket/tcp_server.hpp"

#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <ratio>

/**************************** Constant Definitions ****************************/
constexpr auto kCClientLast = 12;

/*********************** Static Variable Initializations **********************/

/****************************** Type Definitions ******************************/

/***************************** Function Prototypes ****************************/


/******************************************************************************/
/**
 * Default constructor.
 *
 * @param None.
 *
 * @return None.
 *
 * @note
 *
 ******************************************************************************/
TCPServer::TCPServer()
    : fd_socket_server_(-1)
    , fd_f_accepting_()
    , thread_accept_()
    , rg_sockaddr_client_()
    , rg_fd_socket_client_()
{

}

/******************************************************************************/
/**
 * Constructor with connect initialization.
 *
 * @param[in] id_port The identifier of the port.
 *
 * @return None.
 *
 * @note
 *
 ******************************************************************************/
TCPServer::TCPServer(uint16_t id_port)
    : fd_socket_server_(-1)
    , fd_f_accepting_()
    , thread_accept_()
    , rg_sockaddr_client_()
    , rg_fd_socket_client_()
{
    SockListen(id_port);
}

/******************************************************************************/
/**
 * Deconstructor.
 *
 * @param None.
 *
 * @return None.
 *
 * @note
 *
 ******************************************************************************/
TCPServer::~TCPServer()
{
    SockClose();
}

/******************************************************************************/
/**
 * Method for listening over TCP.
 *
 * @param[in] id_port The identifier of the port.
 *
 * @return None.
 *
 * @note Listening is limited to IPv4.
 *
 ******************************************************************************/
void TCPServer::SockListen(uint16_t id_port)
{
    /**- Create a socket for connecting over TCP. */
    fd_socket_server_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_socket_server_ == -1)
    {
        if ((errno == EACCES) || (errno == EAFNOSUPPORT) || (errno == EINVAL) || (errno == EPROTONOSUPPORT))
        {
            throw ExceptionPriority<std::runtime_error>(IDPriority::kIdNotice, "Socket creation failed: " + static_cast<std::string>(std::strerror(errno)));
        }

        throw ExceptionPriority<std::runtime_error>(IDPriority::kIdErr, "Socket creation failed: " + static_cast<std::string>(std::strerror(errno)));
    }

    /**- Set socket options. */
    int f_reuse = true;
    if (setsockopt(fd_socket_server_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &f_reuse, sizeof(f_reuse)) == -1)
    {
        throw ExceptionPriority<std::runtime_error>(IDPriority::kIdErr, "Set socket options failed: " + static_cast<std::string>(std::strerror(errno)));
    }

    /**- Make the data socket non-blocking. */
    auto val_socket_flags = fcntl(fd_socket_server_, F_GETFL);
    if (val_socket_flags == -1)
    {
        throw std::runtime_error("Socket flags retrieval failed: " + static_cast<std::string>(std::strerror(errno)));
    }
    if (fcntl(fd_socket_server_, F_SETFL, val_socket_flags | O_NONBLOCK) == -1)
    {
        throw std::runtime_error("Socket flags setting failed: " + static_cast<std::string>(std::strerror(errno)));
    }

    /**- Initialize socket address structure with provided info. */
    struct sockaddr_in sockaddr_server = {};
    sockaddr_server.sin_family = AF_INET;
    sockaddr_server.sin_port = htons(idPort);
    sockaddr_server.sin_addr.s_addr = htonl(INADDR_ANY);

    /**- Bind socket and listen. */
    if (bind(fd_socket_server_, reinterpret_cast<struct sockaddr*>(&sockaddr_server), sizeof(sockaddr_server)) == -1)
    {
        throw std::runtime_error("Socket bind failed: " + static_cast<std::string>(std::strerror(errno)));
    }

    if (listen(fd_socket_server_, kCClientLast) == -1)
    {
        throw std::runtime_error("Socket listen failed: " + static_cast<std::string>(std::strerror(errno)));
    }

    /**- Accept incoming client connections. */
    fd_f_accepting_ = true;
    thread_accept_ = std::thread([this] { sockAccept(); });
}

/******************************************************************************/
/**
 * Method for accepting client connections.
 *
 * @return None.
 *
 * @note
 *
 ******************************************************************************/
void TCPServer::SockAccept()
{
    auto c_client = 0;
    while ((fd_f_accepting_) && (c_client <= kCClientLast))
    {
        /**- Wait for client connection. */
        fd_set fdset_socket_server;
        FD_ZERO(&fdset_socket_server);
        FD_SET(fd_socket_server_, &fdset_socket_server);

        auto rc_select = select(fd_socket_server_ + 1, &fdset_socket_server, &fdset_socket_server, &fdset_socket_server, nullptr);
        if (rc_select == 0)
        {
            throw std::runtime_error("Timeout when waiting for client!");
        }
        else if (rc_select == -1)
        {
            throw std::runtime_error("Error occured when waiting for client: " + static_cast<std::string>(std::strerror(errno)));
        }

        /**- Accept client connection. */
        struct sockaddr_in sockaddr_client = {};
        socklen_t cb_sockaddr_client = sizeof(sockaddr_client);
        auto fd_socket_client = accept(fd_socket_server_, reinterpret_cast<struct sockaddr*>(&sockaddr_client), &cb_sockaddr_client);
        if (fd_socket_client == -1)
        {
            throw std::runtime_error("Error occured when accepting client: " + static_cast<std::string>(std::strerror(errno)));
        }

        rg_sockaddr_client_.push_back(sockaddr_client);
        rg_fd_socket_client_.push_back(fd_socket_client);
        ++c_client;
    }

    fd_f_accepting_ = false;
}

/******************************************************************************/
/**
 * Method for sending data over TCP.
 *
 * @param[in] p_data       The pointer to the data.
 * @param[in] cb_data_last The last count of bytes of data.
 * @param[in] time_timeout The time for send timeout.
 *
 * @return None.
 *
 * @note Sends to all accepted client sockets.
 *
 ******************************************************************************/
void TCPServer::SockSend(const void* p_data, const size_t cb_data_last, const std::chrono::duration<float> time_timeout)
{
    /**- Check if appropriate to call this method. */
    if (fd_socket_server_ == -1)
    {
        throw std::runtime_error("SockSend() called before listen!");
    }
    if (rg_fd_socket_client_.size() == 0)
    {
        LOG_MSG_WARNING("SockSend() called with no clients!");
        return;
    }

    /**- Begin looping through all clients. */
    for (const auto& fd_socket_client : rg_fd_socket_client_)
    {
        /**- Begin looping until all data sent. */
        size_t cb_data = 0;
        while (cb_data < cb_data_last)
        {
            /**- Send remaining data. */
            auto cb_data_iteration_last = send(fd_socket_client, static_cast<const uint8_t*>(p_data) + cb_data, cb_data_last - cbData, 0);
            if (cb_data_iteration_last == -1)
            {
                throw std::runtime_error("Sending failed: " + static_cast<std::string>(std::strerror(errno)));
            }
            cbData += cb_data_iteration_last;

            /**- Wait for send if not immediately completed. */
            if (cbData < cb_data_last)
            {
                struct timeval time_timeout_select;
                time_timeout_select.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(time_timeout).count();
                time_timeout_select.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(time_timeout).count() % std::micro::den;

                fd_set fdset_socket_client;
                FD_ZERO(&fdset_socket_client);
                FD_SET(fd_socket_client, &fdset_socket_client);

                auto rc_select = select(fd_socket_client + 1, nullptr, &fdset_socket_client, nullptr, &time_timeout_select);
                if (rc_select == 0)
                {
                    throw std::runtime_error("Timeout when waiting send!");
                }
                else if (rc_select == -1)
                {
                    throw std::runtime_error("Error occured when waiting for send: " + static_cast<std::string>(std::strerror(errno)));
                }
            }
        }
        /**- End looping until all data sent. */
    }
    /**- End looping through all clients. */
}

/******************************************************************************/
/**
 * Method for closing the TCP sockets.
 *
 * @return None.
 *
 * @note
 *
 ******************************************************************************/
void TCPServer::SockClose()
{
    /**- Stop and join accept thread. */
    fd_f_accepting_ = false;
    thread_accept_.join();

    /**- Close all clients. */
    for (const auto& fd_socket_client : rg_fd_socket_client_)
    {
        if (fd_socket_client != -1)
        {
            if (shutdown(fd_socket_client, SHUT_RDWR) == -1)
            {
                LOG_MSG_WARNING("Shutting down client socket failed: " + static_cast<std::string>(std::strerror(errno)));
            }

            if (close(fd_socket_client) == -1)
            {
                LOG_MSG_WARNING("Closing client socket failed: " + static_cast<std::string>(std::strerror(errno)));
            }
        }
    }

    /**- Close server. */
    if (fd_socket_server_ != -1)
    {
        if (shutdown(fd_socket_server_, SHUT_RDWR) == -1)
        {
            LOG_MSG_WARNING("Shutting down server socket failed: " + static_cast<std::string>(std::strerror(errno)));
        }

        if (close(fd_socket_server_) == -1)
        {
            LOG_MSG_WARNING("Closing server socket failed: " + static_cast<std::string>(std::strerror(errno)));
        }
    }

    rg_fd_socket_client_.clear();
    fd_socket_server_ = -1;
}

/******************************************************************************/
/**
 * Method for getting current clients.
 *
 * @return rg_sockaddr_client_ The range of client socket addresses.
 *
 * @note
 *
 ******************************************************************************/
const decltype(TCPServer::rg_sockaddr_client_)& TCPServer::GetClients() const
{
    return rg_sockaddr_client_;
}
/** @} */ /* @addtogroup */