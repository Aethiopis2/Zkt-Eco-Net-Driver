//==============================================================================================================|
// File Desc:
//  contains definitions for net-wrapper prototype functions
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
#if defined(WIN32) || defined(_WIN64)
#pragma comment(lib, "ws2_32.lib")   // required library in windows systems (better use visual studio for that!!!)
#endif




//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include "net-wrappers.h"




//==============================================================================================================|
// MACROS
//==============================================================================================================|



//==============================================================================================================|
// GLOBALS
//==============================================================================================================|




//==============================================================================================================|
// PROTOTYPES
//==============================================================================================================|
/**
 * @brief 
 *  Initalizes a series of protocol independant structures that can be used to start connections over TCP enabled
 *  networks using IPv4 or IPv6 formats.
 * 
 * @param [paddr] the address structure that stores the info about the peer 
 * @param [hostname] a host name or IP address to fetch addr info for and connect with 
 * @param [port] the communication port as string
 *  
 * @return int
 *  a 0 on success, alas a -1 with errno having details on error. 
 */
int Init_Addr(struct addrinfo **paddr, const std::string &hostname, const std::string &port)
{
    struct addrinfo hints;

    // is this Windows?
#if defined(WIN32) || defined(_WIN64)
    WSADATA wsa;        // required by the startup function

    // In windows systems we need to instruct the OS to include in sockets libraries into our app,
    //  we do this using a call to a startup function which returns 0 on success. In Unix based systems
    //  the sockets libraries are already loaded by the kernel. We request version 2.2 of WinSock libs
    if (WSAStartup(MAKEWORD(2,2), &wsa))
        return -1;      // WSAGetLastError() for details

#endif

    iZero(&hints, sizeof(hints));
    hints.ai_flags = 0;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname.c_str(), port.c_str(), &hints, paddr) != 0)
        return -1;

    return 0;  
} // end Init_Addr


//==============================================================================================================|
/**
 * @brief 
 *  This function starts a client side Tcp enabled connection using the first convinient IP protocol, i.e. IPv6
 *  vs IPv4, which is totaly dependant on the networking infrastructure.
 * 
 * @param [paddr] pointer to a structure containing the peer address info 
 * 
 * @return int 
 *  on success the descriptor to the socket is returned, on fail UNIX system call error -1 is propagated.
 */
int Connect_Tcp(struct addrinfo *paddr)
{
    int fds;
    struct addrinfo *palias = paddr;  // aliasing pointers makes life easy as a C programmer

    do {

        fds = socket(palias->ai_family, palias->ai_socktype, palias->ai_protocol);

        if (fds < 0)
            continue;  

        // now attempt connection with neo4j server
        if (connect(fds, palias->ai_addr, palias->ai_addrlen) == 0)
            return fds;       // success

        Close_Socket(fds);   // igonre this socket
    } while ( (palias = palias->ai_next) != NULL);
    
    // at the end of the day if socket is null
    return -1;
} // end Connect_TCP


//==============================================================================================================|
/**
 * @brief 
 *  Simpy closes the open file descriptor fds, i.e. param 1
 * 
 * @param [fds] an open file descriptor to a socket
 *  
 * @return int
 *  a 0 on success alas -1 
 */
int Close_Socket(int fds)
{
    return CLOSE(fds);
} // end Disconnect



//==============================================================================================================|
/**
 * @brief 
 *  sends the number of bytes sent in it's "len" parameter. The function persumes an already connected socket 
 *  descriptor "fds".
 * 
 * @param [fds] the socket descriptor
 * @param [buf] the buffer containing the bytes to send to peer
 * @param [len] the length of buffer in bytes
 * 
 * @return int the total number of bytes actually sent on success alas a -1
 */
int Send_Tcp(const int fds, const void* buf, const size_t len)
{
    int bytes_sent;                   // number of bytes sent in one go
    int total{0};                     // total bytes sent thus far should = n_len
    char *p_alias = (char*)buf;

    do {
        bytes_sent = (int)send(fds, p_alias, len, 0);

        if (bytes_sent <= 0)
            return -1;

        total += bytes_sent;
        p_alias += bytes_sent;
    } while (total < (ssize_t)len);

    return bytes_sent;
} // end Send_TCP


//==============================================================================================================|
/**
 * @brief 
 *  This function gets data from a peer on a TCP enabled network. The returned values are always less than or
 *  equal to the buffer len.
 * 
 * @param [fds] the socket descriptor
 * @param [p_buf] this is the buffer to store the received data
 * @param [len] length/size of the buffer
 * 
 * @return int count of bytes recieved or -1 on error with errno global having details
 */
int Recv_Tcp(const int fds, void *p_buf, const size_t len)
{
    int total_bytes{0};                    // the total bytes recieved

    if ( (total_bytes = (int)recv(fds, p_buf, len, 0)) < 0 )
        return -1;          // recive error

    return(total_bytes);    // return the count of bytes recieved
} // end Recv_Data_TCP


//==============================================================================================================|
/**
 * @brief 
 *  Send's data to a UDP connection which may or may not be connected; the slight performance penality here is
 *  we need to get the peer info everytime we choose to send some data.
 * 
 * @param [fds] a descriptor to socket
 * @param [p_buf] the info to send
 * @param [len] the length of info above
 * @param [len] size of buffer above
 * @param [ss] socket address describing the peer address info
 * @param [ss_len] length of the protocol independant address struct
 *  
 * @return int returns 0 if successful alas a -1 with errno with more details
 */
int Send_Udp(const int fds, const void *p_buf, const size_t len, struct sockaddr_storage *ss, socklen_t &ss_len)
{
    int total{0};
    int bytes_sent;
    char *p_alias = (char *)p_buf;

    if (getpeername(fds, (sockaddr*)ss, &ss_len))
        return -1;

    do {
        bytes_sent = (int)sendto(fds, p_alias, len, 0, (struct sockaddr *)&ss, ss_len);
        if (bytes_sent < 0)
            return -1;

        total += bytes_sent;
        p_alias += bytes_sent;
    } while (total < (int)len);

    return 0;
} // end Send_Data_UDP


//==============================================================================================================|
/**
 * @brief 
 *  receives data from its peer using UDP protocol in which the peer may be in a connected unconnected state.
 * 
 * NOTE
 *  the function persumes that the structure "ss" is in it's valid state; i.e. has been initalized by using calls
 *  to getpeername() system call.
 * 
 * @param [fds] the socket descriptor
 * @param [p_buf] a buffer to store the incomming data
 * @param [len] size of buffer above
 * @param [ss] socket address describing the peer address info
 * @param [ss_len] length of the protocol independant address struct
 *  
 * @return int the actual bytes recieved or -1 for an error 
 */
int Recv_Udp(const size_t fds, void *p_buf, const size_t len, struct sockaddr_storage *ss, socklen_t *ss_len)
{
    int bytes_recvd;

    if ( (bytes_recvd = (int)recvfrom(fds, p_buf, len, 0, (sockaddr*)ss, ss_len)) < 0 )
        return -1;

    // success
    return bytes_recvd;
} // end Recv_Data_UDP


//==============================================================================================================|
/**
 * @brief 
 *  Specifies the socket to time out in the number of seconds provided, this is useful for avoiding blocking
 *  recv calls, but there are better ways for that.
 * 
 * @param [fds] the socket descriptor 
 * @param [sec] the number of seconds to wait for
 *  
 * @return int
 *  a 0 on successful setting of the timeout, else -1 on fail 
 */
int Set_RecvTimeout(const int fds, const int sec)
{
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;

    // set socket options
    if (setsockopt(fds, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        return -1;
    
    return 0;
} // end Set_Recv_Timeout


//==============================================================================================================|
/**
 * @brief 
 *  Toggles the Tcp_NoDelay on/off; i.e. the Negel's algorithim (don't know what that is yet ...)
 * 
 * @param [fds] the socket descriptor 
 * @param [flag] the flag value when 0 = off when 1 = on
 *  
 * @return int 
 *  a 0 on success alas -1
 */
int Tcp_NoDelay(const int fds, u32 flag)
{
    if (setsockopt(fds, SOL_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag)) < 0)
        return -1;

    return 0;
} // end Tcp_NoDelay



//==============================================================================================================|
/**
 * @brief 
 *  Sends packets from the kernel that keeps a connection active; useful for long running daemon processes
 * 
 * @param [fds] the socket descriptor 
 * @param [flag] the flag value when 0 = off when 1 = on
 *  
 * @return int 
 *  a 0 on success alas -1
 */
int Keep_Alive(const int fds, u32 flag)
{
    if (setsockopt(fds, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)) < 0)
        return -1;

    return 0;
} // end Keep_Alive



//==============================================================================================================|
//          THE END
//==============================================================================================================|
