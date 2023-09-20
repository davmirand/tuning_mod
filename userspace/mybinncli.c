#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <bits/stdint-uintn.h>
#include <bits/types.h>
#include <ctype.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <time.h>

#define CTIME_BUF_LEN           27
#define MS_CTIME_BUF_LEN        48
#include "binncli.h"

#if 1
/* Following shortens all the typecasts of pointer arguments: */
#define SA      struct sockaddr

/* prototypes for our socket wrapper functions: see {Sec errors} */
int      Accept(int, SA *, socklen_t *);
void     Bind(int, const SA *, socklen_t);
int      Connect(int, const SA *, socklen_t);
void     Listen(int, int);
int      Socket(int, int, int);
int      Writen(int, void *, size_t);

/* include Socket */
int
Socket(int family, int type, int protocol)
{
        int             n;

        if ( (n = socket(family, type, protocol)) < 0)
	{
                printf("socket error\n");
		exit(2);
	}
        return(n);
}
/* end Socket */


ssize_t                                         /* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
        size_t          nleft;
        ssize_t         nwritten;
        const char      *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
                        if (nwritten < 0 && errno == EINTR)
                        {
                                nwritten = 0;           /* and call write() again */
                        }
                        else
                                return(-1);                     /* error */
                }

                nleft -= nwritten;
                ptr   += nwritten;
        }

        return(n);
}
/* end writen */


int
Writen(int fd, void *ptr, size_t nbytes)
{
        if (writen(fd, ptr, nbytes) != nbytes)
        {
                if (errno == EPIPE)
                        return(1);
                else
		{
                        printf("writen error\n");
			return 1;
		}
        }

        return 0;
}

void
Inet_pton(int family, const char *strptr, void *addrptr)
{
        int             n;

        if ( (n = inet_pton(family, strptr, addrptr)) < 0)
                printf("inet_pton error for %s", strptr);      /* errno set */
        else if (n == 0)
	{
                printf("inet_pton error for %s", strptr);     /* errno not set */
		exit(1);
	}

        /* nothing to return */
}

void
Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
        if (bind(fd, sa, salen) < 0)
                printf("bind error\n");
}

int
Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
        if (connect(fd, sa, salen) < 0)
	{
                printf("connect error\n");
		return 1;
	}
	return 0;
}

void
Close(int fd)
{
        if (close(fd) == -1)
                printf("close error\n");
}
#endif


int vPort = 5525; //default listening port
int vShutdown = 0;

enum work_phases {
        STARTING,
	RUNNING,
        SHUTDOWN
};

enum work_phases current_phase = STARTING;

#define NUM_NAMES_MAX 3
const char *workflow_names[NUM_NAMES_MAX] = {
	"STARTING", "RUNNING", "SHUTDOWN"
};

FILE * pHpnClientLogPtr = 0;

const char *phase2str(enum work_phases phase)
{
	if (phase < NUM_NAMES_MAX)
		return workflow_names[phase];
return NULL;
}

void fMake_Binn_Client_Object(struct ClientBinnMsg *pMsg, binn * obj)
{
	binn_object_set_uint32(obj, "msg_type", pMsg->msg_type);
	binn_object_set_uint32(obj, "op", pMsg->op);
return;
}

void fRead_Binn_Object(struct PeerMsg *pMsg, binn * obj)
{

	pMsg->msg_no = binn_object_uint32(obj, "msg_no");
	pMsg->seq_no = binn_object_uint32(obj, "seq_no");
	pMsg->value = binn_object_uint32(obj, "value");
	pMsg->hop_latency = binn_object_uint32(obj, "hop_latency");
	pMsg->queue_occupancy = binn_object_uint32(obj, "queue_occupancy");
	pMsg->switch_id = binn_object_uint32(obj, "switch_id");
	pMsg->ptimes = binn_object_str(obj, "timestamp");
	pMsg->pm = binn_object_str(obj, "msg");

	return;
}




int vDebugLevel = 1;


void gettime(time_t *clk, char *ctime_buf)
{
	*clk = time(NULL);
	ctime_r(clk,ctime_buf);
	ctime_buf[24] = ':';
}
void gettimeWithMilli(time_t *clk, char *ctime_buf, char *ms_ctime_buf)
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	*clk = ts.tv_sec;
	struct tm *t = localtime(clk);
	ctime_r(clk,ctime_buf);
	ctime_buf[19] = '.';
	ctime_buf[20] = '\0';
	sprintf(ms_ctime_buf,"%s%03ld %04d:", ctime_buf, ts.tv_nsec/1000000, t->tm_year+1900);
return;
}


void sig_int_handler(int signum, siginfo_t *info, void *ptr)
{
	fprintf(pHpnClientLogPtr,"Caught SIGINT, exiting...\n");
	fflush(pHpnClientLogPtr);
	printf("Caught SIGINT, Shutting down client and exiting...\n");

	vShutdown = 1;
}

void catch_sigint()
{
	static struct sigaction _sigact;

	memset(&_sigact, 0, sizeof(_sigact));
	_sigact.sa_sigaction = sig_int_handler;
	_sigact.sa_flags = SA_SIGINFO;

	sigaction(SIGINT, &_sigact, NULL);
}

#define DEBUGLEVELMAX	3
void sig_usr1_handler(int signum, siginfo_t *info, void *ptr)
{
	fprintf(pHpnClientLogPtr,"Caught SIGUSR1...\n");
	fprintf(pHpnClientLogPtr,"Debug Level is currently %d...\n",vDebugLevel);
	if (vDebugLevel < DEBUGLEVELMAX)
		vDebugLevel++;
	else
		vDebugLevel = 0;

	fprintf(pHpnClientLogPtr,"Debug Level is now %d...\n",vDebugLevel);
	fflush(pHpnClientLogPtr);
return;
}

void catch_sigusr1()
{
	static struct sigaction _sigact;

	memset(&_sigact, 0, sizeof(_sigact));
	_sigact.sa_sigaction = sig_usr1_handler;
	_sigact.sa_flags = SA_SIGINFO;

	sigaction(SIGUSR1, &_sigact, NULL);
}


ssize_t                                         /* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n)
{
	size_t  nleft;
	ssize_t nread;
	char    *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) 
	{
		if ( (nread = read(fd, ptr, nleft)) < 0) 
		{
			if (errno == EINTR)
			{ 	if (vShutdown) //shutdown requested
					return (-1);
				else
                            		nread = 0;              /* and call read() again */
			}
		else
			return(-1);
		} 
		else 
			if (nread == 0)
				break;	/* EOF */

		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);              /* return >= 0 */
}
/* end readn */


ssize_t
Readn(int fd, void *ptr, size_t nbytes)
{
	ssize_t         n;

	if ( (n = readn(fd, ptr, nbytes)) < 0)
	{
		if (vShutdown)
			fprintf(pHpnClientLogPtr,"***INFO*** WARNING!!!!: Shutdown of Client requested*****\n");
		else	
			printf("readn error\n");
	}
return(n);
}

void fDoHpnReadAllFS(unsigned int val, int sockfd, struct PeerMsg *from_server);
void fDoHpnShutdownFS(unsigned int val, int sockfd);
void fDoHpnStartFS(unsigned int val, int sockfd);
void fDoHpnTimeoutFS(unsigned int val, int sockfd, struct PeerMsg *from_server);
void fDoHpnFromserver(unsigned int val, int sockfd, struct PeerMsg *from_server);


void fDoHpnTimeoutFS(unsigned int val, int sockfd, struct PeerMsg *from_server)
{
	time_t clk;
	char ctime_buf[27];
	char ms_ctime_buf[MS_CTIME_BUF_LEN];

	if (vDebugLevel > 1)
	{
		gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);
		fprintf(pHpnClientLogPtr,"%s %s: ***INFO***: In fDoHpnTimeoutFS(), value is %u***\n", ms_ctime_buf, phase2str(current_phase), val);
	}

	if (vDebugLevel > 2)
	{
		fprintf(pHpnClientLogPtr, "\n%s %s: ***********************HPN_CLIENT_TIMEOUT************************",
								from_server->ptimes, phase2str(current_phase));
		fprintf(pHpnClientLogPtr, "\n%s %s: HPN_CLIENT    : hop_switch_id = %u\n",
								from_server->ptimes, phase2str(current_phase), ntohl(from_server->switch_id));
		fprintf(pHpnClientLogPtr, "%s %s: HPN_CLIENT    : queue_occupancy = %u\n",
								from_server->ptimes, phase2str(current_phase), ntohl(from_server->queue_occupancy));
		fprintf(pHpnClientLogPtr, "%s %s: HPN_CLIENT    : hop_latency = %u\n",
								from_server->ptimes, phase2str(current_phase), ntohl(from_server->hop_latency));
	}
return;
}

void fDoHpnReadAllFS(unsigned int val, int sockfd, struct PeerMsg *from_server)
{
	time_t clk;
	char ctime_buf[27];
	char ms_ctime_buf[MS_CTIME_BUF_LEN];

	if (vDebugLevel > 1)
	{
		gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);
		fprintf(pHpnClientLogPtr,"\n%s %s: ***INFO***: In fDoHpnReadAllFS(), value is %u***", ms_ctime_buf, phase2str(current_phase), val);
	}

	if (vDebugLevel > 0)
	{
		fprintf(pHpnClientLogPtr, "\n%s %s: ***********************HPN_CLIENT************************",
								from_server->ptimes, phase2str(current_phase));
		fprintf(pHpnClientLogPtr, "\n%s %s: HPN_CLIENT    : hop_switch_id = %u\n",
								from_server->ptimes, phase2str(current_phase), ntohl(from_server->switch_id));
		fprintf(pHpnClientLogPtr, "%s %s: HPN_CLIENT    : queue_occupancy = %u\n",
								from_server->ptimes, phase2str(current_phase), ntohl(from_server->queue_occupancy));
		fprintf(pHpnClientLogPtr, "%s %s: HPN_CLIENT    : hop_latency = %u\n",
								from_server->ptimes, phase2str(current_phase), ntohl(from_server->hop_latency));
	}
return;
}

void fDoHpnFromServer(unsigned int val, int sockfd, struct PeerMsg *from_server)
{
	time_t clk;
	char ctime_buf[27];
	char ms_ctime_buf[MS_CTIME_BUF_LEN];

	switch (val) {
		case 144:
			fDoHpnReadAllFS(val, sockfd, from_server);
			break;
		case 199:
			//fDoHpnStartFS(val, sockfd);
			break;
		case  166:
			fDoHpnTimeoutFS(val, sockfd, from_server);
			break;
		default:
			gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);
			fprintf(pHpnClientLogPtr,"%s %s: ***WARNING***: Invalid Hpnmessage value from client. Value is %u***\n", ms_ctime_buf, phase2str(current_phase), val);
			break;
	}

return;
}

void process_request(int sockfd)
{
	ssize_t n;
	struct PeerMsg sMsg;
	char from_cli[204];;
	time_t clk;
	char ctime_buf[27];
	char ms_ctime_buf[MS_CTIME_BUF_LEN];

	for ( ; ; )
	{
		if ( (n = Readn(sockfd, from_cli, sizeof(from_cli))) == 0)
		{
			if (vDebugLevel > 0)
			{
				fprintf(pHpnClientLogPtr,"\n%s %s: ***Connection 1 closed***\n", ms_ctime_buf, phase2str(current_phase));
				fflush(pHpnClientLogPtr);
			}

			return;         /* connection closed by other end */
		}
		else
			{
				if (vShutdown)
					return;
			}
#if 0
		fprintf(pHpnClientLogPtr,"\n%s %s: ***number of bytes to read is  %lu ***\n", ms_ctime_buf, phase2str(current_phase),sizeof(from_cli));
		fprintf(pHpnClientLogPtr,"\n%s %s: ***%lu bytes read ***\n", ms_ctime_buf, phase2str(current_phase),n);
#endif
		fflush(pHpnClientLogPtr);
		fRead_Binn_Object(&sMsg, (binn *)&from_cli);

		gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);
		//if (ntohl(from_cli.msg_no) == HPNSSH_MSG)
		if (ntohl(sMsg.msg_no) == HPNSSH_MSG)
		{
			if (vDebugLevel > 2)
                	{
                		fprintf(pHpnClientLogPtr,"\n%s %s: ***Received Hpnssh message %u from Hpnssh Server...***\n", 
										ms_ctime_buf, phase2str(current_phase), ntohl(sMsg.seq_no));
                       		 fprintf(pHpnClientLogPtr,"%s %s: ***msg_no = %d, msg value = %u, msg buf = %s", 
						ms_ctime_buf, phase2str(current_phase), ntohl(sMsg.msg_no), ntohl(sMsg.value), sMsg.pm);
			}
		
			fDoHpnFromServer(ntohl(sMsg.value), sockfd, &sMsg);
		}
		else
			{
				fprintf(pHpnClientLogPtr,"\n%s %s: ***Received Invalid message %u from Hpnssh Server...***\n", 
									ms_ctime_buf, phase2str(current_phase), ntohl(sMsg.seq_no));
				fprintf(pHpnClientLogPtr,"%s %s: ***msg_no = %d, msg buf = %s", 
								ms_ctime_buf, phase2str(current_phase), ntohl(sMsg.msg_no), sMsg.pm);
			}
		
		fflush(pHpnClientLogPtr);
	}
}

int str_cli(int sockfd, struct ClientBinnMsg *sThisMsg) //str_cli09
{
	binn *myobj = binn_object();
	fMake_Binn_Client_Object(sThisMsg, myobj);
#if 0
	fprintf(pHpnClientLogPtr,"***!!!!!!!Size of binn object = %u...***\n", binn_size(myobj));
	fflush(pHpnClientLogPtr);
#endif
	Writen(sockfd, binn_ptr(myobj), binn_size(myobj));
	binn_free(myobj);

	return 0;
}

int main(int argc, char *argv[])
{
	time_t clk;
	char ctime_buf[27];
	char ms_ctime_buf[MS_CTIME_BUF_LEN];
	int sockfd;
	struct sockaddr_in servaddr;
	struct ClientBinnMsg cliHpnBinnMsg;
	char aMySrc_Ip[32];
	char aLogFile[256];

	//mypid = getpid();
	//sprintf(aLogFile,"/tmp/hpnClientLog.%u",mypid);
	sprintf(aLogFile,"/tmp/hpnClientLog");
	gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);

	pHpnClientLogPtr = fopen(aLogFile,"w");
	if (!pHpnClientLogPtr)
	{
		printf("%s %s: ***Couldn't open HPN Client log for testing, exiting...\n", ms_ctime_buf, phase2str(current_phase));
		exit(1);
	}

	fprintf(pHpnClientLogPtr,"%s %s: ***Starting Client for testing with HPN-QFACTOR server...***\n", ms_ctime_buf, phase2str(current_phase));
	sprintf(aMySrc_Ip,"127.0.0.1");
	if (argc > 1 && argc < 6) //5 args max
	{
		if (argc == 3 && ((strcmp(argv[1],"-p") == 0) || 
				  (strcmp(argv[1],"-d") == 0)))
		{
			if (strcmp(argv[1],"-d") == 0)
				sprintf(aMySrc_Ip, argv[2]);
			if (strcmp(argv[1],"-p") == 0)
				vPort = atoi(argv[2]);
		}
		else
			if (argc == 5 && (((strcmp(argv[3],"-p") == 0) || (strcmp(argv[3],"-d") == 0)) || 
					  ((strcmp(argv[3],"-d") == 0) || (strcmp(argv[3],"-p") == 0))))
			{

				//do nothing yet
			}
	}

	fprintf(pHpnClientLogPtr,"%s %s: ***Connecting to HPNSSN-QFACTOR Server %s, using port %d...***\n", 
							ms_ctime_buf, phase2str(current_phase), aMySrc_Ip, vPort);
	fflush(pHpnClientLogPtr);

	catch_sigint();
	catch_sigusr1();

	memset(&cliHpnBinnMsg,0,sizeof(cliHpnBinnMsg));
	cliHpnBinnMsg.op  = HPNSSH_START;

cli_again:
	gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);

	cliHpnBinnMsg.msg_type = htonl(HPNSSH_MSG);

	switch (cliHpnBinnMsg.op) {
		case  HPNSSH_START: //connect
			if (vDebugLevel > 1)
			{
				fprintf(pHpnClientLogPtr,"%s %s: ***Connecting to HPNSSN_QFACTOR server...***\n", ms_ctime_buf, phase2str(current_phase));
				fflush(pHpnClientLogPtr);
			}

			cliHpnBinnMsg.op = htonl(cliHpnBinnMsg.op);
		
			sockfd = Socket(AF_INET, SOCK_STREAM, 0);
			bzero(&servaddr, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons(vPort);

			Inet_pton(AF_INET, aMySrc_Ip, &servaddr.sin_addr);

			if (Connect(sockfd, (SA *) &servaddr, sizeof(servaddr)))
			{
				goto cli_again;
			}

			cliHpnBinnMsg.op = HPNSSH_READALL; //next state
			current_phase = RUNNING;
			goto cli_again;

			/* never reached */	
			break;

		case HPNSSH_SHUTDOWN:
			fprintf(pHpnClientLogPtr,"%s %s: ***Shutting down this client...***\n", ms_ctime_buf, phase2str(current_phase));
			fflush(pHpnClientLogPtr);
			Close(sockfd);
			exit(0);
			break;
	
		case HPNSSH_READALL:		
			if (vDebugLevel > 1)
			{
				fprintf(pHpnClientLogPtr,"%s %s: ***Sending READALL message to HPNSSN_QFACTOR server...***\n", ms_ctime_buf, phase2str(current_phase));
				fflush(pHpnClientLogPtr);
			}

			cliHpnBinnMsg.op = htonl(cliHpnBinnMsg.op);
			str_cli(sockfd, &cliHpnBinnMsg);        
				
			if (vDebugLevel > 1)
			{
				fprintf(pHpnClientLogPtr,"%s %s: ***Finished sending READALL message to HPNSSN_QFACTOR server...***\n", ms_ctime_buf, phase2str(current_phase));
				fflush(pHpnClientLogPtr);
			}

			process_request(sockfd);
				
			if (vShutdown)
			{
				cliHpnBinnMsg.op = HPNSSH_SHUTDOWN;
				vShutdown = 0;
			}
			else
				cliHpnBinnMsg.op = HPNSSH_READALL;
			break;
			
			default:
				fprintf(pHpnClientLogPtr,"%s %s: ***Invalid cliHpnBinnMsg op %d...***\n", 
								ms_ctime_buf, phase2str(current_phase), cliHpnBinnMsg.op);
				fflush(pHpnClientLogPtr);
				exit(1);
				break;
	}

	goto cli_again;

return 0;
}

