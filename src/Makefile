.PHONY: clean all

all: potato

potato: main.c login2.c userlist.c
	gcc -Wall -Wdeprecated-declarations -g -Og -o potato main.c login2.c userlist.c -lssl -lcrypto

potato_sani: main.c login2.c user.h
	gcc -Wall -g -Og -lcrypto -o potato_sani main.c login2.c userlist.c -lssl -lcrypto -fsanitize=address

potato_afl:	
	afl-gcc -o potato_afl main.c login2.c user.h -lssl -lcrypto

clean:
	rm potato potato_sani potato_afl
