/* This file makes use of Doxygen syntax for generating documentation. */
/******************************************************************************/
/**
 * @file tcp_client.cpp
 * @brief File containing methods for communicating with a TCP server.
 *
 * This file provides TCPClient member definitions for communicating with a
 * TCP server.
 *
 * @author willisjh
 * @date 22 March 2022
 *
 * @bug No known bugs.
 *
 * @addtogroup TCPClient
 * @{
 *
 ******************************************************************************/


/******************************** Include Files *******************************/
#include "common/socket/tcp_client.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include <cerrno>
#include <ratio>

#include "common/log_msg/log_msg.hpp"

/**************************** Constant Definitions ****************************/

/*********************** Static Variable Initializations **********************/

/****************************** Type Definitions ******************************/

/***************************** Function Prototypes ****************************/


/******************************************************************************/
/**
 * Default constructor for TCPClient.
 *
 * @param None.
 *
 * @return None.
 *
 * @note
 *
 ******************************************************************************/
TCPClient::TCPClient()
    : fd_socket_(-1)
    , f_connected_()
{

}

/******************************************************************************/
/**
 * Constructor for TCPClient with SockConnect() initialization.
 *
 * @param None.
 *
 * @return None.
 *
 * @note
 *
 ******************************************************************************/
TCPClient::TCPClient(const char* sz_hostname, uint16_t id_port, const std::chrono::duration<float> time_timeout)
    : fd_socket_(-1)
    , f_connected_()
{
    SockConnect(sz_hostname, id_port, time_timeout);
}

/******************************************************************************/
/**
 * Deconstructor for TCPClient.
 *
 * @param None.
 *
 * @return None.
 *
 * @note
 *
 ******************************************************************************/
TCPClient::~TCPClient()
{
    SockClose();
}

/******************************************************************************/
/**
 * Method for connecting over TCP.
 *
 * @param[in] sz_hostname  The zero-terminated string of the hostname.
 * @param[in] id_port      The identifier of the port.
 * @param[in] time_timeout The time for connection timeout.
 *
 * @return None.
 *
 * @note Connection is limited to IPv4.
 *
 ******************************************************************************/
void TCPClient::SockConnect(const char* sz_hostname, uint16_t id_port, const std::chrono::duration<float> time_timeout)
{
    /**- Create a socket for connecting over TCP. */
    fd_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_socket_ == -1)
    {
        if ((errno == EACCES) || (errno == EAFNOSUPPORT) || (errno == EINVAL) || (errno == EPROTONOSUPPORT))
        {
            throw ExceptionPriority<std::runtime_error>(IDPriority::kIdNotice, "Socket creation failed: " + static_cast<std::string>(std::strerror(errno)));
        }

        throw ExceptionPriority<std::runtime_error>(IDPriority::kIdErr, "Socket creation failed: " + static_cast<std::string>(std::strerror(errno)));
    }

    /**- Make the data socket non-blocking. */
    auto val_socket_flags = fcntl(fd_socket_, F_GETFL);
    if (val_socket_flags == -1)
    {
        throw std::runtime_error("Socket flags retrieval failed: " + static_cast<std::string>(std::strerror(errno)));
    }
    if (fcntl(fd_socket_, F_SETFL, val_socket_flags | O_NONBLOCK) == -1)
    {
        throw std::runtime_error("Socket flags setting failed: " + static_cast<std::string>(std::strerror(errno)));
    }

    /**- Lookup provided hostname. */
    struct addrinfo addrinfo_hints = {};
    addrinfo_hints.ai_family = AF_INET;
    addrinfo_hints.ai_socktype = SOCK_STREAM;
    addrinfo_hints.ai_flags |= AI_CANONNAME;

    struct addrinfo* p_addrinfo_result = {};

    /* Second argument is the service name argument. This could allow auto-retrieving the port number. */
    if (getaddrinfo(sz_hostname, nullptr, &addrinfo_hints, &p_addrinfo_result) != 0)
    {
        throw std::runtime_error("Hostname lookup failed for IPv4!");
    }

    /**- Initialize socket address structure with provided info. */
    struct sockaddr_in sockaddr_afinet = {};
    sockaddr_afinet.sin_family = AF_INET;
    sockaddr_afinet.sin_port = htons(id_port);
    sockaddr_afinet.sin_addr = reinterpret_cast<struct sockaddr_in*>(p_addrinfo_result->ai_addr)->sin_addr;

    freeaddrinfo(p_addrinfo_result);

    /**- Connect over TCP. */
    if (connect(fd_socket_, reinterpret_cast<struct sockaddr*>(&sockaddr_afinet), sizeof(sockaddr_afinet)) == -1)
    {
        if (errno == EINPROGRESS)
        {
            /**- Wait for connection if not immediately completed. */
            struct timeval time_timeout_select;
            time_timeout_select.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(time_timeout).count();
            time_timeout_select.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(time_timeout).count() % std::micro::den;

            fd_set fdset_socket;
            FD_ZERO(&fdset_socket);
            FD_SET(fd_socket_, &fdset_socket);

            auto rc_select = select(fd_socket_ + 1, &fdset_socket, &fdset_socket, &fdset_socket, &time_timeout_select);
            if (rc_select == 0)
            {
                throw std::runtime_error("Timeout when waiting for connect!");
            }
            else if (rc_select == -1)
            {
                throw std::runtime_error("Error occured when waiting for connect: " + static_cast<std::string>(std::strerror(errno)));
            }

            /**- Check if connection completed successfully after waiting. */
            /* Initialize the return code to EINPROGRESS since this should not be returned at this point. */
            auto errno_connect = EINPROGRESS;
            socklen_t cb_errno_connect_last = sizeof(errno_connect);
            if (getsockopt(fd_socket_, SOL_SOCKET, SO_ERROR, &errno_connect, &cb_errno_connect_last) == -1)
            {
                throw std::runtime_error("Checking for successful connection failed: " + static_cast<std::string>(std::strerror(errno)));
            }

            if (cb_errno_connect_last == 0)
            {
                throw std::runtime_error("Checking for successful connection failed!");
            }

            if (errno_connect != 0)
            {
                throw std::runtime_error("Connecting over TCP failed: " + static_cast<std::string>(std::strerror(errno_connect)));
            }
        }
        else
        {
            throw std::runtime_error("Connecting to server failed: " + static_cast<std::string>(std::strerror(errno)));
        }
    }

    f_connected_ = true;
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
 * @note
 *
 ******************************************************************************/
void TCPClient::SockSend(const void* p_data, const size_t cb_data_last, const std::chrono::duration<float> time_timeout)
{
    /**- Check if appropriate to call this method. */
    if (!f_connected_)
    {
        throw std::runtime_error("SockSend() called before connection!");
    }

    /**- Begin looping until all data sent. */
    size_t cb_data = 0;
    while (cb_data < cb_data_last)
    {
        /**- Send remaining data. */
        auto cb_data_iteration_last = send(fd_socket_, static_cast<const uint8_t*>(p_data) + cb_data, cb_data_last - cb_data, 0);
        if (cb_data_iteration_last == -1)
        {
            throw std::runtime_error("Sending failed: " + static_cast<std::string>(std::strerror(errno)));
        }
        cb_data += cb_data_iteration_last;

        /**- Wait for send if not immediately completed. */
        if (cb_data < cb_data_last)
        {
            struct timeval time_timeout_select;
            time_timeout_select.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(time_timeout).count();
            time_timeout_select.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(time_timeout).count() % std::micro::den;

            fd_set fdset_socket;
            FD_ZERO(&fdset_socket);
            FD_SET(fd_socket_, &fdset_socket);

            auto rc_select = select(fd_socket_ + 1, nullptr, &fdset_socket, nullptr, &time_timeout_select);
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

/******************************************************************************/
/**
 * Method for receiving data over TCP.
 *
 * @param[out] p_data       The pointer to the data.
 * @param[in]  cb_data_last The last count of bytes of data.
 * @param[in]  time_timeout The time for recv timeout.
 *
 * @return None.
 *
 * @note p_data must have memory preallocated up to cb_data_last.
 *
 ******************************************************************************/
void TCPClient::SockRecv(void* p_data, const size_t cb_data_last, const std::chrono::duration<float> time_timeout)
{
    /**- Check if appropriate to call this method. */
    if (!f_connected_)
    {
        throw std::runtime_error("SockRecv() called before connection!");
    }

    /**- Begin looping until all data sent. */
    size_t cb_data = 0;
    while (cb_data < cb_data_last)
    {
        /**- Receive remaining data. */
        auto cb_data_iteration_last = recv(fd_socket_, static_cast<uint8_t*>(p_data) + cb_data, cb_data_last - cb_data, 0);
        if (cb_data_iteration_last == -1)
        {
            throw std::runtime_error("Sending failed: " + static_cast<std::string>(std::strerror(errno)));
        }
        cb_data += cb_data_iteration_last;

        /**- Wait for recv if not immediately completed. */
        if (cb_data < cb_data_last)
        {
            struct timeval time_timeout_select;
            time_timeout_select.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(time_timeout).count();
            time_timeout_select.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(time_timeout).count() % std::micro::den;

            fd_set fdset_socket;
            FD_ZERO(&fdset_socket);
            FD_SET(fd_socket_, &fdset_socket);

            auto rc_select = select(fd_socket_ + 1, &fdset_socket, nullptr, nullptr, &time_timeout_select);
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

/******************************************************************************/
/**
 * Method for closing the TCP socket.
 *
 * @return None.
 *
 * @note
 *
 ******************************************************************************/
void TCPClient::SockClose()
{
    if (fd_socket_ != -1)
    {
        if (shutdown(fd_socket_, SHUT_RDWR) == -1)
        {
            LOG_MSG_WARNING("Shutting down socket failed: " + static_cast<std::string>(std::strerror(errno)));
        }

        if (close(fd_socket_) == -1)
        {
            LOG_MSG_WARNING("Closing socket failed: " + static_cast<std::string>(std::strerror(errno)));
        }
    }

    fd_socket_ = -1;
    f_connected_ = false;
}
/** @} */ /* @addtogroup */