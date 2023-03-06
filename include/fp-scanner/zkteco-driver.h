//==============================================================================================================|
// File Desc:
//  This is a little module written in C (and a bit of C++ here and there) is used interface and download info
//  from ZKT eco devices both at demand and in realtime using the ZKT eco network packet format. ZKT eco follows
//  a defined packet format that is not related to other web application layer protocols such as HTTP/S. The device
//  allows connections using UDP & TCP. This module uses TCP for connection (because its a lot stabler), however
//  UDP in these instances can out perform TCP since most of the exchange of info is restricted to within a range
//  of few kilobytes (which is close to nothing in todays terra bits of memory).
//  
// Note:
//  google "ZKT eco packet format" for more info.
//
// Program Author:
//  Rediet Worku, Dr. aka Aethiopis II ben Zahab       PanaceaSolutionsEth@gmail.com, aethiopis2rises@gmail.com
//
// Date Created:
//  23rd of November 2022, Wedensday
//
// Last Updated:
//  23rd of November 2022, Wedensday
//==============================================================================================================|
#ifndef ZKTECO_DRIVER_H
#define ZKTECO_DRIVER_H



//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include "basics.h"
#include "client.h"




//==============================================================================================================|
// MACROS
//==============================================================================================================|
// ZKTEco request codes
#define CMD_CONNECT         1000            // begins a connection
#define CMD_EXIT            1001            // disconnects an active connection
#define CMD_ENABLE_DEVICE   1002            // changes the machine state to normal work
#define CMD_DISABLE_DEVICE  1003            // disables finger print, rfid, and keyboard
#define CMD_RESTART         1004            // restart the machine
#define CMD_POWEROFF        1005            // shuts down the device
#define CMD_SLEEP           1006            // puts the machine in idle state
#define CMD_AWAKEN          1007            // resumes to "awaken"
#define CMD_CAPTUREFINGER   1008            // caputre fingerprint
#define CMD_TEST_TMP        1009            // test if finger print exists
#define CMD_REFRESHDATA     1013            // refresh the machine stored data
#define CMD_REFRESHOPTION   1014            // refresh the configuration paramters
#define CMD_PREPARE_DATA    1500            // prepare for data transmission
#define CMD_DATA            1501            // data packet
#define CMD_FREE_DATA       1502            // release buffer used for data transmission
#define CMD_DATA_WRRQ       1503            // read/write large data sets
#define CMD_DATA_RDY        1504            // inidcates that its ready to receive data
#define CMD_AUTH            1102            // begins authorized connection
#define CMD_USER_WRQ        8               // upload user data
#define CMD_OPTIONS_RRQ     11              // read configuration value of the machine
#define CMD_OPTIONS_WRQ     12              // change the congifuration value of the machine
#define CMD_DELETE_USER     18              // deletes a user from the machine given her/his user_sn
#define CMD_CLEAR_ADMIN     20              // clears all admin previlages
#define CMD_GET_FREE_SIZES  50              // get the machines remaining space
#define CMD_ENABLE_CLOCK    57              // enables the "." on the screen clock
#define CMD_STARTVERIFY     60              // set the machine to authetication state
#define CMD_STARTENROLL     61              // start the enrolling procedure
#define CMD_CANCELCAPTURE   62              // disable normal authetication of users
#define CMD_GET_TIME        201             // request machine time
#define CMD_SET_TIME        202             // set device time
#define CMD_REG_EVENT       500             // real time packets




// ZKTEco reply codes
#define CMD_ACK_OK              2000            // request was successful
#define CMD_ACK_ERROR           2001            // an error processing the request
#define CMD_ACK_DATA            2002
#define CMD_ACK_RETRY 	        2003
#define CMD_ACK_REPEAT 	        2004
#define CMD_ACK_UNAUTH          2005            // unauthorized access
#define CMD_ACK_ERROR_CMD 		65533
#define CMD_ACK_ERROR_INIT 		65532
#define CMD_ACK_ERROR_DATA 		65531
#define CMD_ACK_UNKNOWN         65535 	        // Received unknown command. 	


// flags used during finger print enrollment
#define INVALID_FINGERPRINT     0
#define VALID_FINGERPRINT       1
#define DURESS_FINGERPRINT      3



// the permission levels; the permission is a byte value encoded to convey 
//  meanings on the level of permission (I simply define them as macro constants)
#define E_ENABLE(bit)           bit & 1
#define COMMON_USER(bit)        (bit >> 1) & (000b)
#define ENROLL_USER(bit)        (bit >> 1) & (001b)
#define ADMIN(bit)              (bit >> 1) & (011b)
#define SU(bit)                 (bit >> 1) & (111b)



// makes it easy to compute the size of payload without data
#define PAYLOAD_SIZE    (sizeof(u16) << 2)



// makes it easy to compute the size of entire packet without data since everything is fixed
#define PACKET_SIZE     (PAYLOAD_SIZE + 8)



// upper limit on ZKT data sizes for our application
#define ZKT_DATA_SIZE       2048        // a 2kb space for each packet! that's neat...



//==============================================================================================================|
// TYPES
//==============================================================================================================|
/**
 * @brief 
 *  All ZKT eco packets contain a payload structure which contains info on the type of request (command_id) which
 *  and is a 16-bit value and it's payload data which is variable in length; the other three 16-bit values are 
 *  checksum (which is used by the device to verify the validty of packet), the session_id which is set by device 
 *  after the first CMD_CONNECT (no matter what) for realtime packets this value is always an id for the realtime 
 *  event that occurred and reply_number, which tracks the number of packets exchanged between device and 
 *  application (16-bit aye, what happens after reply no: 65535? wrap around?) for realtime packets the reply 
 *  number is set to 0
 */
typedef struct Zkt_Payload_Format
{
    u16 command_id;             // one of those defined request ids
    u16 checksum;               // used to verify the vality of packet, mess this up and device will hang ...
    u16 session_id;             // set by the machine (when in real time this serves as real time codes)
    u16 reply_number;           // take a wild guess?
    u8 *data{nullptr};          // the payload data
} Payload, *Payload_Ptr;




/**
 * @brief 
 *  A ZKT eco packet format always begins with a 32-bit magic number that tells the device and its peer that
 *  this could be an actual ZKT eco packet format, since I don't want to worry about byte-ordering at this
 *  point and since value never chagnes, I'll define it as byte array. And then we have a 32-bit value
 *  which holds the size of payload data down below (not including the header and itself). Next is the payload
 *  data itself which holds the actual request/response info.
 */
typedef struct Zkt_Packet_Format
{
    u8 header[4]{0x50, 0x50, 0x82, 0x7d};       // ZKT eco packet identifier
    u32 payload_size;                           // the size of payload down below
    Payload payload;                            // the payload data which contains the actual info
} Zkt_Packet, *Zkt_Packet_Ptr;




/**
 * @brief 
 *  The attendance log format; each attendance entry consists of an id and serial number followed by
 *  the verification mode (fingerprint) and status (in/out) and the time of attendance log.
 */
#pragma pack(1)
typedef struct Attendance_Log_Format {
    u16 serial_number;          // the user serial
    u8 user_id[9];              // user identification
    u8 pad[15];                 // zeros
    u8 verify_type;             // the verification type (see constants above)
    u32 att_time;               // time of attendance
    u8 verify_state;            // the verification state (see constants above)
    u8 fixed[8]{0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00};
} Attendance_Entry, *Attendance_Entry_Ptr;




/**
 * @brief 
 *  When the device sends an attendance log info in realtime as response to whenever someone checks in/out
 *  the data part of the payload contains the info on the attendance log, this info is a structure that contains
 *  info on the user_id (who logged), the verification type (for our sake it's always fingerprint), the status
 *  i.e. check in/out or overtime in/out (see defines above for more info), and offcourse the time of
 *  transaction encoded in YY MM DD HR MN SEC format (each being a byte value).
 */
#pragma pack(1)
typedef struct Attendance_Realtime_Log_Format
{
    char user_id[9];        // the user id
    char unused[15];        // all zeros
    u8 verifyType;          // see defined constants above; basically tells the verification mode
    u8 status;              // one of the defined types; check-in,out,ovettime, etc..
    char att_time[6];       // the time of attendance log
} Att_Realtime_Log, *Att_Realtime_Log_Ptr;




/**
 * @brief 
 *  Structure used during enrolling of fingers
 */
#pragma pack(1)
typedef struct Enroll_Data_Format
{
    u8 user_id[24];         // user id given as string padded with zeros
    u8 finger_index;        // index finger (one of the nine possiblites)
    u8 fp_flag;             // finger print flag (see constants above)
} Enroll_Data, *Enroll_Data_Ptr;




/**
 * @brief 
 *  Machine status info returned during calls to free sizes
 */
#pragma pack(1)
typedef struct Machine_Status_Info
{
    u8 pad1[16];        // all zeros
    u32 user_count;     // number of users
    u8 pad2[8];
    u32 fp_template;    // number of finger print templates on the machine
    u32 pwd_count;      // number of passwords
} Machine_Status, *Machine_Status_Ptr;




/**
 * @brief 
 *  Represents a user entry format retured during Get_USer_Info calls
 */
#pragma pack(1)     // force compiler to use 1 byte packaging
typedef struct User_Entry_Format
{
    u16 serial_number;          // the users serial number
    u8 permissions;             // the permission flag for the user
    char password[8];           // the user password (not longer than 8 chars long)
    char name[24];              // the user's name ofcourse
    u32 card_number;            // not used from where I'm around ...
    u8 group_number;            // the group number to which the user belongs to
    u16 user_tzs;               // indicates the user's time zone (native to user)
    u16 tz1;                    // users time zones
    u16 tz2;
    u16 tz3;
    char user_id[9];            // the user id stored as character array aka string
    u8 padding[15]{0};          // all zeros
} User_Entry, *User_Entry_Ptr;




/**
 * @brief 
 *  A little structure used during setting and getting of verification info
 */
#pragma pack(1)
typedef struct Veryfiy_Info_Format
{
    u16 user_serial;        // take a wild guess?
    u8 verification_mode;   // the mode of identification (see contants above)
    u8 pad[21]{0};          // padding
} Verify_Info, *Verify_Info_Ptr;





/**
 * @brief 
 *  Custom structure that stores basic info on client side connection, and pointer to store responses from
 *  our little machine, and session plus reply nums required during on demand requests. This design allows 
 *  us to keep track of each machines request/reply out of each others hair; i.e. each machine keeps its 
 *  own structures.
 */
typedef struct Intaps_Driver_Info_Struct
{
    Client cli;                                  // object is our client connection interface
    std::unordered_map<u16, Zkt_Packet> que;     // a queue of responses mapped as reply_num : que
    u16 session_id{0};          // the session id for this connection
    u16 reply_num{0};           // the current reply number
    bool bok{false};            // used during fetching as crude wait
    bool bconnected{false};     // connection state
    std::string err;            // dumps error      
} Driver_Info, *Driver_Info_Ptr;



//==============================================================================================================|
// GLOBALS
//==============================================================================================================|
extern std::thread *ps_thread;





//==============================================================================================================|
// PROTOTYPES
//==============================================================================================================|
// internals
int Get_Response(const int machine_num, int reply_num, Zkt_Packet &zkt);
void Process_Response(const int machine_num, Zkt_Packet_Ptr ppack);
void Run_Select(const int machine_num);
std::string Whats_Last_Error(const int machine_num);



// terminal operations
int Connect_Net(const int machine_num, const std::string &ip, const std::string &port, const int password=0);
int Disconnect_Net(const int machine_num);
int Get_Device_Status(const int machine_num, Machine_Status *pstat);
int Get_Time(const int machine_num, u32* ptime);
int Refresh(const int machine_num, const u16 command_id=CMD_REFRESHDATA);
int Set_Time(const int machine_num, const u32 _time1);


// data operations
int Data_Ready(const int machine_num, const u32 dlen);
int Read_All_UserIDs(const int machine_num, std::vector<User_Entry> &users);    
int Read_Attendance_Record(const int machine_num, std::vector<Attendance_Entry> &entry);
int Delete_User(const int machine_num, const u16 user_sn);
int Init_Realtime(const int machine_num, const u32 options={1}); 
int Set_User_Info(const int machine_num, User_Entry_Ptr puser);


// other operations
int Enroll_User(const int machine_num, const Enroll_Data &enroll);
int Enable_Device(const int machine_num);
int Disable_Device(const int machine_num); 
int Clear_Admins(const int machine_num);
int Enable_Clock(const int machine_num);
int Start_Identify(const int machine_num);
int Cancel_Operation(const int machine_num);
int Restart_Device(const int machine_num);
int Power_Off(const int machine_num);
int Read_Machine_Config(const int machine_num, const std::string &query, std::string &result);



inline u16 Checksum(Payload_Ptr ppload, u16 *data=nullptr, u32 data_len=0);
inline u32 Commkey(const u16 session_id, const u32 password, const u8 ticks=50);
inline bool Alphanumeric_Support(const std::string &str);
void Print_User_Info(User_Entry &info);
void Print_Att_Info(Attendance_Entry &info);


#endif
//==============================================================================================================|
//          THE END
//==============================================================================================================|