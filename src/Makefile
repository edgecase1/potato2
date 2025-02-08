.PHONY: clean all

all: potato

potato: main.c login2.c userlist.c sock.c
	gcc -Wall -Wdeprecated-declarations -g -Og -o ../potato main.c login2.c userlist.c sock.c -lssl -lcrypto

clean:
	rm potato potato_sani potato_afl
