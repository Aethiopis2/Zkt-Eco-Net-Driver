//==============================================================================================================|
// File Desc:
//  some prototypes for error handlers. In most cases BluNile uses C style error handling, i.e. checking the 
//  return types of function calls. On Some occasions BluNile may call these functions inside of try...catch block.
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
#ifndef GLOBAL_ERRORS_H
#define GLOBAL_ERRORS_H



//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include "basics.h"
#include <syslog.h>         /* for syslog */



//==============================================================================================================|
// MACROS
//==============================================================================================================|
/* this macro stops 'gcc -Wall' complaining that "control reaches end of non void function"
    if we use the following functions to terminate main() or some other non-void function */
#ifdef __GNUC__

#define NORETURN __attribute__ ((__noreturn__))
#else
#define NORETURN

#endif


//==============================================================================================================|
// GLOBALS
//==============================================================================================================|
extern int daemon_proc;   /* set by daemon_init() to a non zero value whenever the process runs as a daemon */



//==============================================================================================================|
// PROTOTYPES
//==============================================================================================================|
void Dump_Err(const char *fmt, ...);
void Dump_Err_Exit(const char *fmt, ...);
void Fatal(const char *fmt, ...);


#endif


//==============================================================================================================|
//          THE END
//==============================================================================================================|