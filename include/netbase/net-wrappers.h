//==============================================================================================================|
// File Desc:
//  contains declerations for prototypes and functions used during network access using sockets api. The 
//  prototypes are simple wrappers for sockets api functions. The single logic behind this design is to reduce
//  the number of coding duplications that might arise during network access at the lower levels.
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
#ifndef NET_WRAPPERS_H
#define NET_WRAPPERS_H



//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include "basics.h"




//==============================================================================================================|
// MACROS
//==============================================================================================================|
// this little macro eases the system call differences between Windows OS and Unix derived systems such as linux, 
//  by redefining the system call depending on the platform at hand
#if defined(WIN32) || defined(_WIN64)
#define CLOSE(s)    closesocket(s)
#elif defined(__unix__) || defined(__linux__)
#define CLOSE(s)    close(s)
#endif



//==============================================================================================================|
// GLOBALS
//==============================================================================================================|




//==============================================================================================================|
// PROTOTYPES
//==============================================================================================================|
int Init_Addr(struct addrinfo **paddr, const std::string &hostname, const std::string &port);
int Connect_Tcp(struct addrinfo *paddr);
int Close_Socket(int fds);

int Send_Tcp(const int fds, const void* buf, const size_t len);
int Recv_Tcp(const int fds, void *buf, const size_t len);
int Send_Udp(const int fds, const void *p_buf, const size_t len, struct sockaddr_storage *ss, socklen_t &ss_len);
int Recv_Udp(const size_t fds, void *p_buf, const size_t len, struct sockaddr_storage *ss, socklen_t *ss_len);


int Set_RecvTimeout(const int fds, const int sec=3);
int Tcp_NoDelay(const int fds, u32 flag);
int Keep_Alive(const int fds, u32 flag);


#endif
//==============================================================================================================|
//          THE END
//==============================================================================================================|