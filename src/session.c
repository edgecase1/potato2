
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>

#include "session.h"
#include "logger.h"
#define MAX_SESSIONS 100

t_session* sessions[MAX_SESSIONS];

pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;
int session_count = 0;

t_session* get_tmp_session()
{
    return (t_session *)malloc(sizeof(t_session));
}

void generate_session_id(char *buffer) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < LEN_SESSION - 1; ++i) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[LEN_SESSION - 1] = '\0';
}

char* create_session(t_session* tmp_session)
{
    char session_id[LEN_SESSION];

    generate_session_id(session_id);
    LOG("generate session");

    pthread_mutex_lock(&session_mutex);
    if (session_count < MAX_SESSIONS) {
	session_count++;
	sessions[session_count] = tmp_session;
        strncpy(sessions[session_count]->session_id, session_id, LEN_SESSION-1);
    }
    LOG("sessioni ready.");
    pthread_mutex_unlock(&session_mutex);
    return sessions[session_count]->session_id;
}

t_session* get_session_by_id(char* session_id)
{
    for (int i = 0; i < MAX_SESSIONS; i++) {
	if(sessions[i] == NULL || sessions[i]->session_id == NULL) continue;
        if (strncmp(sessions[i]->session_id, session_id, LEN_SESSION) == 0) {
            return sessions[i]; // TOCTOU
        }
    }
}

void destroy_session(t_session* session)
{
}

int is_valid_session(const char *session_id) {
    return get_session_by_id(session_id) != NULL;
}
