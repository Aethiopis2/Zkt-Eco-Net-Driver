//==============================================================================================================|
// File Desc:
//  contains implementation for Zkteco-driver functions. ZKT eco device packet format is arranged in little endian
//  format, which is cool for Intel and Intel compatible processors, since some architectures are big endian we
//  handle those situtations using reveresed macros R' macros that do the opposite of the standarad network 
//  functions used in byte arragment issues.
//
// Program Authors:
//  Rediet Worku, Dr. aka Aethiopis II ben Zahab       PanaceaSolutionsEth@gmail.com, aethiopis2rises@gmail.com
//
// Date Created:
//  23rd of November 2022, Wedensday
//
// Last Updated:
//  23rd of November 2022, Wedensday
//==============================================================================================================|



//==============================================================================================================|
// INCLUDES
//==============================================================================================================|
#include "zkteco-driver.h"
#include "utils.h"



//==============================================================================================================|
// MACROS
//==============================================================================================================|
// extends the 8-bit value as 32-bit by simply duplicating the number, n
#define DUP32BIT(n)     ((u32)n << 24) | ((u32)n << 16) | ((u32)n << 8) | ((u32)n)


// exchanges the 16-bit values; i.e. swaps the HO word with the LO word; 
//  identical to the x86 XCHG instruction...
#define SWAP_WORDS(n)   ((u32)n << 16) | ((u32)n >> 16)


// fills up a payload structure
#define SET_PAYLOAD(pld, id, session, reply)    { pld.command_id = RHTONS(id); \
    pld.session_id = RHTONS(session); pld.reply_number = RHTONS(reply); }


// fills up the packet
#define SET_PACKET(pack, chksum, pld_size) { pack.payload.checksum = RHTONS(chksum); \
    pack.payload_size = RHTONL(pld_size); }



// used for sending requestes requiring nothing more than an OK response.
#define ACT_NODATA(machine_num, cmdid) { \
    Zkt_Packet snd, rcv; \
    u16 rnum{rq[machine_num].reply_num++}; \
    ACT(machine_num, snd, rcv, cmdid, rq[machine_num].session_id, rnum, Checksum(&snd.payload), 0); \
    RSP_OK(rcv.payload.command_id, machine_num); \
} // end NODATA_ACT



// shorten's a little code redundancy ... sending the packet twice fixes up some bugs
//  related to sending the address of data instead of its content... 
#define ACT(machine_num, snd, rcv, cid, ssid, rnum, chksum, dlen) { \
    SET_PAYLOAD(snd.payload, cid, ssid, rnum); \
    SET_PACKET(snd, chksum, PAYLOAD_SIZE + dlen); \
    /*Dump_Hex((char*)&snd, PACKET_SIZE); \
    Dump_Hex((char*)snd.payload.data, dlen);*/\
    if (rq[machine_num].cli.Send(&snd, PACKET_SIZE) < 0) return -1; \
    if (dlen > 0) { if (rq[machine_num].cli.Send(snd.payload.data, dlen) < 0) return -1; } \
    if (Get_Response(machine_num, rnum, rcv) < 0) return -1; \
    /*Dump_Hex((char*)&rcv, PACKET_SIZE); \
    Dump_Hex((char*)rcv.payload.data, rcv.payload_size - PAYLOAD_SIZE); */\
} // end ACT macro



// redudanduncy help #2 -- bundles code that only changes in one or two parameters; this macro
//  is used for zkt packets that add extra data info on their requests but require nothing more
//  than an OK response; CMD_AUTH for example.
#define ACT_INDATA(machine_num, cmd_id, dat, dlen) { \
    Zkt_Packet snd, rcv; \
    u16 rnum{rq[machine_num].reply_num++}; \
    snd.payload.data = (u8*)dat; \
    ACT(machine_num, snd, rcv, cmd_id, rq[machine_num].session_id, rnum, \
        Checksum(&snd.payload, (u16*)dat, dlen >> 1), dlen); \
    RSP_OK(rcv.payload.command_id, machine_num); \
} // end ACT_INDATA




// redundancy remover step #3 -- those that require an extra response from the machine
//  besides an OK
#define ACT_OUTDATA(machine_num, cmd_id, dat_in, in_len, dat_out, out_len) { \
    Zkt_Packet snd, rcv;    \
    u16 rnum{rq[machine_num].reply_num++}; \
    snd.payload.data = (u8*)dat_in; \
    ACT(machine_num, snd, rcv, cmd_id, rq[machine_num].session_id, rnum, \
        Checksum(&snd.payload, (u16*)dat_in, in_len >> 1), in_len); \
    if (rcv.payload.data) { \
        out_len = rcv.payload_size - PAYLOAD_SIZE; \
        dat_out = malloc(out_len); \
        if (!dat_out) { return -1; }  \
        iCpy(dat_out, rcv.payload.data, out_len); \
        FREE_BUF(rcv.payload.data); \
    } \
} // end act all



// tests response if being ok or not, and set's the global error string returns -2
#define RSP_OK(cid, mnum) { if (RNTOHS(cid) != CMD_ACK_OK) { rq[mnum].err = "Device returned code: " + std::to_string(cid); \
    return -2; } \
} // end RSP_OK



// decodes ZKT Eco 32-bit date-time format
#define DECODE_DATE(fmt, sec, min, hr, day, mon, yr) { \
    u32 f = RNTOHL(fmt); \
    sec = f % 60; \
    min = (f / 60) % 60; \
    hr = (f / 3600) % 24; \
    day = ((f / (3600 * 24)) % 31) + 1; \
    mon = ((f / (3600 * 24 * 31)) % 12 ) + 1; \
    yr = ((f / (3600 * 24)) / 365) + 2000; \
} // end Decode_Date




// small gc
#define FREE_BUF(dat) { if (dat) { free(dat); dat = nullptr; } }



//==============================================================================================================|
// GLOBALS
//==============================================================================================================|
// Key value pair of client connections plus a couple of more info; i.e. mapped with machine num to response info.
std::unordered_map<int, Driver_Info> rq;
std::thread *ps_thread;                         // c++11 thread (makes it nice since its cross-platform)
MUTEX mutex;                                    // memory protection in case we need any

// states
u32 connenction_count{0};           // tracks active connections
bool brunning{true};                // controls the life-time of Run_Select loop



//==============================================================================================================|
// FUNCTIONS
//==============================================================================================================|


//==============================================================================================================|
// INTERNALS
//==============================================================================================================|
/**
 * @brief 
 *  Device response; simply bundles the sending and receving stages in one go
 * 
 * @param [len] length of packet for the sending stage
 * 
 * @return int 
 *  a 0 on success alas -1
 */
int Get_Response(const int machine_num, const int reply_num, Zkt_Packet &zkt)
{
    // note: this could be a bad approach, since under unforseen extreme cases our loop
    //  may never exit; a better approach would have been to use signaling and wake the 
    //  process up whenever things are ready; however in the real world, I've got no time

    while (!rq[machine_num].bok);
    
    //Mutex_Lock(&mutex);

    // iterate thru the rcvers and get me the message 
    auto it = rq[machine_num].que.find(reply_num);
    if (it != rq[machine_num].que.end())
    {
        iCpy((void*)&zkt, (void*)&it->second, PACKET_SIZE);
        if (it->second.payload_size - PAYLOAD_SIZE > 0) {
            if ( !(zkt.payload.data = (u8*)malloc(it->second.payload_size - PAYLOAD_SIZE)))
                return -1;
            iCpy(zkt.payload.data, it->second.payload.data, it->second.payload_size - PAYLOAD_SIZE);
        } // end if

        rq[machine_num].que.erase(it);
        rq[machine_num].bok = false;
        return 0;
    } // end if

    //Mutex_Unlock(&mutex);
    return -1;
} // end Get_Response


//==============================================================================================================|
/**
 * @brief 
 *  handles the response into one of the following classes; realtime and non-realtime (on demand) packets; these
 *  are distingushed by their command id. For non realtime packets we add the whole package into a map as a form
 *  of queue and let caller worry about it. For realtime we invoke its handler by passing the data as a Attendance
 *  transaction log; all other realtime packets remain unimplemented.
 * 
 * @param [machine_num] a descriptor that identifies the machine we are connecting with
 * @param [ppack] pointer to the ZKT packet format containing the device responses 
 */
void Process_Response(const int machine_num, Zkt_Packet_Ptr ppack)
{
    if (ppack->payload.command_id != CMD_REG_EVENT)
    {
        // this is not a realtime packet add it to the global map so that
        //  its caller can find it.
        //rq[machine_num].que[ppack->payload.reply_number] = *ppack;
        iCpy((void*)&rq[machine_num].que[ppack->payload.reply_number], 
            ppack, PACKET_SIZE);

        if ( !(rq[machine_num].que[ppack->payload.reply_number].payload.data = (u8*)malloc(
            ppack->payload_size - PAYLOAD_SIZE)))
            return;

        iCpy(rq[machine_num].que[ppack->payload.reply_number].payload.data, 
            ppack->payload.data, ppack->payload_size - PAYLOAD_SIZE);
        rq[machine_num].bok = true;
    } // end if not real
    else {
        // this is a realtime packet; invoke its handler pronto, i.e. the callback
        //  and make sure its an attendance data; other types, sorry no can do ...
        if (RNTOHS(ppack->payload.session_id) == 1)      // attendance
        {
            Att_Realtime_Log att;
            iCpy(&att, ppack->payload.data, sizeof(att));
            att.user_id[8] = '\0';
            fputs(att.user_id, stdout);
            printf("\nTime: 20%d/%d/%d %d:%d:%d\n", (u8)att.att_time[0], (u8)att.att_time[1],
                (u8)att.att_time[2], (u8)att.att_time[3], (u8)att.att_time[4], (u8)att.att_time[5]);
        } // end if attendance

        // igonre all others
    } // end else

    // a little house cleaning ...
    if (ppack->payload.data) {
        free(ppack->payload.data);
        ppack->payload.data = nullptr;  // double tap
    } // end if
} // end process


//==============================================================================================================|
/**
 * @brief 
 *  Runs select system call in infinite mode to achievie a better form of non-blocking sockets that are async in
 *  nature, with the excpetion of Realtime signals, all other responses are stored into this semi queue structure
 *  which maps resposese with their reply numbers for avoiding ambiguity.
 * 
 * @param [mn] the machine identifer; set by machine or user (see Set_Machine_Num() method for details)
 * 
 */
void Run_Select(const int machine_num)
{
    // here we re-tailor the select sys call to meet the needs of our app; we want to allocate enough
    //  memory for our response since I don't wanna go back and forth for more data.
    fd_set rset;        // reading set
    FD_ZERO(&rset);

    while (brunning)
    {
        Zkt_Packet rcv;

        FD_SET(rq[machine_num].cli.Get_Socket(), &rset);
        int maxfdp1 = rq[machine_num].cli.Get_Socket() + 1;

        // block on select sys call instead of plain recieve, this allows a form of robustness for our thread
        //  since the waiting is handled by the kernel
        if (select(maxfdp1, &rset, NULL, NULL, NULL) > 0)
        {
            // our receving end is ready for reading (after about infity time wait...), but
            //  make sure it is, since this is the way to go.
            if (FD_ISSET(rq[machine_num].cli.Get_Socket(), &rset))
            {
                // first up read in the header + payload info without the data; that should tell
                //  us everything we need...
                int bytes;
                if ( (bytes = rq[machine_num].cli.Recv(&rcv, PACKET_SIZE)) < 0)
                    break;

                // test if we got more data, that info should be stored in payload_size feild
                //  of our little packet description
                if (rcv.payload_size > PAYLOAD_SIZE)
                {
                    // compute the data length
                    u32 len = rcv.payload_size - PAYLOAD_SIZE;
                    if ( !(rcv.payload.data = (u8*)malloc(len)))
                         break;

                    u8 *alias = rcv.payload.data;
                    //u32 llen = len;
                    iZero(rcv.payload.data, len);
                    do {
                        if ( (bytes = rq[machine_num].cli.Recv(alias, len)) < 0)
                            break;

                        alias += bytes;
                        len -= bytes;
                    } while (len > 0);
                    //Dump_Hex((char *)rcv.payload.data, llen);
                } // end if more data

                Mutex_Lock(&mutex);
                Process_Response(machine_num, &rcv);
                Mutex_Unlock(&mutex);
            } // end if set
        } // end if selecting
        else
            break;
    } // end while
} // end Run_Select


//==============================================================================================================|
/**
 * @brief 
 *  returns the last error as string for the connection
 * 
 * @param [machine_num] identifier
 * 
 * @return std::string 
 *  the error
 */
std::string Whats_Last_Error(const int machine_num)
{
    return rq[machine_num].err;
} // end Set_Request



//==============================================================================================================|
// TERMINAL OPERATIONS
//==============================================================================================================|
/**
 * @brief 
 *  Connects to a ZKTeco device on TCP/IP enabled network. It begins by sending CMD_CONNECT followed by CMD_AUTH
 *  for authenticated connection to start a session.
 * 
 * NOTE:
 *  If a connection fails with CMD_UNAUTH then it must mean that the device has a password set. I have inferred
 *  this password to be a 32-bit wide integer because the only place its used is in authentication in 'Commkey(...)'
 *  function, which the function treats the password as 32-bit value during hashing.
 * 
 * @param [machine_num] the machine identifer
 * @param [ip] the ip address for the device
 * @param [port] the device port number
 * @param [password] the device password (if set, ZKT eco U280 at INTAPS wasn't so, that that!!!)
 * 
 * @return int 
 *  a 0 for success. -ve number on error.
 */
int Connect_Net(const int machine_num, const std::string &ip, const std::string &port, const int password)
{
    Zkt_Packet snd, rcv;    // sending and rcving packets
    Driver_Info di;         // some driver info

    rq[machine_num] = di;
    if ( (rq[machine_num].cli.Tcp_Connect(ip, port)) < 0)
        return -1;

    rq[machine_num].cli.Toggle_TcpDelay();
    rq[machine_num].cli.Set_Recv_Timeout();
    rq[machine_num].cli.Toggle_KeepAlive();

    // fire up select thread; which reterives our response in async
    ps_thread = new std::thread(Run_Select, machine_num);
    Mutex_Init(&mutex);
    ACT(machine_num, snd, rcv, CMD_CONNECT, 0, 0, Checksum(&snd.payload), 0);

    // save session id and all 
    rq[machine_num].session_id = RNTOHS(rcv.payload.session_id);

    // when the device allows for unauthorized connections, it simply responds with
    //  CMD_ACK_OK message, if that's the case we done, end exit here; however mostly
    //  device is configured to only allow authorized connections thus we request for connection
    //  using CMD_AUTH command id, and if all goes well device should respond with CMD_ACK_OK.
    if (RNTOHS(rcv.payload.command_id) == CMD_ACK_UNAUTH)
    {
        u32 hash = Commkey(rq[machine_num].session_id, password);      
        ACT_INDATA(machine_num, CMD_AUTH, &hash, sizeof(hash));

        // test response; plz don't be CMD_ACK_UNAUTH again; it means we don't know
        //  the freaking password ...
        rq[machine_num].reply_num = 3;
    } // end if unauthorized

    return 0;
} // end if Connect_Net


//==============================================================================================================|
/**
 * @brief 
 *  Disconnect's the connected session; removes the item from the que map; if on the last connection sets brunning
 *  to false to exit the Run_Select thread.
 * 
 * @param [machine_num] the machine identifer
 * 
 * @return int 
 *  0 on success, -1 on fail.
 */
int Disconnect_Net(const int machine_num)
{
    ACT_NODATA(machine_num, CMD_EXIT);
    if (rq[machine_num].cli.Disconnect() < 0)
        return -1;
    
    rq.erase(machine_num);
    if (rq.size() == 0)
    {
        brunning = false;
        //ps_thread->join();  // wait for it
    } // end if

    return 0;
} // end Disconnect


//==============================================================================================================|
/**
 * @brief 
 *  Get's the status of the device like its user count and all
 * 
 * @param [machine_num] the machine identifier
 * @param [status] gets the status info from device
 * 
 * @return int 
 *  0 on success, -1 on fail
 */
int Get_Device_Status(const int machine_num, Machine_Status *pstat)
{
    u32 len;        // length in bytes
    void *palias = (void *)pstat;

    ACT_OUTDATA(machine_num, CMD_GET_FREE_SIZES, nullptr, 0, palias, len);

    return 0;  
} // end Get_Device_Status


//==============================================================================================================|
/**
 * @brief 
 *  Get's the device time
 * 
 * @param [machine_num] the machine identifier
 * @param [time] gets the device time encoded in particular format
 * 
 * @return int 
 *  0 on success, -1 on fail
 */
int Get_Time(const int machine_num, u32* ptime)
{
    void *palias = (void*)ptime;
    u32 len;
    ACT_OUTDATA(machine_num, CMD_GET_TIME, nullptr, 0, palias, len);
    
    return 0;  
} // end Get_Device_Time


//==============================================================================================================|
/**
 * @brief 
 *  Refresh device
 * 
 * @param [machine_num] the machine identifier
 * 
 * @return int 
 *  0 on success, -1 on fail
 */
int Refresh(const int machine_num, const u16 command_id)
{
    ACT_NODATA(machine_num, command_id);
    return 0;  
} // end REferesh


//==============================================================================================================|
/**
 * @brief 
 *  Set's the device time
 * 
 * @param [machine_num] the machine identifier
 * @param [time] gets the device time encoded in particular format
 * 
 * @return int 
 *  0 on success, -1 on fail
 */
int Set_Time(const int machine_num, const u32 _time1)
{
    ACT_INDATA(machine_num, CMD_SET_TIME, &_time1, sizeof(_time1));
    if (Refresh(machine_num) < 0)
        return -1;
        
    return 0;  
} // end Set_Time




//==============================================================================================================|
//  DATA OPERATIONS
//==============================================================================================================|
/**
 * @brief 
 *  Returns the entire she-bang of users stored in the device.
 * 
 * @param [machine_num] the device identifer 
 * @param [users] vector of user infos
 *  
 * @return int 
 *  0 on success alas -1 on fail
 */
int Read_All_UserIDs(const int machine_num, std::vector<User_Entry> &users)
{
    Zkt_Packet snd, rcv;

    // the meaining of these values have not yet been deciphered ...
    u8 dat[11]{0x01, 0x09, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    Disable_Device(machine_num);
    u16 rnum = rq[machine_num].reply_num;

    snd.payload.data = dat;
    ACT(machine_num, snd, rcv, CMD_DATA_WRRQ, rq[machine_num].session_id, 
        rnum, Checksum(&snd.payload, (u16*)&dat, 5), 11);
    
    ++rq[machine_num].reply_num;
    switch (rcv.payload.command_id) 
    {
        case CMD_DATA:      // data has been appeneded to this response
        {
            // the first four bytes of data tell us the length of the entire payload
            //  junk in bytes, thus let's take in that info and convert it to counts
            //  by dividing with a user_entry struct size.
            
            u32 len = *((u32*)rcv.payload.data) / sizeof(User_Entry);
            User_Entry_Ptr pusr = (User_Entry_Ptr)(rcv.payload.data + 4);
            u32 i = 0;
            while (i++ < len) 
            {
                User_Entry user = *pusr;
                users.push_back(user);
                pusr = (User_Entry_Ptr)((rcv.payload.data + 4) + (i * sizeof(User_Entry)));
            } // end while
        } break;

        case CMD_ACK_OK:    // our data is large, we require a few added steps
        {
            // a structure containing the size of the packet data that soon arrives is sent to
            //  pc from our device, let's parse the duplicated, God knows why size...
            u32 l = *((u32*)(rcv.payload.data + 1));
            if (!Data_Ready(machine_num, l))
            {
                // at this point machine should respond with CMD_DATA and 
                //  the logs, followed by CMD_ACK_OK to terminate transmission
                FREE_BUF(rcv.payload.data);
                if (Get_Response(machine_num, rq[machine_num].reply_num, rcv) < 0)
                    return -1;
                
                if (RNTOHS(rcv.payload.command_id) == CMD_DATA)
                {
                    u32 len = *((u32*)rcv.payload.data) / sizeof(Attendance_Entry);
                    User_Entry_Ptr pusr = (User_Entry_Ptr)(rcv.payload.data + 4);
                    u32 i = 0;
                    while (i++ < len) 
                    {
                        User_Entry user = *pusr;
                        users.push_back(user);
                        pusr = (User_Entry_Ptr)((rcv.payload.data + 4) + (i * sizeof(User_Entry)));
                    } // end while
                } // end if Deja vu

                FREE_BUF(rcv.payload.data);
                if (Get_Response(machine_num, rq[machine_num].reply_num, rcv) < 0)
                    return -1;

                RSP_OK(rcv.payload.command_id, machine_num);
                if (Refresh(machine_num, CMD_FREE_DATA) < 0)
                    return -1;
            } // end if
        } break;

        default:
            rq[machine_num].err = "Device returned error code: " + std::to_string(RNTOHS(rcv.payload.command_id));
            return -2;
    } // end switch

    FREE_BUF(rcv.payload.data);
    return Enable_Device(machine_num);
} // end Read_All_UserIDs


//==============================================================================================================|
/**
 * @brief 
 *  Processes data for longer messages
 * 
 * @param [machine_num] the machine identifier
 * @param [dlen] the expected length of data
 * 
 * @return int 
 *  a 0 on success alas -ve on fail
 */
int Data_Ready(const int machine_num, const u32 dlen)
{
    struct rdy_struct 
    {
        u32 pad{0};
        u32 len{0};
    }; // end struct

    rdy_struct rdy{0, dlen};
    Zkt_Packet snd, rcv;
    u16 rpnum = rq[machine_num].reply_num;
    snd.payload.data = (u8*)&rdy;
    ACT(machine_num, snd, rcv, CMD_DATA_RDY, rq[machine_num].session_id, rpnum,
        Checksum(&snd.payload, (u16*)&rdy, sizeof(rdy) >> 1), sizeof(rdy));

    if (RNTOHS(rcv.payload.command_id) != CMD_PREPARE_DATA)
    {
        rq[machine_num].err = "Device not ready to send data, returned: " + std::to_string(RNTOHS(rcv.payload.command_id));
        FREE_BUF(rcv.payload.data);
        return -2;
    } // end if

    FREE_BUF(rcv.payload.data);
    return 0;  // success
} // end Data_Ready


//==============================================================================================================|
/**
 * @brief 
 *  Returns the entire shebang of attendance record stored in the machine since time
 * 
 * @param [machine_num] the machine identifier
 * @param [entry] a vector of attendance entries
 *  
 * @return int 
 *  a success 0 or -ve on fail
 */
int Read_Attendance_Record(const int machine_num, std::vector<Attendance_Entry> &entry)
{
    // this a copy pasted version of the Read_User_Info (real programmers plz don't kill me!)
    Zkt_Packet snd, rcv;

    // the meaining of these values have not yet been deciphered ...
    u8 dat[11]{0x01, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    Disable_Device(machine_num);
    u16 rnum = rq[machine_num].reply_num;

    snd.payload.data = dat;
    ACT(machine_num, snd, rcv, CMD_DATA_WRRQ, rq[machine_num].session_id, 
        rnum, Checksum(&snd.payload, (u16*)&dat, 5), 11);
    
    ++rq[machine_num].reply_num;
    switch (RHTONS(rcv.payload.command_id)) 
    {
        case CMD_DATA:      // data has been appeneded to this response
        {
            // the first four bytes of data tell us the length of the entire payload
            //  junk in bytes, thus let's take in that info and convert it to counts
            //  by dividing with a user_entry struct size.
            
            u32 len = *((u32*)rcv.payload.data) / sizeof(Attendance_Entry);
            Attendance_Entry_Ptr patt = (Attendance_Entry_Ptr)(rcv.payload.data + 4);
            u32 i = 0;
            while (i++ < len) 
            {
                Attendance_Entry att = *patt;
                entry.push_back(att);
                patt = (Attendance_Entry_Ptr)((rcv.payload.data + 4) + (i * sizeof(Attendance_Entry)));
            } // end while
        } break;

        case CMD_ACK_OK:    // our data is large, we require a few added steps
        {
            // a structure containing the size of the packet data that soon arrives is sent to
            //  pc from our device, let's parse the duplicated, God knows why size...
            u32 l = *((u32*)(rcv.payload.data + 1));
            if (!Data_Ready(machine_num, l))
            {
                // at this point machine should respond with CMD_DATA and 
                //  the logs, followed by CMD_ACK_OK to terminate transmission
                FREE_BUF(rcv.payload.data);
                if (Get_Response(machine_num, rq[machine_num].reply_num, rcv) < 0)
                    return -1;
                
                if (RNTOHS(rcv.payload.command_id) == CMD_DATA)
                {
                    u32 len = *((u32*)rcv.payload.data) / sizeof(Attendance_Entry);
                    Attendance_Entry_Ptr patt = (Attendance_Entry_Ptr)(rcv.payload.data + 4);
                    u32 i = 0;
                    while (i++ < len) 
                    {
                        if (patt->att_time == 0)
                            continue;

                        Attendance_Entry att = *patt;
                        entry.push_back(att);
                        patt = (Attendance_Entry_Ptr)((rcv.payload.data + 4) + (i * sizeof(Attendance_Entry)));

                    } // end while
                } // end if Deja vu

                FREE_BUF(rcv.payload.data);
                if (Get_Response(machine_num, rq[machine_num].reply_num, rcv) < 0)
                    return -1;

                RSP_OK(rcv.payload.command_id, machine_num);
                if (Refresh(machine_num, CMD_FREE_DATA) < 0)
                    return -1;
            } // end if
        } break;

        default:
            rq[machine_num].err = "Device returned error code: " + std::to_string(RNTOHS(rcv.payload.command_id));
            return -2;
    } // end switch

    FREE_BUF(rcv.payload.data);
    return Enable_Device(machine_num);
} // end Read_Attendance_Record


//==============================================================================================================|
/**
 * @brief 
 *  Deletes user info stored on device
 * 
 * @param [machine_num] the machine identifier
 * @param [user_sn] the user serial number as stored on the machine
 *  
 * @return int 
 *  0 on success alas a fail -1.
 */
int Delete_User(const int machine_num, const u16 user_sn)
{
    ACT_INDATA(machine_num, CMD_DELETE_USER, &user_sn, sizeof(user_sn));  
    if (Refresh(machine_num) < 0)
        return -1;

    return 0;
} // end Delete_User


//==============================================================================================================|
/**
 * @brief 
 *  Register's the system for reception of real time signals that originate from the device; right now I'm only
 *  interested in the attendance transaction.
 * 
 * @param [machine_num] machine descriptor 
 * @param [options] regstration options default we listen only AttendanceTransaction 0x0000ffff}; 
 *      0x0000ffff = all, 1 = Attendance transaction
 * 
 * @return int 
 *  0 on success -1 on fail
 */
int Init_Realtime(const int machine_num, const u32 options)
{  
    ACT_INDATA(machine_num, CMD_REG_EVENT, &options, sizeof(options));
    return 0;
} // end Start_Realtime


//==============================================================================================================|
/**
 * @brief 
 *  Set's the user info; if user exists updates the existing one by overwritting; if not it creates new
 * 
 * @param [machine_num] the machine identifier
 * @param [puser] a user info struct
 * 
 * @return int 
 *  a 0 on success or -1 on fail
 */
int Set_User_Info(const int machine_num, User_Entry_Ptr puser)
{
    if (Disable_Device(machine_num) < 0)
        return -1;

    ACT_INDATA(machine_num, CMD_USER_WRQ, puser, sizeof(User_Entry));

    if (Refresh(machine_num) < 0)
        return -1;

    return Enable_Device(machine_num);
} // end Set_User_Info


//==============================================================================================================|
/**
 * @brief 
 *  Restarts the device, no response is retured; must reconnect again afterwards
 * 
 * @param [machine_num] the machine number to enable
 *  
 * @return int 
 *  0 on success alas -ve on fail
 */
int Restart_Device(const int machine_num)
{
    ACT_NODATA(machine_num, CMD_RESTART);
    Disconnect_Net(machine_num);

    // wait a little longer and reconnect back

    return 0;
} // end Enable_Device


//==============================================================================================================|
/**
 * @brief 
 *  Enrolls a new user.
 * 
 * @param [machine_num] the machine number to enable
 *  
 * @return int 
 *  0 on success alas -ve on fail
 */
int Enroll_User(const int machine_num, const Enroll_Data &enroll)
{
    std::string query[]{{"~PIN2Width\x00"}, {"~IsABCPinEnable\x00"}};
    std::string result{""};

    if (Read_Machine_Config(machine_num, query[0], result) < 0)
        return -1;

    // extract out the width; we should have known by now, because we've already
    //  passed it to the function.

    // test if our user id contains alphanumeric keys and if the device supports it!
    if (Alphanumeric_Support((char*)enroll.user_id))
    {
        int ret = Read_Machine_Config(machine_num, query[1], result);
        if (ret == -1)
            return -1;
        else if (ret == -2) {
            rq[machine_num].err = "Device does not support alphanumeric keys. Digits only allowed";
        } // end else if
    } // end if

    if (Cancel_Operation(machine_num) < 0)
        return -1;

    // begin the enrollment procedure
    ACT_INDATA(machine_num, CMD_STARTENROLL, &enroll, sizeof(enroll));

    // begin verification get real time signals
    return Start_Identify(machine_num);
} // end Enroll_User


//==============================================================================================================|
/**
 * @brief 
 *  Sends a message to enable the device; returns device to normal state
 * 
 * @param [machine_num] the machine number to enable
 *  
 * @return int 
 *  0 on success alas -ve on fail
 */
int Enable_Device(const int machine_num)
{
    ACT_NODATA(machine_num, CMD_ENABLE_DEVICE);
    return 0;
} // end Enable_Device


//==============================================================================================================|
/**
 * @brief 
 *  Sends a message to disable the device;
 * 
 * @param [machine_num] the machine number to enable
 *  
 * @return int 
 *  0 on success alas -ve on fail
 */
int Disable_Device(const int machine_num)
{
    ACT_NODATA(machine_num, CMD_DISABLE_DEVICE);
    return 0;
} // end Enable_Device


//==============================================================================================================|
/**
 * @brief 
 *  the procedure clears all admin previlages and sets all users to "common users"
 * 
 * @param [machine_num] identifier to the machine
 *  
 * @return int 
 *  a 0 on success alas -ve
 */
int Clear_Admins(const int machine_num)
{
    ACT_NODATA(machine_num, CMD_CLEAR_ADMIN);
    return 0;
} // end Clear_Admins


//==============================================================================================================|
/**
 * @brief 
 *  Enables/Disables the clock dots on the device screen
 * 
 * @param [machine_num] machine identifer
 * 
 * @return int 
 *  0 on success, -ve on fail
 */
int Enable_Clock(const int machine_num)
{
    ACT_NODATA(machine_num, CMD_ENABLE_CLOCK);
    return 0;
} // end Enable_Clock


//==============================================================================================================|
/**
 * @brief 
 *  put the machine at a comparision state
 * 
 * @param [machine_num] machine identifer
 * 
 * @return int 
 *  0 on success, -ve on fail
 */
int Start_Identify(const int machine_num)
{
    ACT_NODATA(machine_num, CMD_STARTVERIFY);
    return 0;
} // end Start_Identify


//==============================================================================================================|
/**
 * @brief 
 *  Disable normal verification of the machine
 * 
 * @param [machine_num] machine identifer
 * 
 * @return int 
 *  0 on success, -ve on fail
 */
int Cancel_Operation(const int machine_num)
{
    ACT_NODATA(machine_num, CMD_CANCELCAPTURE);
    return 0;
} // end Cancel_Operation


//==============================================================================================================|
/**
 * @brief 
 *  Turns of the device. When started must be reconnected.
 * 
 * @param [machine_num] machine identifer
 * 
 * @return int 
 *  0 on success, -ve on fail
 */
int Power_Off(const int machine_num)
{
    ACT_NODATA(machine_num, CMD_POWEROFF);
    Disconnect_Net(machine_num);
    return 0;
} // end Power_Off


//==============================================================================================================|
/**
 * @brief 
 *  Reads the configuration info passed into query.
 * 
 * @param [machine_num] the machine identifer; driver specific only
 * @param [query] the string value to read info for such as ~PIN2Width -- which tells the length for user_id 
 * @param [result] the result of query as a data string; encoded in specific to caller
 *  
 * @return int 
 *  a 0 on success, -1 on fail, -2 if query not supported
 */
int Read_Machine_Config(const int machine_num, const std::string &query, std::string &result)
{
    Zkt_Packet snd, rcv;
    u16 rnum = rq[machine_num].reply_num;

    ACT(machine_num, snd, rcv, CMD_OPTIONS_RRQ, rq[machine_num].session_id, 
        rnum, Checksum(&snd.payload, (u16*)query.c_str(), query.length() / 2), query.length());

    ++rq[machine_num].reply_num;
    if (rcv.payload.command_id == CMD_ACK_ERROR)
        return -2;      // whatever is requested not supported

    RSP_OK(rcv.payload.command_id, machine_num);

    // copy the databack
    result = (char*)rcv.payload.data;
    if (rcv.payload.data)
    {
        free(rcv.payload.data);
        rcv.payload.data = nullptr;
    } // end if

    return 0;
} // end Read_Machine_Config



//==============================================================================================================|
/**
 * @brief 
 *  The checksum computes everything by splitting the packet into 16-bit words and accumulating the result 32-bit,
 *  it then adds the HO word of the 32-bit value on LO word, and finally it computes the ones compliment to arrive
 *  at the checksum value.
 * 
 * NOTE:
 *  ZKT eco device does not really do according to its specs, i.e. it won't send checksum error for an incorrect
 *  checksum (or close the connection), it instead keeps silent making the recv call hang forever. In some unforseen
 *  chain of events it might even cause the device to become unresponsive for any other request and connection --
 *  in such instances restart the device -- manully.
 * 
 * @param [ppload] pointer to the payload structure
 * @param [data] the data feild
 * @param [data_len] length of the variable feild data (if present)
 */
inline u16 Checksum(Payload_Ptr ppload, u16 *data, u32 data_len)
{
    u32 result = ppload->command_id + ppload->reply_number + ppload->session_id;
    while (data_len--) 
        result += *data++;

    // sweet, now add HO word on the LO word
    u16 checksum = (u16)(((result >> 16) & 0x0000FFFF) + (result & 0x0000FFFF));
    return checksum ^ 0xFFFF;
} // end Compute_Checksum


//==============================================================================================================|
/**
 * @brief 
 *  the hashing algorithim used by ZKTeco devices during authorized connection; this hash is dependant on the
 *  parameters passed and works by XORing each against known and/or pre-cooked values.
 * 
 * @param [session_id] the session id returned from device during CMD_CONNECT
 * @param [key] a password for the machine; must be an integer of size 32-bit (the hash algo depends on that)
 * @param [ticks] defaults to 50 -- this must be like a seed or salt value
 * 
 * @return u32 
 *  the hashed value as 32-bit descriptor
 */
inline u32 Commkey(const u16 session_id, const u32 password, const u8 ticks)
{
    // magic number used in ZKTeco command authorization hashing; 
    const static u32 magic_num{RHTONL(0x4F534B5A)}; 
    u32 t32 = DUP32BIT(ticks);
    u32 k = 0;

    /* hash the password */

    k = (k + session_id) ^ magic_num;
    k = (SWAP_WORDS(k)) ^ t32; 

    char *ptr = (char *)&k;     // a useful hack ...
    ptr[2] = ticks;

    return k;
} // end Commkey


//==============================================================================================================|
/**
 * @brief 
 *  determines if the string contains alphanumeric keys 
 * 
 * @param [str] string to be tested
 * 
 * @return bool
 *  a true if string contain alphanumeric alas false
 */
inline bool Alphanumeric_Support(const std::string &str)
{
    for (auto &s : str)
    {
        if (s < 0x30 && s > 0x39)
            return true;        // alphanumeric
    } // end for

    return false;
} // end Alphanumeric_Support


//==============================================================================================================|
/**
 * @brief 
 *  Dumps the contents of user info to the console
 * 
 * @param info 
 *  the user info structure to print
 */
void Print_User_Info(User_Entry &info)
{
    std::cout << "Serial Number: " << info.serial_number 
        << "\nPermissions: " << info.permissions
        << "\nPassword: " << info.password
        << "\nName: " << info.name
        << "\nCard Number: " << info.card_number
        << "\nGroup Number: " << info.group_number
        << "\nUser Timezone: " << info.user_tzs
        << "\nTime Zone1: " << info.tz1
        << "\nTime Zone2: " << info.tz2
        << "\nTime Zone3: " << info.tz3
        << "\nUser Id: " << info.user_id
        << "\n---------------------------------" << std::endl; 
} // end Print_User_Info


//==============================================================================================================|
/**
 * @brief 
 *  Dumps the contents of Attendance record struct
 * 
 * @param info 
 *  the user info structure to print
 */
void Print_Att_Info(Attendance_Entry &info)
{
    u32 s, m, h, d, mn, yy;
    DECODE_DATE(info.att_time, s, m, h, d, mn, yy);
    std::cout << "User Serial Number: " << info.serial_number
        << "\nUser Id: " << info.user_id
        << "\nVerify type: " << info.verify_type
        << "\nRecord time: " << mn << "/" << d << "/" << yy << " " << h << ":" << m << ":" << s
        << "\nVerify state: " << info.verify_state
        << "\n---------------------------------" << std::endl; 
} // end Print_User_Info


//==============================================================================================================|
//          THE END
//==============================================================================================================|