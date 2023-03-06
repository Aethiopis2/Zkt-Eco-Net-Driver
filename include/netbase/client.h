//==============================================================================================================|
// File Desc:
//  contains declerations class client at a raw level; which is mostly responsible for starting a network comm
//  using underlying protocols.
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
#ifndef CLIENT_H
#define CLIENT_H




//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include "basics.h"
#include "net-wrappers.h"



//==============================================================================================================|
// MACROS
//==============================================================================================================|
#define TCP_PROTO       0
#define UDP_PROTO       1


//==============================================================================================================|
// GLOBALS
//==============================================================================================================|




//==============================================================================================================|
// CLASS
//==============================================================================================================|
class Client
{
public:

    Client(const int protocol = TCP_PROTO);
    ~Client() {}

    int Tcp_Connect(const std::string &hostname, const std::string &port);
    int Select(void *buf, const size_t len);
    int Disconnect();

    int Send(const void *pbuf, const size_t len);
    int Recv(void *pbuf, const size_t len);
    int Set_Recv_Timeout(int sec=3);
    int Toggle_TcpDelay();
    int Toggle_KeepAlive();

    int Get_Socket();
    
private:

    int fds;                        // a socket descriptor
    fd_set rset;                    // read set used for select() based async read/write (we can control the write)
    addrinfo *paddr;                // holds address info of the peer
    struct sockaddr_storage ss;     // a storage long enough to store any address info
    socklen_t ss_len;               // length of the socket storage struct (varies according to protocol)

    u32 delaytcp;                   // a false int that determines if tcp is delayed or not
    u32 keep_alive;                 // boolean used to determine if we need to keep an idle connection
    
           
    // utilities
    ssize_t Tcp_Send(const void* buf, const size_t len);
    ssize_t Tcp_Recv(void *buf, const size_t len);
    ssize_t Udp_Send(const void* buf, const size_t len);
    ssize_t Udp_Recv(void *buf, const size_t len);

};


#endif
//==============================================================================================================|
//          THE END
//==============================================================================================================|