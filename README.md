# Potato2
This is a VULNERABLE test program to understand linux, container technologies, stack and heap corruption as well as fuzzing.
It provides a secure shell for authenticated users via:
- console interface (stdin/stdout), 
- socket server (tcp/222) and 
- http server (tcp/80).

This project also provides a multi-stage Dockerfile to build and create a container image.
The Docker container exposes the necessary interfaces for testing.
For debugging and attack prototyping the program may be built natively (to attach gdb without namespace mangling).

The use case is to 1) authenticate via commandline or web server and to 2) invoke the secure shell.
Console users drop into a container environment (the web server doesn't support this feature yet). 

The users are stored in the `userlist` file using a `passwd`-like structure along with a password hash.
The default user is `peter` using password `12345`.
The system works session-based: each authenticated user is assigned a session-ID; the web server accepts session-IDs via a Cookie value `session`.

## Build the Container Image

Start the build process via `./build.sh` (the process is tested on Linux x86\_64).
This starts a two-stage build-process for 1) building `potato2` using gcc and 2) creating the container image.
```
└─$ ./build.sh                 
[+] Building 51.3s (12/18)                                                                    docker:default
 => [internal] load build definition from Dockerfile                                                    0.1s
 => => transferring dockerfile: 816B                                                                    0.0s 
 => [internal] load metadata for docker.io/library/ubuntu:24.04                                         2.7s 
 => [internal] load metadata for docker.io/library/gcc:14                                               1.6s
 => [internal] load .dockerignore                                                                       0.0s
 => => transferring context: 2B                                                                         0.0s 
 => [builder 1/5] FROM docker.io/library/gcc:14@sha256:5004ba96657738ce0497542c98fdc3422fdd2b741b4e8d9  0.0s 
 => [stage-1 1/8] FROM docker.io/library/ubuntu:24.04@sha256:d1e2e92c075e5ca139d51a140fff46f84315c0fd  22.7s 
 => => resolve docker.io/library/ubuntu:24.04@sha256:d1e2e92c075e5ca139d51a140fff46f84315c0fdce203eab2  1.1s
 => => sha256:bbdabce66f1b7dde0c081a6b4536d837cd81dd322dd8c99edd68860baf3b2db3 2.29kB / 2.29kB          0.0s
...
 => [stage-1 8/8] COPY toybox-x86_64 /app                                                               0.1s 
 => exporting to image                                                                                  1.2s 
 => => exporting layers                                                                                 1.1s
 => => writing image sha256:93afdde168695f008ae93736ef25566fb98da79ac9418bb3896027641a14e491            0.0s
 => => naming to docker.io/library/potato2   ```
```
The resulting image is `docker.io/library/potato2` or `potato2 for short.

## Run Container Image

After the build-process of the image we can start the server via `./run.sh http`.
This should start the server using the host's network and pid namespace, to naturally integrate into the system, but without tainting the underlying Linux system with dependencies.

```
WARNING: Published ports are discarded when using host network mode
starting up (pid 2409491)
reading file userlist
[session.c:51] session ready.
handle_client
cmd> Server running on port 80...
```
The other modes are `./run.sh server` and `./run.sh console`.

The web server should now be reachable at `http://localhost/login` and via console.

## Manual Building

The program can be build in the `src` folder with `make`. This creates the `potato` program in the `src` directory.

The program can be started in the root directory of the Git repository via `sudo ./src/potato http`.
It parses the provided [userlist](userlist) file.

Make sure that the program runs as root or has the `CAP_SYS_ADMIN` capability to change namespaces of the child processes as well as mounting filesystems. 
This can be achieved with `sudo`.

## Functions

```
cmd> help
> register              create a temporary user (no automatically persistet)
> login                 authenticate a user
> logout                logout and destroy the current session

functions as authenticated user
> whoami                show user id
> changepw              change the password
> changename            change the username
> shell                 start the shell in the user profile

system functions:
> list          show all registered users
> delete        delete a user
> read          parse the 'userlist' file
> write         write out the 'userlist' file
> purge         empty the userlist
> debug         show the userlist data structure
> exit          close the program/connection
```
### Console Login
There is a build-in user (`peter`) with a predefined password. You can also create a new user using the `register` function.
```
cmd> login
Welcome!
username: peter
password: 12345
searching for user ...
checking password ...
You are authorized.
```
After you logged in you have different functions available (use `help`).

### Web Server API

The web server accessible via `http://localhost/` provides the following endpoints:
- `/login` the HTML login mask for username and password.
- `/run` the HTML command interface for the web shell.
- `/api` backend functions

![login](/docs/images/login.png)
![run command](/docs/images/run.png)

### Network mode
The same program code runs in the code and opens port 222. Any socket client can connect the the application.
```
nc -v 127.0.0.1 222
```
## Hints
There is a Python program `pwn_potato2.py` boiler-plate for pwntools. 
The necessary requirements are `pip install pwntools`.
I recommend to use Virtualenv to manage the Python-pip dependencies.

```
virtualenv venv
. ./venv/bin/activate
pip install pwntools requests
git clone https://github.com/hugsy/gef.git
echo source `pwd`/gef/gef.py >> ~/.gdbinit
```

There is `http_client.py` that tests the web server functions.


