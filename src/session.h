#include "user.h"
#define LEN_SESSION 16

struct _session
{
     t_user* logged_in_user;
     time_t start_time;
     char session_id[LEN_SESSION];
} typedef t_session;

char* create_session(); // returns session id
void destroy_session(t_session* session);
t_session* get_session_by_id(char* id);
int is_valid_session(const char *session_id);
t_session* get_tmp_session();
