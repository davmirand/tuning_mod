#include "fio_cli.h"
#include "http.h"

static void on_response(http_s *h);

int main(int argc, const char *argv[]) 
{
	int argc_try = 3;
	char *tmp = "http://67.17.206.218:3000/my_ppath?foo=bar";
	const char * argv_try[3];

	argv_try[0] = "fioappcli";
	argv_try[1] = tmp;
	argv_try[2] = "-t yyyyy";

	fio_cli_start(
		argc_try, argv_try, 1, 1, NULL,
	//fio_cli_start(
		//argc, argv, 0, 0,
		//"This is an HTTP client example, use:\n"
		//"\n\tfioapp http://example.com/foo\n",
		FIO_CLI_STRING("-test -t test (has no place in url)."),
		FIO_CLI_STRING("-unix -u Unix Socket address (has no place in url).")
	);
    
	//http_connect(fio_cli_unnamed(1), fio_cli_get("-u"),
	printf("f = ***%s***\n",fio_cli_get("-u"));
	printf("t = ***%s***\n",fio_cli_get("-t"));

	if (argc > 1)
	printf("argc[1] = ***%s***\n",argv[1]);
 	
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
