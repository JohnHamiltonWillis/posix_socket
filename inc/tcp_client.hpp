/* This file makes use of Doxygen syntax for generating documentation. */
/******************************************************************************/
/**
 * @file tcp_client.hpp
 * @brief File containing class for communicating with a TCP server.
 *
 * This file provides a class definition for communicating with a TCP server.
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

#ifndef TCP_CLIENT_HPP
#define TCP_CLIENT_HPP

/******************************** Include Files *******************************/
#include <cstdint>
#include <chrono>

/**************************** Constant Definitions ****************************/

/****************************** Type Definitions ******************************/

/****************************** Class Declaration *****************************/

/*
 * TCPClient class members defined in TCPClient.cpp
 */
class TCPClient
{
public:
    TCPClient();
    TCPClient(const char* sz_hostname, uint16_t id_port, const std::chrono::duration<float> time_timeout);
    virtual ~TCPClient();

    void SockConnect(const char* sz_hostname, uint16_t id_port, const std::chrono::duration<float> time_timeout);
    void SockSend(const void* p_data, const size_t cb_data_last, const std::chrono::duration<float> time_timeout);
    void SockRecv(void* p_data, const size_t cb_data_last, const std::chrono::duration<float> time_timeout);
    void SockClose();

private:
    int fd_socket_;
    bool f_connected_;
};

#endif  /* #ifndef TCP_CLIENT_HPP */
/** @} */ /* @addtogroup */