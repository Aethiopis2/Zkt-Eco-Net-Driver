//==============================================================================================================|
// File Desc:
//  some prototypes for basic and support functions
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
#ifndef UTLITITES_H
#define UTLITITES_H



//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include "basics.h"



//==============================================================================================================|
// TYPES
//==============================================================================================================|
/**
 * @brief 
 *  An application configuration is but a key-value pair. When BluNile starts it reads the info stored at 
 *  "config.dat" file and caches the items in memory for later referencing.
 */
typedef struct APP_CONFIG_FILE
{
    std::map<std::string, std::string> dat;
} APP_CONFIG, *APP_CONFIG_PTR;



//==============================================================================================================|
// GLOBALS
//==============================================================================================================|



//==============================================================================================================|
// PROTOTYPES
//==============================================================================================================|
int Read_Config(APP_CONFIG_PTR p_config);
void Split_String(const std::string &str, const char tokken, std::vector<std::string> &dest);
void Dump_Hex(const char *p_buf, const size_t len);

int Mutex_Init(MUTEX *mutex);
int Mutex_Lock(MUTEX *mutex);
int Mutex_Unlock(MUTEX *mutex);

#endif


//==============================================================================================================|
//          THE END
//==============================================================================================================|