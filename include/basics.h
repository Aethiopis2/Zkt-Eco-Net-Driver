//==============================================================================================================|
// File Desc:
//  contains defintions, includes, types, and globals that are commonly used through out the WSIS application.
//
// Program Authors:
//  Rediet Worku, Dr. aka Aethiopis II ben Zahab       PanaceaSolutionsEth@gmail.com, aethiopis2rises@gmail.com
//
// Date Created:
//  7th of August 2022, Sunday (My daughter's We'le'te-Sen'be't, baptisim)
//
// Last Updated:
//  8th of August 2022, Monday
//==============================================================================================================|
#ifndef BASIC_HEADERS_H
#define BASIC_HEADERS_H



//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include <iostream>         // standard C++ io-stream
#include <iomanip>          // some formatters C++ style
#include <string>           // C++ style enhanced strings
#include <fstream>          // C++ way of file maniuplation
#include <vector>           // C++ style vectors/array
#include <memory>           // C++ memory and pointer maniuplations
#include <map>              // defintions for maps
#include <unordered_map>    // defintions for unordered maps
#include <queue>            // C++ qeues
#include <algorithm>        // many important iterator algorithims
#include <sstream>          // stream handling 
#include <iterator>         // basic c++ container iterators
#include <cstdint>          // some c++ platform type definitions
#include <thread>           // C++11 threads

#include <sys/types.h>          // some C style types
#include <string.h>             // C style strings and many impt C string libs
#include <errno.h>              // defines the global errno
#include <stdarg.h>             // ANSI C header file 
#include <inttypes.h>           // defines some platform types

#if defined (__unix__) || defined (__linux__)
#include <sys/socket.h>         /* basic socket functions */
#include <sys/time.h>           /* time_val {} for select */
#include <netinet/tcp.h>        /* some low level tcp stuff */
#include <netinet/in.h>         /* sockaddr_in {} definition */
#include <arpa/inet.h>          /* inet(3) functions */
#include <unistd.h>             /* many unix system calls */
#include <netdb.h>              /* extended net defintions */
#include <pthread.h>            /* POSIX threads */
#else 
#if defined(WIN32) || defined(_WIN64)
#include <Windows.h>            /* many Windows API */
#include <WinSock2.h>           /* windows socket library */
#endif
#endif



//==============================================================================================================|
// MACROS
//==============================================================================================================|
#ifndef MAX_PATH                // in Windows systems, this is defined
#define MAX_PATH        260     // but we can redefine it anyways
#endif 


#define BUFFER_SIZE     2048        // a generailzied storage buffer size (usually overrideable using config)

#define HAVE_VSNPRINTF  1           // let's cheat a bit

#define iCpy(d, s, l)   memcpy(d, s, l)
#define iZero(d, l)     memset(d, 0, l)


enum Boolean{FALSE, TRUE};


// basic type redefinitions
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;
typedef int64_t s64;
typedef __int128_t s128;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;



// some endian arragments; we'd want to be using these functions no problem regardless
// of the machine architecture be it big endian or little
#if __BIG_ENDIAN__
#define HTONS(x)    (x)         /* 16-bit version */
#define HTONL(x)    (x)         /* 32-bit version */
#define HTONLL(x)   (x)         /* 64-bit version */

#define NTHOS(x)    (x)
#define NTHOL(x)    (x)
#define NTHOLL(x)   (x)
#else
#define HTONS(x)    htons(x)
#define HTONL(x)    htonl(x)
#define HTONLL(x)   (((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))

// these reverse orders back into the host format; i.e. little-endian
#define NTOHS(x)    ntohs(x)
#define NTOHL(x)    ntohl(x)
#define NTOHLL(x)   (((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif


// some endian arragments; reversals of the above functions; i.e. these do the inverse
#if __BIG_ENDIAN__
#define RHTONS(x)    htons(x)         /* 16-bit version */
#define RHTONL(x)    htonl(x)         /* 32-bit version */
#define RHTONLL(x)   (((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))         /* 64-bit version */

#define RNTHOS(x)    ntohs(x)
#define RNTHOL(x)    ntohl(x)
#define RNTHOLL(x)   (((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#else
#define RHTONS(x)   (x)
#define RHTONL(x)   (x)
#define RHTONLL(x)  (x)

// these reverse orders back into the host format; i.e. little-endian
#define RNTOHS(x)    (x)
#define RNTOHL(x)    (x)
#define RNTOHLL(x)   (x)
#endif


// mutexs the cross platform way
#if defined(__unix__) || defined(__linux__)
#define MUTEX   pthread_mutex_t
#elif defined(WIN32) || defined(_WIN64)
#define MUTEX   HANDLE
#endif



#endif
//==============================================================================================================|
//          THE END
//==============================================================================================================|