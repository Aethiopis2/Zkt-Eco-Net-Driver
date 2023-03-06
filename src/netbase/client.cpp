//==============================================================================================================|
// File Desc:
//  contains implementation for class Client which models a raw client object.
//
// Program Authors:
//  Rediet Worku, Dr. aka Aethiopis II ben Zahab       PanaceaSolutionsEth@gmail.com, aethiopis2rises@gmail.com
//
// Date Created:
//  7th of August 2022, Sunday (My daughter's We'le'te-Sen'be't, baptisim)
//
// Last Updated:
//  29th of Novemeber 2022, Tuesday
//==============================================================================================================|




//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include "client.h"



//==============================================================================================================|
// TYPES
//==============================================================================================================|
// function pointers to send and recieve functions for NetBase, we declare 'em outside the class for because I 
//  did not want to pass around pointers to the class using "this" memeber
typedef ssize_t(Client::*pfn_Send)(const void *p_buf, const size_t len);
typedef ssize_t(Client::*pfn_Recv)(void *p_buf, const size_t len);



//==============================================================================================================|
// GLOBALS
//==============================================================================================================|
pfn_Send fn_Send;       // instances of function pointers
pfn_Recv fn_Recv;



//==============================================================================================================|
// CLASS
//==============================================================================================================|
/**
 * @brief Construct a new Client:: Client object
 *  the constructor
 */
Client::Client(const int protocol)
    : fds{-1}, paddr{nullptr}, ss_len{0},
      delaytcp{0}, keep_alive{0}
{
    FD_ZERO(&rset); // clear

    // initalize pointers to functions used to send and recv and data depending
    //  on the value of "sock_type" parameter sent
    if (protocol == TCP_PROTO)
    {
        fn_Send = &Client::Tcp_Send;
        fn_Recv = &Client::Tcp_Recv;
    } // end if
    else if (protocol == UDP_PROTO)
    {
        // using UDP
        fn_Send = &Client::Udp_Send;
        fn_Recv = &Client::Udp_Recv;
    } // end else
} // end constructor


//==============================================================================================================|
/**
 * @brief 
 *  Starts a net session over TCP/IP protocol.
 * 
 * @param [hostname] the host name or ip to connect to 
 * @param [port] the port address
 *  
 * @return int 
 *  a 0 on success alas -1 on fail
 */
int Client::Tcp_Connect(const std::string &hostname, const std::string &port)
{
    if (Init_Addr(&paddr, hostname, port) < 0)
        return -1;

    if ( (fds = Connect_Tcp(paddr)) < 0)
        return -1;

    return 0;
} // end Tcp_Connect


//==============================================================================================================|
/**
 * @brief 
 *  starts a blocking mode on a socket that expects to receive inputs async; it waits in select(...) till the
 *  kernel notifies the caller of an input ready for reading. 
 * 
 * @param [buf] the buffer to recieve the input from 
 * @param [len] length of buffer in bytes
 *  
 * @return int 
 *  0 on success alas -1
 */
int Client::Select(void *buf, const size_t len)
{
    // one time operation only and wait indefinitly till something cooks
    FD_SET(fds, &rset);
    int maxfdp1 = fds + 1;      

    if ( select(maxfdp1, &rset, NULL, NULL, NULL) > 0 )
    {
        // since we won't timeout very soon, at this point it means descriptor
        //  is ready for receving (something bout recv low watermark reached or passed)
        if (FD_ISSET(fds, &rset))
        {
            // ready to rock
            return static_cast<int>(recv(fds, buf, len, 0));
        } // end if
    } // end if selecting

    return -1;
} // end Select


//==============================================================================================================|
/**
 * @brief 
 *  Closes the net session; ends it, i.e. send a FIN packet to its peer
 * 
 * @return int 
 *  a 0 on success, -1 on fail
 */
int Client::Disconnect()
{
    return Close_Socket(fds);
} // end Disconnect


//==============================================================================================================|
/**
 * @brief 
 *  places a limit on the number of seconds to wait on a socket receiving signals
 * 
 * @param [sec] the seconds to wait for
 *  
 * @return int 
 *  0 on success, -1 on fail
 */
int Client::Set_Recv_Timeout(int sec)
{
    return Set_RecvTimeout(fds, sec);
} // end Set_Recv_Timeout


//==============================================================================================================|
/**
 * @brief 
 *  Toggles the tcp no delay state on/off
 * 
 * @return int 
 *  0 on success alas -1
 */
int Client::Toggle_TcpDelay()
{
    delaytcp = (!delaytcp ? 1 : 0);
    return Tcp_NoDelay(fds, delaytcp);
} // end Toggle_TcpDely


//==============================================================================================================|
/**
 * @brief 
 *  Toggles keepalive socket option on/off.
 * 
 * @return int 
 *  0 on success alas -1
 */
int Client::Toggle_KeepAlive()
{
    keep_alive = (!keep_alive ? 1 : 0);
    return Keep_Alive(fds, keep_alive);
} // end Toggle_KeepAlive


//==============================================================================================================|
/**
 * @brief 
 *  return's the socket descriptor
 * 
 * @return int 
 */
int Client::Get_Socket()
{
    return fds;     
} // end Get_Socket


//==============================================================================================================|
/**
 * @brief 
 *  Wraps around the actual sending protocol; i.e. UDP vs TCP; thus applications shouldn't really care which is
 *  is which and use the same functions regardless.
 * 
 * @param [pbuf] pointer to buffer containing the info to send 
 * @param [len] the length of our buddy the buffer in bytes
 *  
 * @return int
 *  the bytes sent on success, -1 on fail 
 */
int Client::Send(const void *pbuf, const size_t len)
{
    return (this->*fn_Send)(pbuf, len);
} // end Send


//==============================================================================================================|
/**
 * @brief 
 *  Wraps around the recieving functionalitiy thus higher levels could be sheilded from nuiscences of underlyding
 *  protocol.
 * 
 * @param [pbuf] buffer that gets the data 
 * @param [len] length of the buffer
 *  
 * @return int 
 *  on success the bytes recieved or -1 on fail
 */
int Client::Recv(void *pbuf, const size_t len)
{
    return (this->*fn_Recv)(pbuf, len);
} // end Recv


//==============================================================================================================|
/**
 * @brief 
 *  Make life easy wrapper -- wraps around underlying somewhat raw sending function, this function explicitly
 *  uses TCP protocol.
 * 
 * @param [buf] buffer containing data 
 * @param [len] length of data in buffer in bytes
 *  
 * @return ssize_t 
 *  the number of bytes sent or on fail -1
 */
ssize_t Client::Tcp_Send(const void* buf, const size_t len)
{
    return Send_Tcp(fds, buf, len);
} // end Tcp_Send


//==============================================================================================================|
/**
 * @brief 
 *  Wraps around the actual TCP receiving api.
 * 
 * @param [buf] buffer that will store the data 
 * @param [len] the length of the buffer in bytes
 *  
 * @return ssize_t 
 *  number of bytes recieved on success, -1 on fail
 */
ssize_t Client::Tcp_Recv(void *buf, const size_t len)
{
    return Recv_Tcp(fds, buf, len);
} // end Tcp_Recv


//==============================================================================================================|
/**
 * @brief 
 *  Wraps around the UDP sending function.
 * 
 * @param [buf] buffer containing data 
 * @param [len] length of data in buffer in bytes
 *  
 * @return ssize_t 
 *  the number of bytes sent or on fail -1
 */
ssize_t Client::Udp_Send(const void* buf, const size_t len)
{
    return Send_Udp(fds, buf, len, &ss, ss_len);
} // end Udp_Send


//==============================================================================================================|
/**
 * @brief 
 *  Wraps around the UDP based receving function.
 * 
 * @param [buf] buffer that will store the data 
 * @param [len] the length of the buffer in bytes
 *  
 * @return ssize_t 
 *  number of bytes recieved on success, -1 on fail 
 */
ssize_t Client::Udp_Recv(void *buf, const size_t len)
{
    return Recv_Udp(fds, buf, len, &ss, &ss_len);
} // end Udp_Recv


//==============================================================================================================|
//          THE END
//==============================================================================================================|