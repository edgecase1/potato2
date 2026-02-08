
// http_server.c
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

#define PORT 80
#define BUF_SIZE 4096
#define LEN_SESSION 16

#include "func.h"
#include "logger.h"

char hex_to_char(char high, char low) {
    int hi = (isdigit(high)) ? high - '0' : tolower(high) - 'a' + 10;
    int lo = (isdigit(low)) ? low - '0' : tolower(low) - 'a' + 10;
    return (hi << 4) | lo;
}

// In-place decoding
void url_decode(char *src) {
    char *dst = src;
    while (*src) {
        if (*src == '%' && isxdigit(src[1]) && isxdigit(src[2])) {
            *dst++ = hex_to_char(src[1], src[2]);
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Parse value from form data: key1=val1&key2=val2
int get_form_value(const char *body, const char *key, char* value) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "%s=", key);
    char *start = strstr(body, pattern);
    if (!start) return -1;
    start += strlen(pattern);
    char *end = strchr(start, '&'); // search until the next value
    if (!end) end = strchr(start, '\0'); // or the end
    int len = end - start;
    if (len > 255) len = 255;
    strncpy(value, start, len); // copy the value after '='
    value[len] = '\0';
    url_decode(value);
    return 0;
}

char* read_file(const char *filename, size_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    if (size) *size = len;
    return buf;
}


void handle_login(int client_sock, const char *body) {
    char username[32];
    char password[32]; 
    get_form_value(body, "username", username);
    get_form_value(body, "password", password);
    char* session_id;

    t_session* tmp_session = get_tmp_session();
    LOG("login attempt");
    if (login(tmp_session, username, password) != -1) {
        // Accept any credentials
    	LOG("login via HTTP successful");
        session_id = create_session(tmp_session);
    	LOG("session active");

        char response[256];
        snprintf(response,
		 sizeof(response),
                 "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nSet-Cookie: session=%s\r\n\r\nauthentication successful", 
		 session_id);
	LOG("sending response");
        send(client_sock, response, strlen(response), 0);
    } else {
        char *resp = "HTTP/1.1 400 Bad Request\r\n\r\nAuth failed";
        send(client_sock, resp, strlen(resp), 0);
    }
}

void handle_run(int client_sock, const char *body) {
    char session_id[LEN_SESSION];
    get_form_value(body, "session_id", session_id);
    char command[255];
    get_form_value(body, "command", command);
    LOG("run command request");

    if (!command) {
        LOG("auth via session failed");
        char *resp = "HTTP/1.1 403 Forbidden\r\n\r\nInvalid session or missing command";
        send(client_sock, resp, strlen(resp), 0);
        return;
    }

    LOG("auth via session successful");
    char cmd_output[1024] = {0};
    FILE *fp = popen(command, "r");
    if (fp) {
        fread(cmd_output, 1, sizeof(cmd_output) - 1, fp);
        pclose(fp);
    }

    LOG("command finished creating response");
    char response[1500];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%s", cmd_output);
    send(client_sock, response, strlen(response), 0);
}

void *handle_http_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);

    char buffer[BUF_SIZE] = {0};
    read(client_sock, buffer, BUF_SIZE - 1);

    char method[8], path[128];
    sscanf(buffer, "%s %s", method, path);

    if(strcmp(method, "GET")==0)
    {
	size_t size = 0;
        char *file_contents = read_file("index.html", &size);
        send(client_sock, file_contents, strlen(file_contents), 0);
    }
    else if(strcmp(method, "POST")==0)
    {
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) body += 4;

        if (strcmp(path, "/api/login") == 0 && strcmp(method, "POST") == 0) {
            handle_login(client_sock, body);
        } else if (strcmp(path, "/api/run") == 0 && strcmp(method, "POST") == 0) {
            handle_run(client_sock, body);
        } else {
            char *resp = "HTTP/1.1 404 Not Found\r\n\r\n";
            send(client_sock, resp, strlen(resp), 0);
        }
    }
    else
    {
	LOG("error method");
    }

    LOG("Closing socket");

    close(client_sock);
    return NULL;
}

void *http_server(void *arg) {
    srand(time(NULL));
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    printf("Server running on port %d...\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;
        pthread_t t;
        pthread_create(&t, NULL, handle_http_client, pclient);
        pthread_detach(t);
    }
}
