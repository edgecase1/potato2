
#define GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "logger.h"
#include "userlist.h"
#include "user.h"
#include "sock.h"
#include "func.h"
#include "http.h"
#include "console_server.h"

#define MODE_SERVER 1
#define MODE_CONSOLE 2
#define MODE_HTTP 3

int
main(int argc, char** argv)
{
    int mode = -1;

    if(argc == 2 && strncmp("server", argv[1], 6) == 0)
    {
        mode = MODE_SERVER;
    }
    else if(argc == 2 && strncmp("console", argv[1], 7) == 0)
    {
        mode = MODE_CONSOLE;
    }
    else if(argc == 2 && strncmp("http", argv[1], 4) == 0)
    {
        mode = MODE_HTTP;
    }
    else
    {
	printf("%s console\n", argv[0]);
	printf("%s server\n", argv[0]);
	printf("%s http\n", argv[0]);
	exit(1);
    }

    assert(mode != -1);
    printf("starting up (pid %d)\n", getpid());
    setbuf(stdout, NULL); // unbuffered stdout
    init_userlist();
    read_list("userlist");
    switch(mode)
    {
       case MODE_SERVER:
          start_server(&handle_client);
          stop_server();
          break;
       case MODE_CONSOLE:
          handle_client();
          break;
       case MODE_HTTP:
          pthread_t t;
          pthread_create(&t, NULL, &http_server, NULL);
          pthread_detach(t);
          handle_client();
          break;
       default:
	  printf("error\n");
	  exit(2);
	  break;
    }

    return 0;
}
