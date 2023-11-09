#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/time.h>    /* timeval{} for select() */
#include <time.h>                /* timespec{} for pselect() */
#include <pthread.h>
#include <stdarg.h>              /* ANSI C header file */
#include <syslog.h>              /* for syslog() */
#include "binncli.h"

#define CTIME_BUF_LEN           27
#define MS_CTIME_BUF_LEN        48

#define WORKFLOW_NAMES_MAX      4
#define	MAXLINE		4096	/* max text line length */

typedef struct {
        int argc;
        char ** argv;
} sArgv_t;

int vDebugLevel = 1;
int vPort = 5525; //default listening port
int vShutdown = 0;
FILE * pHpnServerLogPtr = 0;

#define SA      struct sockaddr
#define	LISTENQ		1024	/* 2nd argument to listen() */
/* prototypes for socket wrapper functions */
int     Accept(int, SA *, socklen_t *);
void    Bind(int, const SA *, socklen_t);
void    Listen(int, int);
int     Socket(int, int, int);
int     Writen(int, void *, size_t);
int	err_sys(const char *, ...);

int             daemon_proc;            /* set nonzero by daemon_init() */
static void     err_doit(int, int, const char *, va_list);

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */
static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
        int             errno_save, n;
        char    buf[MAXLINE + 1];

        errno_save = errno;             /* value caller might want printed */
#ifdef  HAVE_VSNPRINTF
        vsnprintf(buf, MAXLINE, fmt, ap);       /* safe */
#else
        vsprintf(buf, fmt, ap);                                 /* not safe */
#endif
        n = strlen(buf);
        if (errnoflag)
                snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
        strcat(buf, "\n");

        if (daemon_proc) {
                syslog(level, "%s", buf);
        } else {
                fflush(stdout);         /* in case stdout and stderr are the same */
                fputs(buf, stderr);
                fflush(stderr);
        }
        return;
}

int
err_sys(const char *fmt, ...)
{
        va_list         ap;

        va_start(ap, fmt);
        err_doit(1, LOG_ERR, fmt, ap);
        va_end(ap);
//        exit(1);
return 1;
}

void Pthread_mutex_lock(pthread_mutex_t *);
void Pthread_mutex_unlock(pthread_mutex_t *);
void Pthread_cond_signal(pthread_cond_t *cptr);
void Pthread_cond_wait(pthread_cond_t *cptr, pthread_mutex_t *mptr);

void
Pthread_cond_signal(pthread_cond_t *cptr)
{
        int             n;

        if ( (n = pthread_cond_signal(cptr)) == 0)
                return;
        errno = n;
        err_sys("pthread_cond_signal error");
}

void
Pthread_cond_wait(pthread_cond_t *cptr, pthread_mutex_t *mptr)
{
        int             n;

        if ( (n = pthread_cond_wait(cptr, mptr)) == 0)
                return;
        errno = n;
        err_sys("pthread_cond_wait error");
}

/* include Pthread_mutex_lock */
void
Pthread_mutex_lock(pthread_mutex_t *mptr)
{
        int             n;

        if ( (n = pthread_mutex_lock(mptr)) == 0)
                return;
        errno = n;
        err_sys("pthread_mutex_lock error");
}
/* end Pthread_mutex_lock */

void
Pthread_mutex_unlock(pthread_mutex_t *mptr)
{
        int             n;

        if ( (n = pthread_mutex_unlock(mptr)) == 0)
                return;
        errno = n;
        err_sys("pthread_mutex_unlock error");
}

void Close(int fd)
{
        if (close(fd) == -1)
                printf("close error\n");

        return;
}

int Socket(int family, int type, int protocol)
{
        int             n;

        if ( (n = socket(family, type, protocol)) < 0)
        {
                printf("socket error\n");
                exit(2);
        }
        return(n);
}

ssize_t writen(int fd, const void *vptr, size_t n)
{
        size_t nleft;
        ssize_t nwritten;
        const char *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0)
        {
                if ( (nwritten = write(fd, ptr, nleft)) <= 0)
                {
                        if (nwritten < 0 && errno == EINTR)
                        {
                                nwritten = 0;   /* and call write() again */
                        }
                        else
                                return(-1);     /* error */
                }

                nleft -= nwritten;
                ptr   += nwritten;
        }

        return(n);
}

/* include Listen */
void
Listen(int fd, int backlog)
{
        char    *ptr;

                /*4can override 2nd argument with environment variable */
        if ( (ptr = getenv("LISTENQ")) != NULL)
                backlog = atoi(ptr);

        if (listen(fd, backlog) < 0)
                err_sys("listen error");
}
/* end Listen */

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


const char *phase2str(enum work_phases phase)
{
        if (phase < WORKFLOW_NAMES_MAX)
                return workflow_names[phase];
        return NULL;
}

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

int str_cli(int sockfd, struct PeerMsg *sThisMsg);

#define HPNSSH_QFACTOR	1
#define HPNSSH_QFACTOR_BINN 1
#define HPNSSH_MSG      2

//For HPNSSH_MSGs value will be whether to do a read or readall or shutdown
#define HPNSSH_READ             33
#define HPNSSH_READALL          44
#define HPNSSH_SHUTDOWN         55
#define HPNSSH_START            99
#define HPNSSH_READ_FS          133
#define HPNSSH_READALL_FS       144
#define HPNSSH_DUMMY            166

void fDoHpnRead(unsigned int val, int sockfd);
void fDoHpnReadAll(unsigned int val, int sockfd);
void fDoHpnShutdown(unsigned int val, int sockfd);
void fDoHpnStart(unsigned int val, int sockfd);
void fDoHpnAssessment(unsigned int val, int sockfd);

ssize_t                                         /* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n)
{
        size_t  nleft;
        ssize_t nread;
        char    *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ( (nread = read(fd, ptr, nleft)) < 0) {
                        if (errno == EINTR)
                                nread = 0;              /* and call read() again */
                        else
                                return(-1);
                } else if (nread == 0)
                        break;                          /* EOF */

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
                err_sys("readn error");
        return(n);
}

void fRead_Binn_Client_Object(struct ClientBinnMsg *pMsg, binn * obj)
{
        pMsg->msg_type = binn_object_uint32(obj, "msg_type");
        pMsg->op = binn_object_uint32(obj, "op");

        return;
}


#ifdef HPNSSH_QFACTOR
void * doProcessHpnClientReq(void * arg)
{
        ssize_t n;
#ifdef HPNSSH_QFACTOR_BINN
        struct ClientBinnMsg sMsg;
        char from_cli[BUFFER_SIZE_FROM_CLIENT];
#endif
        time_t clk;
        char ctime_buf[27];
        char ms_ctime_buf[MS_CTIME_BUF_LEN];

        int sockfd = (int)arg;

        pthread_detach(pthread_self());

        for ( ; ; )
        {
                //struct binn_struct from_cli;

#ifdef HPNSSH_QFACTOR_BINN
                if ( (n = Readn(sockfd, from_cli, sizeof(from_cli))) == 0)
#endif
                {
                        if (vDebugLevel > 0)
                        {
                                fprintf(pHpnServerLogPtr,"\n%s %s: ***Hpn Client Connection closed***\n", ms_ctime_buf, phase2str(current_phase));
                                fflush(pHpnServerLogPtr);
                        }

                        Close(sockfd);
                        return (NULL);         /* connection closed by other end */
                }

#ifdef HPNSSH_QFACTOR_BINN
#if 0
                if (vDebugLevel > 1)
                {
                        fprintf(pHpnServerLogPtr,"\n%s %s: ***num bytes read from Hpn Client = %lu***\n", ms_ctime_buf, phase2str(current_phase),n);
                        fflush(pHpnServerLogPtr);
                }
#endif
                fRead_Binn_Client_Object(&sMsg, (binn *)&from_cli);
#endif
                gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);
#ifdef HPNSSH_QFACTOR_BINN
                if (sMsg.msg_type == HPNSSH_MSG)
                {
                        if (vDebugLevel > 0)
                        {
                                fprintf(pHpnServerLogPtr,"\n%s %s: ***Received Hpnssh message from Hpnssh Client...***\n", ms_ctime_buf, phase2str(current_phase));
                                fprintf(pHpnServerLogPtr,"%s %s: ***msg type = %d, msg op = %u\n", ms_ctime_buf, phase2str(current_phase), sMsg.msg_type, sMsg.op);
                        }

                        fDoHpnAssessment(sMsg.op, sockfd);
                }
                        else
                                if (vDebugLevel > 0)
                                {
                                        fprintf(pHpnServerLogPtr,"\n%s %s: ***Received unknown message from some Hpn client???...***\n", ms_ctime_buf, phase2str(current_phase));
                                        fprintf(pHpnServerLogPtr,"%s %s: ***msg_type = %d", ms_ctime_buf, phase2str(current_phase), sMsg.msg_type);
                                }

                fflush(pHpnServerLogPtr);
        }
}
#endif



void fMake_Binn_Server_Object(struct PeerMsg *pMsg, binn * obj)
{
        binn_object_set_uint32(obj, "msg_type", pMsg->msg_no);
        binn_object_set_uint32(obj, "op", pMsg->value);
        binn_object_set_uint32(obj, "hop_latency", pMsg->hop_latency);
        binn_object_set_uint32(obj, "queue_occupancy", pMsg->queue_occupancy);
        binn_object_set_uint32(obj, "switch_id", pMsg->switch_id);
        binn_object_set_str(obj, "timestamp", pMsg->timestamp);

        return;
}

#ifdef HPNSSH_QFACTOR
pthread_mutex_t hpn_ret_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t hpn_ret_cond = PTHREAD_COND_INITIALIZER;
static int hpnretcdone = 0;
struct PeerMsg sHpnRetMsg;
struct PeerMsg sHpnRetMsg2;
unsigned int hpnRetMsgSeqNo = 0;
struct PeerMsg sTimeoutMsg;
char aDest_Ip2_Binary[32];
#endif

void fDoHpnRead(unsigned int val, int sockfd)
{
        time_t clk;
        char ctime_buf[27];
        char ms_ctime_buf[MS_CTIME_BUF_LEN];
        struct PeerMsg sRetMsg;
        int y,n;
        struct timeval tv;
        struct timespec ts;
        int saveerrno = 0;
        char mychar;
	static unsigned int count = 1111;

        gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);
        if (vDebugLevel > 6)
                fprintf(pHpnServerLogPtr,"%s %s: ***INFO***: In fDoHpnRead(), value is %u***\n", ms_ctime_buf, phase2str(current_phase), val);

#ifdef HPNSSH_QFACTOR_BINN
        //BINN objects are cross platform - no need for big endian, littl endian worries - so sayeth the binn repo
        sRetMsg.msg_no = HPNSSH_MSG;
        sRetMsg.value = HPNSSH_READ_FS;
#endif

read_again:
#if 0
        Pthread_mutex_lock(&hpn_ret_mutex);
        if (gettimeofday(&tv, NULL) < 0)
                err_sys("gettimeofday error");
        ts.tv_sec = tv.tv_sec + 5; //seconds in future
        ts.tv_nsec = tv.tv_usec * 1000; //microsec to nanosec

        while(hpnretcdone == 0)
                if ( (n = pthread_cond_timedwait(&hpn_ret_cond, &hpn_ret_mutex, &ts)) != 0)
                {
                        saveerrno = errno;
                        gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);
                        if (vDebugLevel > 8)
                                fprintf(pHpnServerLogPtr,"%s %s: ***INFO***: In fDoHpnRead(),  errno = %d, n= %d**\n", ms_ctime_buf, phase2str(current_phase), saveerrno,n);

                        if (n == ETIME || n == ETIMEDOUT) //apparently n could also be ETIME although man notes only mention ETIMEDOUT
                        {
                                if (vDebugLevel > 7)
                                {
                                        fprintf(pHpnServerLogPtr,"%s %s: ***WARNING***: In fDoHpnRead(), wait condition timed out errno = %d, n= %d**\n", ms_ctime_buf, phase2str(current_phase), saveerrno,n);
                                        fflush(pHpnServerLogPtr);
                                }

                                Pthread_mutex_unlock(&hpn_ret_mutex); //release the Kraken
                                //goto read_again;
                                y = recv(sockfd, &mychar, 1, MSG_DONTWAIT|MSG_PEEK);
                                saveerrno = errno;
                                if (vDebugLevel > 6)
                                {
                                        fprintf(pHpnServerLogPtr,"%s %s: ***INFO***: In fDoHpnRead(), after recv(), errno = %d, y= %d**\n", ms_ctime_buf, phase2str(current_phase), saveerrno,y);
                                        fflush(pHpnServerLogPtr);
                                }
                                if (saveerrno && !y) //cpnnection dropped on client side
                                {
                                        if (vDebugLevel > 1)
                                        {
                                                fprintf(pHpnServerLogPtr,"%s %s: ***INFO***: client closed connection, returning from read fDoHpnRead()****\n", ms_ctime_buf, phase2str(current_phase));
                                                fflush(pHpnServerLogPtr);
                                        }

                                        return;
                                }
                                else
                                        goto read_again;
                        }
                }
#endif
        memcpy(sRetMsg.timestamp, ms_ctime_buf, MS_CTIME_BUF_LEN);
        sRetMsg.hop_latency = count++;
        sRetMsg.queue_occupancy = count++;
        sRetMsg.switch_id = count++;

        sHpnRetMsg.pts = 0;

#if 0
        hpnretcdone = 0;
        Pthread_mutex_unlock(&hpn_ret_mutex);
#endif
#if 0
        if (!str_cli(sockfd, &sRetMsg))
                goto read_again;

        fprintf(pHpnServerLogPtr,"%s %s: ***WARNING***: client closed connection, EPIPE error???****\n", ms_ctime_buf, phase2str(current_phase));
        fflush(pHpnServerLogPtr);
#endif
	y = str_cli(sockfd, &sRetMsg);

return;
}

void fDoHpnAssessment(unsigned int val, int sockfd)
{
        time_t clk;
        char ctime_buf[27];
        char ms_ctime_buf[MS_CTIME_BUF_LEN];

        switch (val) {
                case  HPNSSH_READ:
                        fDoHpnRead(val, sockfd);
                        break;
                default:
                        gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);
                        fprintf(pHpnServerLogPtr,"%s %s: ***WARNING***: Invalid Hpnmessage value from client. Value is %u***\n", ms_ctime_buf, phase2str(current_phase), val);
                        break;
        }

return;
}
#endif	
	
#ifdef HPNSSH_QFACTOR
void * doHandleHpnsshQfactorEnv(void * vargp)
{
        time_t clk;
        char ctime_buf[27];
        char ms_ctime_buf[MS_CTIME_BUF_LEN];
        int listenfd, connfd;
        pthread_t tid;
        socklen_t clilen;
        struct sockaddr_in cliaddr, servaddr;

#if 1
                        struct sockaddr_in peeraddr;
                        socklen_t peeraddrlen;
                        struct sockaddr_in localaddr;
                        socklen_t localaddrlen;

                        peeraddrlen = sizeof(peeraddr);
                        localaddrlen = sizeof(localaddr);
#endif
			        gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);
        fprintf(pHpnServerLogPtr,"%s %s: ***Starting Listener for receiving messages from HPNSSH...***\n", ms_ctime_buf, phase2str(current_phase));
        fflush(pHpnServerLogPtr);

        listenfd = Socket(AF_INET, SOCK_STREAM, 0);

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family      = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port        = htons(vPort);

        Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
        Listen(listenfd, LISTENQ);

        for ( ; ; )
        {
                clilen = sizeof(cliaddr);
                if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0)
                {
                        if (errno == EINTR)
                                continue;  /* back to for() */
                        else
                                err_sys("accept error");
                }
#if 1
                fprintf(pHpnServerLogPtr,"%s %s: ***Accepted connection with Listener for receiving messages from HPNSSH...***\n", ms_ctime_buf, phase2str(current_phase));
                gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);

                int retval = getpeername(connfd, (struct sockaddr *) &peeraddr, &peeraddrlen);
                if (retval == -1)
                {
                        fprintf(pHpnServerLogPtr,"%s %s: ***Peer error:***\n", ms_ctime_buf, phase2str(current_phase));
                //      perror("getpeername()");
                }
                else
                        {
                                char *peeraddrpresn = inet_ntoa(peeraddr.sin_addr);
				sprintf(aDest_Ip2_Binary,"%02X",peeraddr.sin_addr.s_addr);
                                //total_time_passed = 0;
                                if (vDebugLevel > 1)
                                {
                                        fprintf(pHpnServerLogPtr,"%s %s: ***Peer information: ip long %s\n", ms_ctime_buf, phase2str(current_phase), aDest_Ip2_Binary);
                                        fprintf(pHpnServerLogPtr,"%s %s: ***Peer information:\n", ms_ctime_buf, phase2str(current_phase));
                                        fprintf(pHpnServerLogPtr,"%s %s: ***Peer Address Family: %d\n", ms_ctime_buf, phase2str(current_phase), peeraddr.sin_family);
                                        fprintf(pHpnServerLogPtr,"%s %s: ***Peer Port: %d\n", ms_ctime_buf, phase2str(current_phase), peeraddr.sin_port);
                                        fprintf(pHpnServerLogPtr,"%s %s: ***Peer IP Address: %s***\n\n", ms_ctime_buf, phase2str(current_phase), peeraddrpresn);
                                }
                        }

                retval = getsockname(connfd, (struct sockaddr *) &localaddr, &localaddrlen);
                if (retval == -1)
                {
                        fprintf(pHpnServerLogPtr,"%s %s: ***sock error:***\n", ms_ctime_buf, phase2str(current_phase));
                }
                else
                        {
                                char *localaddrpresn = inet_ntoa(localaddr.sin_addr);

                                if (vDebugLevel > 1)
                                {
                                        fprintf(pHpnServerLogPtr,"%s %s: ***Socket information:\n", ms_ctime_buf, phase2str(current_phase));
                                        fprintf(pHpnServerLogPtr,"%s %s: ***Local Address Family: %d\n", ms_ctime_buf, phase2str(current_phase), localaddr.sin_family);
                                        fprintf(pHpnServerLogPtr,"%s %s: ***Local Port: %d\n", ms_ctime_buf, phase2str(current_phase), ntohs(localaddr.sin_port));
                                        fprintf(pHpnServerLogPtr,"%s %s: ***Local IP Address: %s***\n\n", ms_ctime_buf, phase2str(current_phase), localaddrpresn);
                                }
                                //strcpy(aLocal_Ip,localaddrpresn);
                        }

                fflush(pHpnServerLogPtr);
#endif

#if 0
                if ( (childpid = Fork()) == 0)
                {        /* child process */

                        Close(listenfd); /* close listening socket */
                        process_request(connfd);/* process the request */
                        exit(0);
                }
#endif
                pthread_create(&tid, NULL, &doProcessHpnClientReq, (void *) connfd);
                //process_request(connfd);/* process the request */

                //Close(connfd); /* parent closes connected socket */
        }

        return ((char *)0);
}
#endif




int Writen(int fd, void *ptr, size_t nbytes)
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

void Inet_pton(int family, const char *strptr, void *addrptr)
{
	int             n;

	if ( (n = inet_pton(family, strptr, addrptr)) < 0)
		printf("inet_pton error for %s", strptr);      /* errno set */
	else 
		if (n == 0)
		{
			printf("inet_pton error for %s", strptr);     /* errno not set */
			exit(1);
		}
	return;
}

void Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (bind(fd, sa, salen) < 0)
		printf("bind error\n");

	return;
}

void sig_int_handler(int signum, siginfo_t *info, void *ptr)
{
	fprintf(pHpnServerLogPtr,"Caught SIGINT, exiting...\n");
	fflush(pHpnServerLogPtr);
	printf("Caught SIGINT, Shutting down server and exiting...\n");

	vShutdown = 1;
	exit(1);
}

void catch_sigint()
{
	static struct sigaction _sigact;

	memset(&_sigact, 0, sizeof(_sigact));
	_sigact.sa_sigaction = sig_int_handler;
	_sigact.sa_flags = SA_SIGINFO;

	sigaction(SIGINT, &_sigact, NULL);
}

#define DEBUGLEVELMAX	9
void sig_usr1_handler(int signum, siginfo_t *info, void *ptr)
{
	fprintf(pHpnServerLogPtr,"Caught SIGUSR1...\n");
	fprintf(pHpnServerLogPtr,"Debug Level is currently %d...\n",vDebugLevel);
	if (vDebugLevel < DEBUGLEVELMAX)
		vDebugLevel++;
	else
		vDebugLevel = 0;

	fprintf(pHpnServerLogPtr,"Debug Level is now %d...\n",vDebugLevel);
	fflush(pHpnServerLogPtr);
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

int str_cli(int sockfd, struct PeerMsg *sThisMsg) //str_cli09
{
        int y;
#ifdef HPNSSH_QFACTOR_BINN
        binn *myobj = binn_object();
        fMake_Binn_Server_Object(sThisMsg, myobj);
#if 0
        fprintf(pHpnServerLogPtr,"***!!!!!!!Size of binn object = %u...***\n", binn_size(myobj));
        fflush(pHpnServerLogPtr);
#endif
        y = Writen(sockfd, binn_ptr(myobj), binn_size(myobj));
        binn_free(myobj);
#else
        y = Writen(sockfd, sThisMsg, sizeof(struct PeerMsg));
#endif
        return y;
}

int main(int argc, char *argv[])
{
	time_t clk;
	char ctime_buf[27];
	char ms_ctime_buf[MS_CTIME_BUF_LEN];
	char aLogFile[256];
	sArgv_t sArgv;

	sArgv.argc = argc;
        sArgv.argv = argv;
	memset(aDest_Ip2_Binary,0,sizeof(aDest_Ip2_Binary));

#ifdef HPNSSH_QFACTOR
	int vRetFromHandleHpnsshQfactorEnvThread, vRetFromHandleHpnsshQfactorEnvJoin;
	pthread_t doHandleHpnsshQfactorEnvThread_id;
#endif

#ifdef HPNSSH_QFACTOR
        //Handle messages from hpnssh client
        vRetFromHandleHpnsshQfactorEnvThread = pthread_create(&doHandleHpnsshQfactorEnvThread_id, NULL, doHandleHpnsshQfactorEnv, &sArgv);
        memset(&sHpnRetMsg,0,sizeof(sHpnRetMsg));
        strcpy(sHpnRetMsg.msg, "Hello there!!! This is a Hpn msg...\n");
        sHpnRetMsg.msg_no = htonl(HPNSSH_MSG);

        memset(&sHpnRetMsg2,0,sizeof(sHpnRetMsg));
        strcpy(sHpnRetMsg2.msg, "Hello there!!! This is a Hpn msg...\n");
        sHpnRetMsg2.msg_no = htonl(HPNSSH_MSG);

        memset(&sTimeoutMsg,0,sizeof(sTimeoutMsg));
        strcpy(sTimeoutMsg.msg, "Hello there!!! This is  a dummy message ..., Here's some data\n");
        sTimeoutMsg.msg_no = htonl(HPNSSH_MSG);
        sTimeoutMsg.value = htonl(HPNSSH_DUMMY);;
        memcpy(sTimeoutMsg.timestamp, ms_ctime_buf, MS_CTIME_BUF_LEN);
        sTimeoutMsg.hop_latency = htonl(1);
        sTimeoutMsg.queue_occupancy = htonl(2);
        sTimeoutMsg.switch_id = htonl(3);
        sTimeoutMsg.seq_no = htonl(4);

        //Send messages tp HpnsshQfactor server for simulated testing
#endif

	sprintf(aLogFile,"/tmp/hpnServerLog");
	gettimeWithMilli(&clk, ctime_buf, ms_ctime_buf);

	pHpnServerLogPtr = fopen(aLogFile,"w");
	if (!pHpnServerLogPtr)
	{
		printf("%s %s: ***Couldn't open HPN Server log for testing, exiting...\n", ms_ctime_buf, phase2str(current_phase));
		exit(1);
	}

#if 0
	fprintf(pHpnServerLogPtr,"%s %s: ***Starting Client for testing with HPN-QFACTOR server...***\n", ms_ctime_buf, phase2str(current_phase));
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

	fprintf(pHpnServerLogPtr,"%s %s: ***Connecting to HPNSSN-QFACTOR Server %s, using port %d...***\n", 
							ms_ctime_buf, phase2str(current_phase), aMySrc_Ip, vPort);
	fflush(pHpnServerLogPtr);
#endif
	catch_sigint();
	catch_sigusr1();

#ifdef HPNSSH_QFACTOR
        if (vRetFromHandleHpnsshQfactorEnvThread == 0)
                vRetFromHandleHpnsshQfactorEnvJoin = pthread_join(doHandleHpnsshQfactorEnvThread_id, NULL);
#endif

return 0;
}

