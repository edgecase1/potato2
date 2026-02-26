.PHONY: clean all

all: potato

potato: func.h main.c runr.c sock.c userlist.c func.c login2.c runr.h sock.h user.h userlist.h http.h http_server.c logger.h console_server.c console_server.h session.c session.h
	gcc -g -Wno-deprecated-declarations -Wno-implicit-function-declaration -Wno-unused-result -Og main.c login2.c userlist.c sock.c func.c runr.c http_server.c console_server.c session.c -lssl -lcrypto -o ../potato 

clean:
	rm potato potato_sani potato_afl
