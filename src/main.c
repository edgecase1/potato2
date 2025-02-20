
#define GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "userlist.h"
#include "user.h"
#include "sock.h"
#include "func.h"

void
init()
{
     init_userlist();
     logout();
}

void handle_client()
{
    char command[255];

    printf("handle_client");
    while(1) {
        fprintf(stdout, "\ncmd> ");
        if(fgets(command, sizeof(command), stdin) == NULL)
	{
	    break;
	}
        command[strcspn(command, "\n")] = 0x00; // null terminator

        if(strncmp(command, "list", 4) == 0)
        { // list
            walk_list(print_list_element);

	}
       	else if(strncmp(command, "register", 8) == 0)
        { 
	    // new user (free for all)
            create_user();
        }
        else if(strncmp(command, "delete", 6) == 0)
        { // delete
            if(! is_authenticated() || ! is_privileged()) continue;
            delete_user();
        }
        else if(strncmp(command, "read", 4) == 0)
        {
            if(! is_authenticated()) continue;
            if(! is_privileged()) continue;
            //purge_list();
            read_list("userlist");
        }
        else if(strncmp(command, "write", 5) == 0)
        {
            if(! is_authenticated()) continue;
            if(! is_privileged()) continue;
            write_list("userlist");
        } 
        else if(strncmp(command, "purge", 5) == 0)
        {
            if(! is_authenticated()) continue;
            if(! is_privileged()) continue;
            purge_list();
        }
        else if(strncmp(command, "debug", 5) == 0)
        {
            if(! is_authenticated()) continue;
            if(! is_privileged()) continue;
            walk_list(print_debug);
        }
        else if(strncmp(command, "login", 5) == 0)
        {
            login();
        }
        else if(strncmp(command, "logout", 6) == 0)
        {
            if(! is_authenticated()) continue;
            logout();
        }
        else if(strncmp(command, "shell", 5) == 0)
        {
            if(! is_authenticated()) continue;
            shell();
        }
        else if(strncmp(command, "changepw", 8) == 0)
        {
            if(! is_authenticated()) continue;
            change_password();
        }
        else if(strncmp(command, "changename", 10) == 0)
        {
            if(! is_authenticated()) continue;
            change_name();
        }
        else if(strncmp(command, "whoami", 6) == 0)
        {
            if(! is_authenticated()) continue;
            whoami();
        }
        else if(strncmp(command, "exit", 4) == 0)
        {
            //if net close client socket
            //close_client();
            //else is console
            return;
        }
        else if(strncmp(command, "help", 4) == 0)
        {
            print_usage();
	}
        else
        {
	    fprintf(stderr, "Type 'help' to print the usage.");
        }
    } // while
}

int
main(int argc, char** argv)
{
    setbuf(stdout, NULL);
    init();
    read_list("userlist");
    start_server(&handle_client);
    stop_server();
    return 0;
}
