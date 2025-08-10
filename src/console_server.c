
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "user.h"
#include "func.h"
#include "userlist.h"

#define is_privileged() _is_privileged(console_session->logged_in_user)
#define is_authenticated() _is_authenticated(console_session->logged_in_user)

// globals
t_session *console_session;

int
_is_privileged(t_user* user)
{
     if(user->id < 1 || user->gid < 1) // is a root user
     {
          return 1;
     }
     else
     {
          fprintf(stderr, "privileged users only!");
          return 0;
     }
}

int
_is_authenticated(t_user* user)
{
    if(user == NULL)
    {
        fprintf(stderr, "not logged in.\n");
        return 0;
    }
    else
    {
        return 1;
    }
}

void
console_login()
{
    char input_username[USERNAME_LENGTH];
    //char* input_password;
    char input_password[PASSWORD_LENGTH];

    fputs("Welcome!\n", stdout);
    fputs("username: ", stdout); fflush(stdout);
    fgets(input_username, USERNAME_LENGTH, stdin);
    input_username[strcspn(input_username, "\n")] = 0x00;
    // if terminal
    //input_password = getpass("Password: "); fflush(stdout);
    fputs("password: ", stdout); fflush(stdout);
    fgets(input_password, PASSWORD_LENGTH, stdin);
    input_password[strcspn(input_password, "\n")] = 0x00;

    t_session tmp_session;
    if(login(&tmp_session, input_username, input_password) != -1)
    {
	create_session(&tmp_session);
	console_session = &tmp_session;
        fprintf(stdout, "Authentication successful.\n");
    }
    else
    {
        fprintf(stderr, "Authentication failed.\n");
    }
}

void
console_logout()
{
    logout(console_session);
}

void
console_create_user()
{
    char input_username[USERNAME_LENGTH];
    char input_password[PASSWORD_LENGTH];

    fprintf(stdout, "What is the name > ");
    fgets(input_username, sizeof(input_username), stdin);
    input_username[strcspn(input_username, "\n")] = 0x00; // terminator instead of a newline
   
    //input_password = getpass("Password: "); fflush(stdout);
    fprintf(stdout, "Password: ");
    fgets(input_password, sizeof(input_password), stdin);
    input_password[strcspn(input_password, "\n")] = 0x00; // terminator instead of a newline
    if(create_user(input_username, input_password) != -1)
    {
    	fprintf(stdout, "User added.\n");
    }
    else
    {
        fprintf(stderr, "Operation failed.\n");
    }
}

void
console_change_name()
{
    char input_username[USERNAME_LENGTH];
        
    fprintf(stdout, "What is the name > ");
    //fgets(input_username, sizeof(input_username), stdin);
    fscanf(stdin, "%s", input_username); // TODO security
    input_username[strcspn(input_username, "\n")] = 0x00; // terminator instead of a newline
    change_name(console_session->logged_in_user, input_username);
}

void
console_change_password()
{
    char input_password[PASSWORD_LENGTH];
    fprintf(stdout, "Password: ");
    fgets(input_password, sizeof(input_password), stdin);
    input_password[strcspn(input_password, "\n")] = 0x00; // terminator instead of a newline
    change_password(console_session->logged_in_user, input_password);
}

void
console_delete_user()
{
    int id;
    walk_list(print_list_element);
    fprintf(stdout, "Which one? > ");
    scanf("%d", &id);
    if(!delete_user_by_id(id)) {
         fprintf(stderr, "not found.\n");
    }
}

void
console_list_users()
{
    walk_list(print_list_element);
}

void
console_shell()
{
    shell(console_session->logged_in_user);
}

void
console_whoami()
{
    whoami(console_session->logged_in_user);
}

void
print_usage()
{
     printf("> register\t\tcreate a temporary user\n");
     printf("> login\t\tauthenticate a user\n");
     printf("> logout\t\tdestroy the current session\n");
     printf("\n");
     printf("functions as authenticated user\n");
     printf("> whoami\t\tshow user id\n");
     printf("> changepw\t\tchange the password\n");
     printf("> changename\t\tchange the username\n");
     printf("> shell\t\tstart the shell in the user profile\n");
     printf("\n");
     printf("system functions:\n");
     printf("> list\t\tshow all registered users\n");
     printf("> delete\t\tdelete a user\n");
     printf("> read\t\tparse the 'userlist' file\n");
     printf("> write\t\twrite out the 'userlist' file\n");
     printf("> purge\t\tempty the userlist\n");
     printf("> debug\t\tshow the userlist data structure\n");
     printf("> exit\t\tclose the program/connection\n");
}

void
handle_client()
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
            if(! is_authenticated()) continue;
            console_list_users();
	}
       	else if(strncmp(command, "register", 8) == 0)
        { 
	    // new user (free for all)
            console_create_user();
        }
        else if(strncmp(command, "delete", 6) == 0)
        { // delete
            if(! is_authenticated() || ! is_privileged()) continue;
            console_delete_user();
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
            walk_list(print_debug);
        }
        else if(strncmp(command, "login", 5) == 0)
        {
            console_login();
        }
        else if(strncmp(command, "logout", 6) == 0)
        {
            if(! is_authenticated()) continue;
            console_logout();
        }
        else if(strncmp(command, "shell", 5) == 0)
        {
            if(! is_authenticated()) continue;
	    console_shell();
        }
        else if(strncmp(command, "changepw", 8) == 0)
        {
            if(! is_authenticated()) continue;
            console_change_name();
        }
        else if(strncmp(command, "changename", 10) == 0)
        {
            if(! is_authenticated()) continue;
            console_change_name();
        }
        else if(strncmp(command, "whoami", 6) == 0)
        {
            if(! is_authenticated()) continue;
            console_whoami();
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
