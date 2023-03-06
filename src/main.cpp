//==============================================================================================================|
// File Desc:
//  contains entry point for BluNile application
//
// Program Authors:
//  Rediet Worku, Dr. aka Aethiopis II ben Zahab       PanaceaSolutionsEth@gmail.com, aethiopis2rises@gmail.com
//
// Date Created:
//  8th of August 2022, Monday (after, my daughter's We'le'te-Sen'be't, baptisim)
//
// Last Updated:
//  26th of August 2022, Friday
//==============================================================================================================|



//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include "basics.h"
#include "utils.h"
#include "global-errors.h"
#include "zkteco-driver.h"


using namespace std;



//==============================================================================================================|
// GLOBALS
//==============================================================================================================|
int daemon_proc = 0;
APP_CONFIG config;          // an application configuration info



//==============================================================================================================|
// FUNCTIONS
//==============================================================================================================|
/**
 * @brief 
 *  the program entry point
 * 
 * @param [argc] command line argument count
 * @param [argv] command line arguments
 * 
 * @return int 
 */
int main(int argc, char **argv)
{
    /* decoration */
    string logo = "INTAPS Software Engineering Plc";
    string appname = "Test driving ZKteco driver";
    int logo_len = logo.length();
    int i;

    cout << "\n\033[35m";
    for (i = 0; i < logo_len + 4; i++)
        cout << "*";

    cout << "\n" << setw(logo_len + 2) <<  logo << endl;
    cout << setw(logo_len) << appname << endl;
    cout << "\033[35m";

    for (i = 0; i < logo_len + 4; i++)
        cout << "*";

    cout << "\033[37m\n" << endl;
    /* decoration */

    //cout << "Reading configuration file ..." << endl;
    // if (Read_Config(&config) < 0)
    //     Dump_Err_Exit("Reading configuration file"); 

    if (Connect_Net(0, "192.168.1.200", "4370") < 0)
        Fatal("ZKT eco connection error");
        
    cout << "Return of the Jedi" << endl;
    Init_Realtime(0);

    //Delete_User(0, 1);
    
    vector<User_Entry> users;
    Read_All_UserIDs(0, users);
    cout << "Printing user entries ...\n" << endl;
    for (auto &x : users)
        Print_User_Info(x);


    vector<Attendance_Entry> att;
    Read_Attendance_Record(0, att);
        //Dump_Err_Exit("Attendance record fetching");

    cout << "Printing attendance entries ...\n" << endl;
    for (auto &x : att)
        Print_Att_Info(x);


    //Restart_Device(0);
    ps_thread->join();
    
    Disconnect_Net(0);

    
    return 0;
} // end main


//==============================================================================================================|
//          THE END
//==============================================================================================================|
