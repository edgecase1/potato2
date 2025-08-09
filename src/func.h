
#include "session.h"

//int is_privileged(t_user* user);
void shell(t_user* user);
int create_user(char* input_username, char* input_password);
int login(t_session* session, char* input_username, char* input_password);
void logout(t_session* session);
void change_name(t_user* user, char* input_username);
void change_password(t_user* user, char* input_password);
void whoami(t_user* user);
void not_implemented();
