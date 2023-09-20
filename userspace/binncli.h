#ifndef __binncli_h
#define __binncli_h

#include "/usr/local/include/binn.h"

#ifndef HPNSSH_QFACTOR_BINN
#define HPNSSH_MSG      2

//For HPNSSH_MSGs operations
#define HPNSSH_READALL  44
#define HPNSSH_SHUTDOWN 55
#define HPNSSH_START    99

struct ServerBinnMsg {
	unsigned int msg_type;
	unsigned int op;
	unsigned int hop_latency;
	unsigned int queue_occupancy;
	unsigned int switch_id;
	char *timestamp;
};
#endif

struct ClientBinnMsg {
	unsigned int msg_type;
	unsigned int op;
};
#endif //__binncli_h
