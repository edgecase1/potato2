#include "user.h"
#define LEN_SESSION 16

struct _session
{
     t_user* logged_in_user;
     time_t start_time;
     char session_id[LEN_SESSION];
} typedef t_session;
