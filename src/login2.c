
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>

#include "user.h"

char *str2md5(const char *str, int length) {
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(90); // md5 plus null terminator for snprintf

    MD5_Init(&c);
    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }
    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2+1, "%02x", (unsigned int)digest[n]);
    }

    return out;
} 

t_user* new_user(char* username, char* password)
{
	char* hash;
	t_user* user;

    user = (t_user *) malloc(sizeof(t_user));
	strcpy(user->name, username);
    sprintf(user->home, "/home/%s", username);
    strcpy(user->shell, "/usr/bin/rbash");
    hash = str2md5(password, strlen(password));
    strncpy(user->password_hash, hash, 32);
    free(hash);

	return user;
}

int
check_password(t_user* user, char* password)
{
    return (0 == strncmp(
                        user->password_hash, 
		        str2md5(password, strlen(password)), 
			32)); // md5 length
}

void
print_user(t_user* user)
{
     printf("user(name='%s' id=%d gid=%d home='%s' shell='%s')\n", 
		     user->name, 
		     user->id, 
	             user->gid, 
		     user->home, 
		     user->shell);
}

t_user* parse_userlist_list(char* line)
{
	char* token;
	int column = 0;
	t_user* parsed_user = (t_user *)malloc(sizeof(t_user));
	
	token = strtok(line, ":");
	while(token != NULL)
	{
	    switch(column)
	    {
	   	    case 0: // name
	   		strcpy(parsed_user->name, token);
	   		break;
	   	    case 1: // hash
	   		strncpy(parsed_user->password_hash, token, 32);
	   		break;
	   	    case 2: // id
	   		parsed_user->id = atoi(token);
	   		parsed_user->gid = atoi(token);
	   	        break;
	   	    // TODO gid
	   	    case 3: // home
	   		strcpy(parsed_user->home, token);
	   		break;
	   	    case 4: // shell
	   		strcpy(parsed_user->shell, token);
	   		break;
	   	    default:
	   		free(parsed_user);
	   		return NULL;
	   }
	   token = strtok(NULL, ":");
	   column++;
	}
	return parsed_user;
}

/*
int main(int argc, char* argv)
{
	char line[] = "peter:827ccb0eea8a706c4c34a16891f84e7b:1000:/home/peter:/bin/bash";
	t_user* user = parse_userlist_line(line);
	print_user(user);
	free(user);
	return 0;
}
*/
