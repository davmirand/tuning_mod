#include "fio_cli.h"
#include "http.h"

static void on_response(http_s *h);

static char * Usage = "This is an HTTP client to talk to Tuning Module. \
		       \nPlease use \"tuncli -t\" to apply tunings that were recommended \n";

int main(int argc, const char *argv[]) 
{
	int argc_try = 2;
	char *localaddr = "http://127.0.0.1";
	char aTmp[512];
	char aListenPort[32];
	char *gAPI_listen_port = "5523"; //default
	char *pAPI_listen_port_name = "API_listen_port";
	const char * argv_try[2];
	char *pUserCfgFile = "user_config.txt";
	FILE *userCfgPtr = 0;
	int found = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	char *p = 0;
	char setting[256];

	if (argc != 2)
	{
		printf("%s",Usage);
		exit(0);
	}

	if (strcmp(argv[1],"-t") != 0)
	{
		printf("%s",Usage);
		exit(-1);
	}
		
	userCfgPtr = fopen(pUserCfgFile,"r");
	if (!userCfgPtr)
	{
		int save_errno = errno;
		printf("\n***ERROR : Opening of %s failed, errno = %d\n", pUserCfgFile, save_errno);
		printf("\n***Exiting...\n");
                return -1;
        }

	while ((nread = getline(&line, &len, userCfgPtr)) != -1)
	{
		int ind = 0;
		memset(setting,0,sizeof(setting));
		p = line;
		while (!isblank((int)p[ind]))
		{
			setting[ind] = p[ind];
			ind++;
		}

		/* compare with known item in list now */
		if (strcmp(pAPI_listen_port_name, setting) == 0) //found
		{
			int y = 0;
			memset(setting,0,sizeof(setting));
			while (isblank((int)p[ind])) //get past blanks etc
				ind++;

			setting[y++] = p[ind++];

			while (isalnum((int)p[ind]))
			{
				setting[y++] = p[ind++]; 
			}

			strcpy(aListenPort, setting);
			found = 1;
			break;
		}
	}

	if (!found)
		strcpy(aListenPort,gAPI_listen_port);

	sprintf(aTmp,"%s:%s/%s",localaddr,aListenPort,argv[1]);
	/* a hack so I could use 127.0.0.1 */
	argv_try[0] = "tuncli";
	argv_try[1] = aTmp;

	fio_cli_start(
		argc_try, argv_try, 1, 1, NULL,
		//"This is an HTTP client to talk to Tuning Module, use\n"
		//"\n\ttuncli - http://example.com/foo\n",
		FIO_CLI_STRING("-unix -u Unix Socket address (has no place in url)."),
		FIO_CLI_STRING("-test -t test (has no place in url).")
	);
    
	http_connect(fio_cli_unnamed(0), fio_cli_get("-u"),
		.on_response = on_response);
	fio_start(.threads = 1);
	return 0;
}

static void on_response(http_s *h) 
{
	if (h->status_str == FIOBJ_INVALID) 
	{
		/* first response is always empty, nothing was sent yet */
		http_finish(h);
		return;
	}
  	
	/* Second response is actual response */
	FIOBJ r = http_req2str(h);
	fprintf(stderr, "%s\n", fiobj_obj2cstr(r).data);
	fio_stop();
}
