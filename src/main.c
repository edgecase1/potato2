
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "userlist.h"
#include "user.h"
#include "sock.h"

struct _session
{
     t_user* logged_in_user;
     time_t start_time;
} typedef t_session;

void logout();

// globals
t_user_list user_list;
t_session session;

void
init()
{
     logout();
}

void print_usage()
{
     printf("> list\n");
     printf("> new\n");
     printf("> delete\n");
     printf("> read\n");
     printf("> purge\n");
     printf("> debug\n");
     printf("\n");
     printf("> login\n");
     printf("> logout\n");
     printf("\n");
     printf("> shell\n");
     printf("> whoami\n");
     printf("> changepw\n");
     printf("> changename\n");
}

int
is_privileged(t_user* user)
{
     if(user->gid < 1) // 
     {
          return 1;
     } else {
          return 0;
     }
}

void
create_user()
{
     char input_username[USERNAME_LENGTH];
     char* input_password;
     t_user* user;
     t_user_list_element* element;
     
     fprintf(stdout, "What is the name > ");
     fgets(input_username, sizeof(input_username), stdin);
     input_username[strcspn(input_username, "\n")] = 0x00; // terminator instead of a newline
     input_password = getpass("Password: "); fflush(stdout);
     
     if(get_user_by_name(input_username) != NULL)
     {
	     fprintf(stdout, "username already taken\n");
	     free(input_password);
          return;
     }
     user = new_user(input_username, input_password);
     element = get_last_element();
     if(element != NULL) // avoid NULL pointer exception
     {
          user->id = element->user->id + 1; // last known id+1
          user->gid = element->user->id + 1; // same as id
     }
     add_user_to_list(user);
}

void
shell(int stdin, int stdout)
{
     char *argv[] = { NULL, NULL, NULL, NULL };
     struct stat st = {0};
     char *envp[] = {
        "PS1=C:\\ >",
        "PATH=/bin:/usr/bin",
        0
    };

     if(session.logged_in_user == NULL)
     {
          fprintf(stderr, "not logged in.\n");
          return;
     }
     fprintf(stdout, "starting shell '%s'. User ID %d, home '%s' ...\n", session.logged_in_user->shell, session.logged_in_user->id, session.logged_in_user->home); 
     fflush(stdout);
     // home dir
     if (stat(session.logged_in_user->home, &st) == -1)
     {
          mkdir(session.logged_in_user->home, 0700);
     }
     chown(session.logged_in_user->home, session.logged_in_user->id, session.logged_in_user->id);
     chdir(session.logged_in_user->home);
     // change id
     if(setgroups(0, NULL) < 0) { // remove all groups
          fprintf(stderr, "can't remove groups. need sudo."); 
          return ; 
     }; 
     if(setresgid(session.logged_in_user->id, 
                  session.logged_in_user->id, 
                  session.logged_in_user->id) < 0) { 
          fprintf(stderr, "can't setresgid. need sudo.");
          return ; 
     }; 
     if(setresuid(session.logged_in_user->id, 
                  session.logged_in_user->id, 
                  session.logged_in_user->id) < 0) {
          fprintf(stderr, "can't setresuid. need sudo."); 
          return ; 
     };
     // start shell
     argv[0] = session.logged_in_user->shell;
     //argv[1] = "-i";
     execve(session.logged_in_user->shell, argv, envp);
}

void
login(int stdin, int stdout)
{
     char input_username[USERNAME_LENGTH];
     char* input_password;
     t_user* user;

     fputs("Welcome!\n", stdout);
     fputs("username: ", stdout); 
     fflush(stdout);
     fgets(input_username, USERNAME_LENGTH, stdin);
     input_username[strcspn(input_username, "\n")] = 0x00;
     input_password = getpass("Password: "); fflush(stdout);

     if((user = get_user_by_name(input_username)) == NULL)
     {
          fprintf(stdout, "no such user\n");
          return;
     }

     if(check_password(user, input_password) == 1)
     {
        fprintf(stdout, "You are authorized.\n");
        session.logged_in_user = user;
        session.start_time = time(0);
        chdir(user->home);
     }
     else
     {
     	fprintf(stderr, "Authentication failure\n");
     }
}

void
logout()
{
     session.logged_in_user = NULL;
     session.start_time = 0;
}

void
change_name(int stdin, int stdout)
{
     char input_username[USERNAME_LENGTH];
         
     fprintf(stdout, "What is the name > ");
     fgets(input_username, sizeof(input_username), stdin);
     input_username[strcspn(input_username, "\n")] = 0x00; // terminator instead of a newline

     strncpy(session.logged_in_user->name, input_username, strlen(input_username)+1);
     fprintf(stdout, "Name changed.\n");
}

void
change_password()
{
     char* input_password;

     input_password = getpass("Password: "); fflush(stdout);
     strncpy(session.logged_in_user->password_hash, str2md5(input_password, strlen(input_password)), 32); // error: password not hash 
     fprintf(stdout, "Password changed.\n");
}

void
whoami()
{
     if(session.logged_in_user != NULL)
     {
          print_user(session.logged_in_user);
     }
}

void
not_implemented()
{
    puts("not implemented");
}

int
main(int argc, char** argv)
{
     int id = 0;
     user_list.number_of_elements = 0;
     user_list.head = NULL;
     char command[255];
     char buf[255];
     char user[100];
     char password[100];
     t_server *server;
     int client;

     init();
     read_list("userlist");

     start_server();

     while((client = server->next_client())) // command line loop
     {
	  int stdin = stdin;
	  int stdout = stdout;
	  int stderr = stderr;

          fprintf(stdout, "\ncmd> ");
          if(fgets(command, sizeof(command), stdin) == NULL) break;
          command[strcspn(command, "\n")] = 0x00;
 
          if(strncmp(command, "list", 4) == 0)
	     { // list
              walk_list(print_list_element);
	     }
	     else if(strncmp(command, "new", 3) == 0)
	     { // new
	          create_user();
	     }
	     else if(strncmp(command, "delete", 6) == 0)
	     { // delete
               if(session.logged_in_user == NULL)
               {
                    fprintf(stderr, "not logged in.\n");
                    continue;
               }   
               if(!is_privileged(session.logged_in_user))
               {
                    fprintf(stderr, "privileged users only!");
                    continue;
               }
               walk_list(print_list_element);
               fprintf(stdout, "Which one? > ");
               scanf("%d", &id);
               if(!delete_user_by_id(id)) {
                    fprintf(stderr, "not found.\n");
               }
	     }
	     else if(strncmp(command, "read", 4) == 0)
	     {
               if(session.logged_in_user == NULL)
               {
                    fprintf(stderr, "not logged in.\n");
                    continue;
               }   
               if(!is_privileged(session.logged_in_user))
               {
                    fprintf(stderr, "privileged users only!");
                    continue;
               }
		     //purge_list();
               read_list("userlist");
	     } 
          else if(strncmp(command, "write", 5) == 0)
	     {
               if(session.logged_in_user == NULL)
               {
                    fprintf(stderr, "not logged in.\n");
                    continue;
               }   
               if(!is_privileged(session.logged_in_user))
               {
                    fprintf(stderr, "privileged users only!");
                    continue;
               }
               write_list("userlist");
	     } 
	     else if(strncmp(command, "purge", 5) == 0)
	     {
               if(session.logged_in_user == NULL)
               {
                    fprintf(stderr, "not logged in.\n");
                    continue;
               }   
               if(!is_privileged(session.logged_in_user))
               {
                    fprintf(stderr, "privileged users only!");
                    continue;
               }
               purge_list();
	     }
	     else if(strncmp(command, "debug", 5) == 0)
	     {
               if(session.logged_in_user == NULL)
               {
                    fprintf(stderr, "not logged in.\n");
                    continue;
               }   
               if(!is_privileged(session.logged_in_user))
               {
                    fprintf(stderr, "privileged users only!");
                    continue;
               }
               walk_list(print_debug);
	     }
	     else if(strncmp(command, "login", 5) == 0)
	     {
		    login();
	     }
          else if(strncmp(command, "logout", 6) == 0)
	     {
		    logout();
	     }
          else if(strncmp(command, "shell", 5) == 0)
	     {
		    shell();
	     }
          else if(strncmp(command, "changepw", 8) == 0)
	     {
               if(session.logged_in_user == NULL)
               {
                    fprintf(stderr, "not logged in.\n");
                    continue;
               }   
		     change_password();
	     }
          else if(strncmp(command, "changename", 10) == 0)
	     {
               if(session.logged_in_user == NULL)
               {
                    fprintf(stderr, "not logged in.\n");
                    continue;
               }   
		    change_name();
	     }
          else if(strncmp(command, "whoami", 6) == 0)
	     {
               if(session.logged_in_user == NULL)
               {
                    fprintf(stderr, "not logged in.\n");
                    continue;
               }   
		     whoami();
	     }
          else if(strncmp(command, "exit", 4) == 0)
	     {
		    //if net close client socket
		    //close_client();
		    //else is console
		    return 0;
	     }
	     else
	     {
               print_usage();
          }
     }

     stop_server();
	
     return 0;
}
