
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "user.h"
#include "func.h"

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

    if(login(input_username, input_password) != -1)
    {
        fprintf(stdout, "Authentication successful.\n");
    }
    else
    {
        fprintf(stderr, "Authentication failed.\n");
    }
}
