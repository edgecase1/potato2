

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

#include "logger.h"
#include "userlist.h"
#include "runr.h"
#include "session.h"

void
shell(t_user* user)
{
    t_runr_args arg;
    char child_stack[STACK_SIZE];
    pid_t pid = -1;

    fprintf(stdout, "starting shell '%s'. Name '%s' User ID %d, home '%s' ...\n", 
       	     user->shell, 
       	     user->name,
       	     user->id, 
       	     user->home); 
    fflush(stdout);

    arg.home = user->home;
    arg.id = user->id;
    arg.gid = 22222;
    strncpy(arg.proc, user->shell, strlen(user->shell));

    printf("cloning a new child\n");
    pid = runr_start(&arg); // this runs the shell in a container
    printf("child pid is %d\n", pid);
    waitpid(pid, NULL, 0);
    printf("child died\n");
}

int create_user(char* input_username, char* input_password)
{
    t_user* user;
    t_user_list_element* element;
    int free_id = -1;

    if(strlen(input_username) <= 0 || strlen(input_password) <= 0)
    {
         LOG("username or password empty");
         return -1;
    }
    
    if(get_user_by_name(input_username) != NULL)
    {
         LOG("username already taken");
         return -1;
    }
    user = new_user(input_username, input_password);
    free_id = next_free_id();
    user->id = free_id;
    user->gid = free_id;

    add_user_to_list(user);
    return user->id;
}

int
login(t_session* session, char* input_username, char* input_password)
{
    t_user* user;
    LOG("searching for user");
    if((user = get_user_by_name(input_username)) == NULL)
    {
        LOG("no such user\n");
        return -1;
    }

    LOG("checking password");
    if(check_password(user, input_password) == 1)
    {
        LOG("You are authorized.\n");
        session->logged_in_user = user;
        session->start_time = time(0);
        chdir(user->home);
	return user->id;
    }
    else
    {
    	LOG("Authentication failure");
	return -1;
    }
}

void
logout(t_session* session)
{
    //session = NULL;
    session->logged_in_user = NULL;
    //session->start_time = 0;
}

void
change_name(t_user* user, char* input_username)
{
    strncpy(user->name, input_username, strlen(input_username)+1);
    fprintf(stdout, "Name changed.\n");
}

void
change_password(t_user* user, char* input_password)
{
    strncpy(user->password_hash, 
            str2md5(input_password, strlen(input_password)), 
	    32);
    fprintf(stdout, "Password changed.\n");
}

void
whoami(t_user* user)
{
    if(user != NULL)
    {
         print_user(user);
    }
    else
    {
	 puts("not logged in.");
    }
}

void
not_implemented()
{
    puts("not implemented");
}

