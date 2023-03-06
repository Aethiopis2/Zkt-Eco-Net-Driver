//==============================================================================================================|
// File Desc:
//  defintions for support prototype functions
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



//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include "utils.h"



//==============================================================================================================|
// TYPES
//==============================================================================================================|



//==============================================================================================================|
// GLOBALS
//==============================================================================================================|



//==============================================================================================================|
// PROTOTYPES
//==============================================================================================================|
/**
 * @brief 
 *  a configuration file in WSIS BluNile is but a key-value pair, and C++ maps allow just to do that. The function
 *  opens a pre-set (canned) configuration file and reads its contents into a structure used to hold the values
 *  in memory for later referencing. This function should be called at startup.
 * 
 * @param [p_config] a buffer to store the contents of configuration file; APP_CONFIG type contains a map member
 *  that is used to store the actual configuration key,value pairs
 * 
 * @return int a 0 indicates success (or nothing happened as in parameter is null), alas -1 for error
 */
int Read_Config(APP_CONFIG_PTR p_config)
{
    // we can override this local by setting the environment variable 
    //  for configuration file name when BluNile starts
    std::string filename = "config.dat";
    char *env_name;         // storage ptr for getenv syscall

    // reject false calls
    if (!p_config)
        return 0;

    // read environment variable "BLUNILE_CONFIG_FILENAME", if it exists override
    //  the string filename (NOTE: this works best in Unix based environments)
    if ( (env_name = getenv("BLUNILE_CONFIG_FILE_NAME")) )
        filename = env_name;

    // open file for reading only this time
    std::ifstream config_file{filename};
    if (!config_file)
        return -1;      // read-open error

    
    // some storage buffers, and read in the file till the end
    std::string key, value;
    while (config_file >> quoted(key) >> quoted(value))
        p_config->dat[key] = value;

    
    return 0;
} // end Read_Config




//==============================================================================================================|
/**
 * @brief 
 *  this function splits a string at the positions held by "tokken" which is a character pointing where to split
 *  string. The function uses a std::getline to split and store the results in a vector of strings passed as 
 *  parameter 3.
 * 
 * @param [str] the buffer containing the strings to split
 * @param [tokken] the character marking the positions of split in string
 * @param [dest] the destination vector taking on all the splitted strings
 */
void Split_String(const std::string &str, const char tokken, std::vector<std::string> &dest)
{
    std::string s;      // get's the split one at a time
    std::istringstream istr(str.c_str());

    while (std::getline(istr, s, tokken))
        dest.push_back(s);
} // end Split_String



//==============================================================================================================|
/**
 * @brief 
 *  Initalizes a mutex, that we can keep to make multi-threaded applications thread safe (more or less).
 * 
 * @param [mutex] the newly created mutex returned here
 *  
 * @return int 
 *  a 1 on success 
 */
int Mutex_Init(MUTEX *mutex)
{
#if defined(__unix__) || defined(__linux__)
    return pthread_mutex_init(mutex, NULL);
#elif defined(_WIN32) || defined(_WIN64)
    *mutex = CreateMutex(0, FALSE, 0);
    return (*mutex == 0);
#endif
    return -1;    
} // end Mutex_Init


//==============================================================================================================|
/**
 * @brief 
 *  Cross platform mutex locking mechanism; simply compiles into the right platform.
 * 
 * @param [mutex] a handle or pointer to mutex
 *  
 * @return int 
 *  a 1 on success 
 */
int Mutex_Lock(MUTEX *mutex)
{
#if defined(__unix__) || defined(__linux__)
    return pthread_mutex_lock(mutex);
#elif defined(_WIN32) || defined(_WIN64)
    return (WaitForSingleObject(*mutex, INFINITE) == WAIT_FAILED ? 1 : 0);
#endif
    return -1; 
} // end Mutex_Lock


//==============================================================================================================|
/**
 * @brief 
 *  Cross platform mutex un-locking mechanism;
 * 
 * @param [mutex] a handle or pointer to mutex
 *  
 * @return int 
 *  a 1 on success 
 */
int Mutex_Unlock(MUTEX *mutex)
{
#if defined(__unix__) || defined(__linux__)
    return pthread_mutex_unlock(mutex);
#elif defined(_WIN32) || defined(_WIN64)
    return (ReleaseObject(*mutex) == 0);
#endif
    return -1; 
} // end Mutex_Lock


//==============================================================================================================|
/**
 * @brief 
 *  Prints the contents of buffer in hex notation along side it's ASCII form much like hex viewer's do it.
 * 
 * @param ps_buffer 
 *  the information to dump as hex and char arrary treated as a char array.
 * @param len 
 *  the length of the buffer above
 */
void Dump_Hex(const char *p_buf, const size_t len)
{
    size_t i, j;                    // loop var
    const size_t skip = 8;           // loop skiping value
    size_t remaining = skip;       // remaining rows


    if (len < skip)
        remaining = len;
    
    for (i = 0; i < len; i+=skip)
    {   
        printf("\t");
        for (j = 0; j < remaining; j++)
            printf("%02X ", (uint8_t)p_buf[i+j]);

        printf("\t\t");

        for (j = 0; j < remaining; j++)
            printf("%c", p_buf[i+j]);

        printf("\n");

        if (len - (i + j) < skip)
            remaining = len - (i + j);
    } // end for

    printf("\n");
} // end Dump_Hex

//==============================================================================================================|
//          THE END
//==============================================================================================================|