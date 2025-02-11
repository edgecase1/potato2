
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <sys/mman.h>

#include "userlist.h"
#include "user.h"
#include "sock.h"

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

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
     printf("> register\n");
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
	
bool
is_authenticated()
{
    if(session.logged_in_user == NULL)
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
delete_user()
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
create_user()
{
    char input_username[USERNAME_LENGTH];
    char input_password[PASSWORD_LENGTH];
    t_user* user;
    t_user_list_element* element;
    int free_id = -1;
    
    fprintf(stdout, "What is the name > ");
    fgets(input_username, sizeof(input_username), stdin);
    input_username[strcspn(input_username, "\n")] = 0x00; // terminator instead of a newline
   
    if(get_user_by_name(input_username) != NULL)
    {
         fprintf(stdout, "username already taken\n");
         free(input_password);
         return;
    }

    //input_password = getpass("Password: "); fflush(stdout);
    fprintf(stdout, "Password: ");
    fgets(input_password, sizeof(input_password), stdin);
    input_password[strcspn(input_password, "\n")] = 0x00; // terminator instead of a newline
    user = new_user(input_username, input_password);
    free_id = next_free_id();
    user->id = free_id;
    user->gid = free_id;
    /*
    element = get_last_element();
    if(element != NULL) // avoid NULL pointer exception
    {
         user->id = element->user->id + 1; // last known id+1
         user->gid = element->user->id + 1; // last known id+1
         //user->gid = element->user->id + 1; // same as id
    }
    */
    add_user_to_list(user);
    fprintf(stdout, "User added.");
}

static int
child_func(void *arg)
{

    // remount proc to see the current pid namespace in /proc
    if (mount("proc", "/proc", "proc", 0, NULL) == -1)
    {
        errExit("mount");
    }

    char *argv[] = {"/bin/sh", NULL};  // Arguments to pass to execve
    char *envp[] = {NULL};             // Environment variables

    execve("/bin/sh", argv, envp);     // Execute shell
    printf("done\n");
}

int test(void *arg)
{
    char **args = arg;
    char *envp = { 0 };
    struct stat st = {0};
    t_user* user = session.logged_in_user;

    exit(1);

    fprintf(stderr, "args %s", args[0]);
    // home dir
    if (stat(user->home, &st) == -1) // home must exist
    {
         mkdir(user->home, 0700);
    }
    // set persmissions and chdir into it
    chown(user->home, user->id, user->gid);
    chdir(user->home);
    // remove all groups
    if(setgroups(0, NULL) < 0) {
         fprintf(stderr, "can't remove groups. need sudo."); 
         return ; 
    }; 
    // change the uid and gid
    if(setresgid(user->gid, 
                 user->gid, 
                 user->gid) < 0) { 
         fprintf(stderr, "can't setresgid. need sudo.");
         return ; 
    }; 
    if(setresuid(user->id, 
                 user->id, 
                 user->id) < 0) {
         fprintf(stderr, "can't setresuid. need sudo."); 
         return ; 
    };

    char *argv[] = {"/bin/sh", 0};
    execve(argv[0], argv, envp);
}

void
shell()
{
    char *argv[3];

    t_user* user = session.logged_in_user;

    fprintf(stdout, "starting shell '%s'. Name '%s' User ID %d, home '%s' ...\n", 
       	     user->shell, 
       	     user->name,
       	     user->id, 
       	     user->home); 
    fflush(stdout);


    // start shell
    argv[0] = "/bin/sh"; // TODO user->shell;
    argv[1] = "/bin/sh";
    

    wait(); // till the termination of the shell
}

void
login()
{
    char input_username[USERNAME_LENGTH];
    //char* input_password;
    char input_password[PASSWORD_LENGTH];
    t_user* user;

    fputs("Welcome!\n", stdout);
    fputs("username: ", stdout); fflush(stdout);
    fgets(input_username, USERNAME_LENGTH, stdin);
    input_username[strcspn(input_username, "\n")] = 0x00;
    // if terminal
    //input_password = getpass("Password: "); fflush(stdout);
    fputs("password: ", stdout); fflush(stdout);
    fgets(input_password, PASSWORD_LENGTH, stdin);
    input_password[strcspn(input_password, "\n")] = 0x00;

    printf("searching for user ...\n");
    if((user = get_user_by_name(input_username)) == NULL)
    {
        fprintf(stdout, "no such user\n");
        return;
    }

    printf("checking password ...\n");
    if(check_password(user, &input_password) == 1)
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
change_name()
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

#define STACK_SIZE (1024*1024)
static char child_stack[STACK_SIZE];

void handle_client()
{
    char command[255];

    printf("handle_client");
    while(true) {
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
            if(! is_authenticated() || ! is_privileged(session.logged_in_user)) continue;
            delete_user();
        }
        else if(strncmp(command, "read", 4) == 0)
        {
            if(! is_authenticated()) continue;
            if(! is_privileged(session.logged_in_user)) continue;
            //purge_list();
            read_list("userlist");
        }
        else if(strncmp(command, "write", 5) == 0)
        {
            if(! is_authenticated()) continue;
            if(! is_privileged(session.logged_in_user)) continue;
            write_list("userlist");
        } 
        else if(strncmp(command, "purge", 5) == 0)
        {
            if(! is_authenticated()) continue;
            if(! is_privileged(session.logged_in_user)) continue;
            purge_list();
        }
        else if(strncmp(command, "debug", 5) == 0)
        {
            if(! is_authenticated()) continue;
            if(! is_privileged(session.logged_in_user)) continue;
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
            pid_t pid = -1;
            // need to fork for unshare
	    // https://lwn.net/Articles/531419/
            char *stack = mmap(NULL,
                               STACK_SIZE,
                               PROT_READ | PROT_WRITE,  // rw-
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, // private stack, not shared
                               -1,
                               0);
            if (stack == MAP_FAILED)
            {
                perror("mmap");
                exit(1);
            }
            if ((pid = clone(child_func,         // called routine
                	     child_stack + STACK_SIZE,      // stack top
                	     CLONE_NEWPID | SIGCHLD,  //
                	     NULL)) == -1)            // function argument
            {
                perror("clone failed.");
                exit(1);
            }
            printf("child pid is %d\n", pid);
	    waitpid(pid, NULL, 0);
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
            return 0;
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
    int client_fd;
    pid_t p;
    int server_fd;

    setbuf(stdout, NULL);

    init();
    user_list.number_of_elements = 0;
    user_list.head = NULL;
    read_list("userlist");

    //handle_client();

    start_server();

    while(client_fd = next_client()) // command line loop
    {
	p = fork();
        if(p<0) {
            printf("fork error");
            break;
        } else if (p == 0) { // child
            close(server_fd);
	    dup2(client_fd, STDIN_FILENO);
            dup2(client_fd, STDOUT_FILENO);
            dup2(client_fd, STDERR_FILENO);
            close(client_fd);
            handle_client();
	    close(stdin);
	    close(stdout);
	    close(stderr);
	    exit(0);
        } else { // parent 
            close(client_fd);
	}
	// Optionally, clean up zombie processes
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

     stop_server();
     return 0;
}
