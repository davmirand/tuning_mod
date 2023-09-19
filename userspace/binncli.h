#ifndef __binncli_h
#define __binncli_h
//Added for Q-Factor
#define CTIME_BUF_LEN           27
#define MS_CTIME_BUF_LEN        48

#if 1
#define HPNSSH_QFACTOR  1
#if 1
#define HPNSSH_QFACTOR_BINN  1
#endif
#include "/usr/local/include/binn.h"
#endif

#define TEST_MSG        0
#define QINFO_MSG       1
#define HPNSSH_MSG      2

//For HPNSSH_MSGs value will be whether to do a read or readall or shutdown
#define HPNSSH_READ     33
#define HPNSSH_READALL  44
#define HPNSSH_SHUTDOWN 55
#define HPNSSH_START    99
#define HPNSSH_DUMMY    166

struct PeerMsg {
        unsigned int msg_no;
        unsigned int seq_no;
        unsigned int value;
        unsigned int hop_latency;
        unsigned int queue_occupancy;
        unsigned int switch_id;
        char timestamp[MS_CTIME_BUF_LEN];
        char msg[80];
        char * pts;
        char * ptimes;
        char * pm;
};

struct ClientBinnMsg {
        unsigned int msg_no;
        unsigned int value;
};

struct ServerBinnMsg {
        unsigned int msg_no;
        unsigned int value;
        unsigned int hop_latency;
        unsigned int queue_occupancy;
        unsigned int switch_id;
        char timestamp[MS_CTIME_BUF_LEN];
};
#endif //__binncli_h
