
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <mntent.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <grp.h>
#include <sched.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "runr.h"

#define MAX_MOUNTS 1024
#define MAX_PATH 512

#define errExit(msg)    do { \
                            fprintf(stderr, "error: %s\n", msg); exit(EXIT_FAILURE); \
                        } while (0)

#define BUF_SIZE 1024

void
copy_file(char* source, char* dest)
{
    int inputFd, outputFd, openFlags;
    mode_t filePerms;
    ssize_t numRead;
    char buf[BUF_SIZE];    

    inputFd = open(source, O_RDONLY);
    if (inputFd == -1)
        errExit("opening file");

    openFlags = O_CREAT | O_WRONLY | O_TRUNC;
    filePerms = S_IRUSR | S_IWUSR | S_IXUSR |
	        S_IRGRP | S_IWGRP | S_IXGRP |
                S_IROTH | S_IXOTH;      /* rwxrwxr-x */
    outputFd = open(dest, openFlags, filePerms);
    if (outputFd == -1)
        errExit("opening file");

    while ((numRead = read(inputFd, buf, BUF_SIZE)) > 0)
        if (write(outputFd, buf, numRead) != numRead)
            errExit("write() returned error or partial write occurred");
    if (numRead == -1)
        errExit("read");

    if (close(inputFd) == -1)
        errExit("close input");
    if (close(outputFd) == -1)
        errExit("close output");
}

void
passwd_add(char* name, int id, int gid, char* home)
{
    FILE* fp = fopen("etc/passwd", "a");
    if (!fp) {
        errExit("write /etc/passwd");
    }
    fprintf(fp, "%s:x:%d:%d:User:%s:/bin/sh\n",
    		name,
    		id,
    		gid,
    		home);
    fclose(fp);
}

void
group_add2(char* name, int gid)
{    
    FILE* fp = fopen("etc/group", "w");
    if (!fp) {
        errExit("write /etc/group");
    }
    fprintf(fp, "potato:x:22222:\n"); // TODO
    fclose(fp);
}

int
umount_all()
{
    FILE *fp = fopen("/proc/self/mounts", "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    char mounts[MAX_MOUNTS][MAX_PATH];
    int count = 0;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        char source[MAX_PATH], target[MAX_PATH], fstype[MAX_PATH];
        if (sscanf(line, "%s %s %s", source, target, fstype) == 3) {
            if (count < MAX_MOUNTS) {
                strncpy(mounts[count], target, MAX_PATH);
                count++;
            }
        }
    }
    fclose(fp);

    // Unmount in reverse order
    for (int i = count - 1; i >= 0; i--) {
        if (strcmp(mounts[i], "/") == 0)
            continue;  // never unmount root
        if (strcmp(mounts[i], "/proc") == 0)
            continue;  // never unmount proc

        if (umount2(mounts[i], MNT_DETACH) == -1) {
            fprintf(stderr, "Failed to unmount %s: %s\n",
                    mounts[i], strerror(errno));
        } else {
            printf("Unmounted %s\n", mounts[i]);
        }
    }

    return 0;
}

static int 
child_func(void *arg)
{
    char *envp[] = {NULL};             // discard env
    struct stat st = {0};
    char cwd[1024];
    char old[255] = {};
    getcwd(cwd, sizeof(cwd));
    t_runr_args *runr_args = (t_runr_args *)arg;
    //system("/bin/sh");

    fprintf(stderr, "alive id=%d gid=%d shell=%s home=%s root=%s\n", 
                                            runr_args->id,
		    			    runr_args->gid,
					    runr_args->proc, 
					    runr_args->home,
					    runr_args->root);

    fprintf(stderr, "mount-MS_PRIVATE /\n");
    if (mount(NULL, "/", NULL, MS_REC | MS_SLAVE, NULL) == -1)
    {
        errExit("mount-MS_PRIVATE");
    }

    snprintf(old, sizeof(old), "%s/%s", runr_args->root, "old"); // old directory path
    fprintf(stderr, "old dir '%s'\n", old);
    if (stat(old, &st) == -1)
    {
        if (mkdir(old, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
        {
            errExit("old dir");
        }
    }
    // ensure that 'new_root' is a mount point
    fprintf(stderr, "mount-MS_BIND %s\n", runr_args->root);
    if (mount(runr_args->root, runr_args->root, NULL, MS_BIND, NULL) == -1)
    {
        errExit("mount-MS_BIND");
    }
    fprintf(stderr, "pivot root to %s\n", runr_args->root);
    if(pivot_root(runr_args->root, old) == -1)
    {
        errExit("pivot root");
    }
    if (chdir("/") == -1) 
    {
        perror("error chdir to new_root");
    }

    /*
    if(umount2("/old", MNT_FORCE) == -1)
    {
        errExit("umount");
    }*/
    fprintf(stderr, "mount proc\n");
    if (mkdir("/proc", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        errExit("mkdir /proc");
    }
    if (mount("proc", "/proc", "proc", 0, NULL) == -1)
    {
        errExit("mount /proc");
    }

    //umount_all();

    // you get a tmpfs
    /*
    if (mount("tmpfs", 
              runr_args->home, 
              "tmpfs",
              0,
              "size=100M,mode=0755"
        ) == -1)
    {
            perror("mount");
            return EXIT_FAILURE;
    }*/    

    // set persmissions and chdir into it
    fprintf(stderr, "chown home\n");
    chown(runr_args->home,  
	  runr_args->id, 
	  runr_args->gid);

    
    fprintf(stderr, "cd '%s' (home)\n", runr_args->home);
    if(chdir(runr_args->home) == -1)
    {
        errExit("cd home");
    }

    // add CAP_NET_RAW and CAP_NET_BIND_SERVICE

    fprintf(stderr, "drop groups\n");
    // remove all groups
    if(setgroups(0, NULL) < 0) {
         errExit("can't remove groups. need sudo."); 
         return -1; 
    }; 
    // change the uid and gid
    if(setresgid(runr_args->gid,
                 runr_args->gid, 
                 runr_args->gid) < 0) { 
         errExit("can't setresgid. need sudo.");
         return -1; 
    }; 
    if(setresuid(runr_args->id, 
                 runr_args->id, 
                 runr_args->id) < 0) {
         errExit("can't setresuid. need sudo."); 
         return -1; 
    };

    //fprintf(stderr, "exec %s\n", runr_args->proc);
    //char* exec_args[] = {runr_args->proc, 0};
    //execve(exec_args[0], exec_args, envp);     // Execute shell
    char* exec_args[] = {"/bin/toybox", "sh", 0};
    fprintf(stderr, "exec %s\n", exec_args[0]);
    execve(exec_args[0], exec_args, envp);     // Execute shell
}

pid_t
runr_start(t_runr_args *arg)
{
    fprintf(stderr, "runr name=%s id=%d gid=%d shell='%s' home='%s'\n", 
                                                     arg->name,
                                                     arg->id,
		    				     arg->gid,
						     arg->proc,
						     arg->home);
    char template[] = "/tmp/locker.XXXXXX";
    struct stat st = {0};
    char *new_root = mkdtemp(template);
    char userhome[1024];

    if (new_root == NULL)
    {
        errExit("mkdtemp: error: Cannot create tmp directory");   
    }
    if(chmod(new_root, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
    {
        errExit("chmod tmp");   
    }
    fprintf(stderr, "temp dir %s\n", new_root);
    strncpy(arg->root, new_root, strlen(new_root)+1);

    if(chdir(new_root) == -1)
    {
        errExit("chdir new_root");	 
    }

    fprintf(stderr, "etc\n");
    if (mkdir("etc", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
    {
        errExit("etc dir");
    }

    fprintf(stderr, "group %s\n", arg->name);
    group_add2(arg->name, arg->gid);

    fprintf(stderr, "passwd\n");
    passwd_add("root", 0, 0, "/root");
    passwd_add(arg->name, arg->id, arg->gid, arg->home);


    // home dir
    fprintf(stderr, "home\n");
    if (stat("home", &st) == -1) // home must exist
    {
         if(mkdir("home", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
         {
             errExit("mkdir home");
         }
    }
    snprintf(userhome, sizeof(userhome), "home/%s", arg->name);
    fprintf(stderr, "userhome=%s\n", userhome);
    if(mkdir(userhome, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
    {
        errExit("mkdir userhome");
    }

    fprintf(stderr, "bin\n");
    if (stat("bin", &st) == -1) // home must exist
    {
         if(mkdir("bin", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
         {
             errExit("mkdir bin");
         }
    }
    copy_file("/app/toybox-x86_64", "bin/toybox");

    pid_t pid = -1;
    char *stack = mmap(NULL,
                       STACK_SIZE,
                       PROT_READ | PROT_WRITE,  // rw-
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, // private stack, not shared
                       -1,
                       0);
    if (stack == MAP_FAILED)
    {
        errExit("mmap");
        exit(1);
    }
    // need to fork for unshare
    // https://lwn.net/Articles/531419/
    if ((pid = clone(child_func,         // called routine
        	     stack + STACK_SIZE,      // stack top
        	     CLONE_NEWNS | CLONE_NEWPID | SIGCHLD,  //
        	     arg)) == -1)            // function argument
    {
        errExit("clone failed.");
        exit(1);
    }

    return pid; // TODO return rc
}
