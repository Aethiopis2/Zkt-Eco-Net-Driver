//==============================================================================================================|
// File Desc:
//  defintion for "global-errors.h" prototypes
//
// Program Authors:
//  Rediet Worku, Dr. aka Aethiopis II ben Zahab       PanaceaSolutionsEth@gmail.com, aethiopis2rises@gmail.com
//
// Date Created:
//  7th of August 2022, Sunday (My daughter's We'le'te-Sen'be't, baptisim)
//
// Last Updated:
//  23rd of August 2022, Tuesday
//==============================================================================================================|



//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include "global-errors.h"


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
 *  generically outputs errors to the standard error stream if external global "deamon_proc" has not been set or
 *  is set to 0. If not then it logs the error to sys log in Unix/Linux systems.
 * 
 * @param [errno_flag] the errno global (see unix system calls and c lib standards)
 * @param [level] flag for syslog() system call, ususally is set to LOG_INFO
 * @param [fmt] the string containing user side error description formatted in c style to accept variable arguments
 * @param [ap] pointer to the variable arguments list
 */
static void Output_Err(int errno_flag, int level, const char *fmt, va_list ap)
{
    size_t n;                       // stores string length for buffers
    char buf[BUFFER_SIZE + 1];     
    int saved_errno = errno; 

#ifdef HAVE_VSNPRINTF               /* some systems may not yet define vsnprintf function as it was relatively new */
    vsnprintf(buf, BUFFER_SIZE, fmt, ap);
#else
    vsprintf(buf, fmt, ap);         /* or use the unsafe version/depreciated */
#endif

    n = strlen(buf);

    if (errno_flag) 
        snprintf(buf + n, BUFFER_SIZE - n, ": \033[31m%s\033[37m", strerror(saved_errno));
    strcat(buf, "\n");

    // now are we logging to system log or dumping to standard out?
    if (daemon_proc)
        syslog(level, buf);
    else 
    {
        // finally, do it the C++ way
        std::cerr << "\033[31m*** Err: " << buf << "\033[37m";
    } // end else
} // end Output_Err


//==============================================================================================================|
/**
 * @brief 
 *  terminates the application in one of 3 ways. If the environment variable EF_DUMPCORE has been set it exits
 *  by calling abort() if not it either exits by calling exit() or _exit() system call which have the effect of
 *  flushing the output buffer in the latter case.
 * 
 * @param [use_exit3] boolean when set false tells the function to use _exit() and flush the stdio buffers
 */
static void Terminate(Boolean use_exit3)
{
    char *s;

    /* Dump core if EF_DUMPCORE is defined and is a non-empty string; otherwise call
     *  exit(3) or _exit(2) depending on the value of use_exit variable
     */
    s = getenv("EF_DUMPCORE");

    if (s != NULL && *s != '\0')
        abort();
    else if (use_exit3)
        exit(EXIT_FAILURE);
    else 
        _exit(EXIT_FAILURE);
} // end Terminate


//==============================================================================================================|
/**
 * @brief 
 *  Dumps error to the standard error stream. This function is good for using against system call errors that
 *  do not require the application to terminate.
 * 
 * @param [fmt] a character pointer containing c style formatting for more arguments embedding
 * @param ... 
 */
void Dump_Err(const char *fmt, ...)
{
    va_list arg_list;           /* handles the variable length arguments */

    va_start(arg_list, fmt);
    Output_Err(1, LOG_INFO, fmt, arg_list);
    va_end(arg_list);
} // end Dump_Err


//==============================================================================================================|
/**
 * @brief 
 *  this is like the Dump_Err function except this function terminates the application by calling _exit()
 * 
 * @param [fmt] pointer character with c style formatting
 * @param ... 
 */
void Dump_Err_Exit(const char *fmt, ...)
{
    va_list arg_list;           /* handles the variable length arguments */

    va_start(arg_list, fmt);
    Output_Err(1, LOG_INFO, fmt, arg_list);
    va_end(arg_list);

    Terminate(TRUE);
} // end Dump_Err_Exit


//==============================================================================================================|
/**
 * @brief 
 *  this is more sutied to application specific errors rather than system call errors, this function also 
 *  terminates the application with exit()
 * 
 * @param [fmt] c sytle formatted string
 * @param ... 
 */
void Fatal(const char *fmt, ...)
{
    va_list arg_list;

    va_start(arg_list, fmt);
    Output_Err(0, LOG_INFO, fmt, arg_list);
    va_end(arg_list);

    Terminate(TRUE);
} // end Fatal



//==============================================================================================================|
//          THE END
//==============================================================================================================|