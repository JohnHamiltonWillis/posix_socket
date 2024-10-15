/* This file makes use of Doxygen syntax for generating documentation. */
/******************************************************************************/
/**
 * @file tcp_server.hpp
 * @brief File containing class for communicating with TCP clients.
 *
 * This file provides a class definition for communicating with TCP clients.
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

#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

/******************************** Include Files *******************************/
#include "common/log_msg/log_msg.hpp"

#include <sys/socket.h>

#include <stdexcept>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>

/**************************** Constant Definitions ****************************/

/****************************** Type Definitions ******************************/

/****************************** Class Declaration *****************************/

/*
 * TCPServer class members defined in tcp_server.cpp
 */
class TCPServer
{
private:
    /* Declaration needs to occur before GetClients() declaration. */
    std::vector<struct sockaddr_in> rg_sockaaddr_client_;

public:
    TCPServer();
    TCPServer(uint16_t idPort);
    virtual ~TCPServer();

    void SockListen(uint16_t idPort);
    void SockAccept();
    void SockSend(const void* p_data, const size_t cb_data_last, const std::chrono::duration<float> time_timeout);

    template<typename T_DATA>
    void SockRecv(std::vector<std::unique_ptr<T_DATA>>& rg_p_data, const std::chrono::duration<float> time_timeout);

    void SockClose();
    const decltype(rg_sockaaddr_client_)& GetClients() const;

private:
    int fd_socket_server_;
    bool f_accepting_;
    std::thread thread_accept_;
    std::vector<int> rg_fd_socket_client_;
};

/******************************************************************************/
/**
 * Method for receiving data over TCP.
 *
 * @param[out] rg_p_data    The range of pointers to the data.
 * @param[in]  time_timeout The time for recv timeout.
 *
 * @return None.
 *
 * @note Receives from all accepted client sockets.
 *
 ******************************************************************************/
template<typename T_DATA>
void TCPServer::SockRecv(std::vector<std::unique_ptr<T_DATA>>& rg_p_data, const std::chrono::duration<float> time_timeout)
{
    /**- Check if appropriate to call this method. */
    if (fd_socket_server_ == -1)
    {
        throw std::runtime_error("SockRecv() called before listen!");
    }
    if (rg_fd_socket_client_.size() == 0)
    {
        LOG_MSG_WARNING("SockRecv() called with no clients!");
        return;
    }

    /**- Begin looping through all clients. */
    for (const auto& fd_socket_client : rg_fd_socket_client_)
    {
        /**- Allocate memory for expected recv data. */
        rg_p_data.push_back(std::make_unique<T_DATA>());

        /**- Begin looping until all data sent. */
        size_t cb_data = 0;
        while (cb_data < sizeof(T_DATA))
        {
            /**- Receive remaining data. */
            auto cd_data_iteration_last = recv(fd_socket_client, static_cast<uint8_t*>(static_cast<void*>(rg_p_data.back().get())) + cb_data, sizeof(T_DATA) - cb_data, 0);
            if (cd_data_iteration_last == -1)
            {
                throw std::runtime_error("Sending failed: " + static_cast<std::string>(std::strerror(errno)));
            }
            cb_data += cd_data_iteration_last;

            /**- Wait for recv if not immediately completed. */
            if (cb_data < sizeof(T_DATA))
            {
                struct timeval time_timeout_select;
                time_timeout_select.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(time_timeout).count();
                time_timeout_select.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(time_timeout).count() % std::micro::den;

                fd_set fdset_socket_client;
                FD_ZERO(&fdset_socket_client);
                FD_SET(fd_socket_client, &fdset_socket_client);

                auto rc_select = select(fd_socket_client + 1, &fdset_socket_client, nullptr, nullptr, &time_timeout_select);
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

#endif  /* #ifndef TCP_SERVER_HPP */
/** @} */ /* @addtogroup */