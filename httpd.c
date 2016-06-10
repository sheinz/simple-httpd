#include "httpd.h"

#include <sys/socket.h>
#include <sys/types.h>

#ifdef HOST_BUILD
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define SERVER_STRING "Server: simple-httpd/0.0.1\r\n"

void httpd_init(Httpd *httpd)
{
    
}

uint16_t read_line(int fd, char *buff, uint16_t buff_size)
{
    uint8_t count = 0;
    while (count < buff_size - 1){
        if (recv(fd, buff, 1, 0) > 0) {
            if (*buff == '\n') {
                break;
            } else if (*buff != '\r') {
                count++;
                buff++;
            }
        } else {
            break;
        }
    }

    *buff = 0;
    return count;
}

// The buff will be null-terminated
uint16_t read_till(int fd, char *buff, uint16_t buff_size, char delim)
{
    uint8_t count = 0;
    while (count < buff_size - 1){
        if (recv(fd, buff, 1, 0) > 0) {
            if (*buff == '\n' || *buff == delim) {
                break;
            } else if (*buff != '\r') {
                count++;
                buff++;
            }
        } else {
            break;
        }
    }

    *buff = 0;
    return count;
}

/**
 * Read out all data until double '\r\n' is encountered
 */
static void read_all_headers(int fd)
{
    char ch;
    uint8_t match = 0;

    while (recv(fd, &ch, 1, 0) > 0) {
        /* printf("%c\n", ch); */
        if (ch != '\r' && ch != '\n') {
            match = 0;
            continue;
        }
        match++;
        if (ch == '\n') {
            if (match == 4) {
                break;
            }
        }
    }
}

/**
 * Method searches if boundary occurs in the buff starting from the buff start.
 * If it finds 'boundary' in the 'buff' it returns number of characters matched.
 *
 * For examlpe:
 * buff: "123123123456", boundary: "12312345689"
 * It will return 9 which means last 9 bytes in buff matches
 */
static inline uint8_t rematch(const char *buff, uint16_t len, char *boundary)
{
    uint8_t matched = 0;    
    uint8_t boundary_len = strlen(boundary);
    uint16_t i;
    uint8_t j;
    
    for (i = 0; i < len; i++) {
        for (j = 0; j < boundary_len; j++) {
            if (buff[i + j] == boundary[j]) {
                matched++;
            } else {
                matched = 0;
                break;
            }
            if (i + j +1 >= len) {
                return matched;
            }
        }
        matched = 0;
    }
    return matched;
}

/**
 * Read data from socket 'fd' until string in 'boundary' is encountered.
 * The data is written in 'buff'. The data is not null terminated.
 * The amount of read data is returned.
 * Function saves its state in 'matched' variable. It must be 0 on the first
 * run. If the boundary is encountered on the buffer edge the function will
 * store how much boundary it matched on the previous run.
 */
uint16_t read_till_boundary(int fd, char *buff, uint16_t buff_size, 
        uint8_t *matched, char *boundary)
{
    uint16_t data_len = 0;
    uint8_t data;

    if (*matched >= strlen(boundary)) {
        return 0;
    }

    if (*matched) {  // number of previously matched data
        memcpy(buff, boundary, *matched);
    }
    
    while (recv(fd, &data, 1, 0) > 0) {
        // unconditionally accumulate data
        buff[data_len + *matched] = data;

        if (data == boundary[*matched]) {
            (*matched)++;
        } else {
            data_len++;
            if (*matched)
            {
                uint8_t new_match = rematch(&buff[data_len], *matched, boundary);
                data_len += (*matched) - new_match;
                *matched = new_match;
            }
        }

        if (*matched >= strlen(boundary)) {
            // complete match
            break;
        }

        if (data_len + *matched >= buff_size) {
            // can't handle more data
            break;
        }
    }

    return data_len; 
}

static inline bool extract_boundary(Httpd *httpd, char *buff, 
        uint16_t buff_size, char *boundary, uint8_t boundary_size)
{
    while (read_till(httpd->client_fd, buff, buff_size, ' ')) {
        if (!strcmp(buff, "Content-Type:")) {
            read_till(httpd->client_fd, buff, buff_size, ' ');
            if (!strcmp(buff, "multipart/form-data;")) {
                read_till(httpd->client_fd, buff, buff_size, '=');
                strcpy(boundary, "\r\n--");  // new line and extra two dashes
                read_till(httpd->client_fd, boundary + strlen(boundary), 
                        boundary_size - strlen(boundary), '\n');
            }
            break;
        }
    }
    if (strlen(boundary) == 0) {
        printf("Boundary not found\n");
        return false;
    } 

    return true;
}

static inline bool process_post_data(Httpd *httpd, 
        char *buff, uint16_t buff_size)
{
    char boundary[64] = {0};
    char name[32];

    if (!extract_boundary(httpd, buff, buff_size, boundary, sizeof(boundary))) {
        return false;
    }

    uint8_t state = 0;
    // discard all headers till boundary
    while (read_till_boundary(httpd->client_fd, buff, buff_size, 
                &state, boundary)) {
    }

    read_till(httpd->client_fd, buff, buff_size, '\n');  // discard \r\n
    while (true) {
        read_till(httpd->client_fd, buff, buff_size, ' ');
        if (strcmp(buff, "Content-Disposition:")) {
            printf("Content-Disposition missing\n");
            printf("%s\n", buff);
            return false;
        }
        read_till(httpd->client_fd, buff, buff_size, ' '); // discard "form-data;"
        read_till(httpd->client_fd, buff, buff_size, '"');
        if (strcmp(buff, "name=")) {
            printf("name= missing\n");
            return false;
        }
        read_till(httpd->client_fd, buff, buff_size, '"');
        strncpy(name, buff, sizeof(name));
        read_all_headers(httpd->client_fd);  // discard all till data start
        state = 0;
        uint16_t data_len;
        while ((data_len = read_till_boundary(httpd->client_fd, buff, 
                        buff_size, &state, boundary)) != 0) {
            httpd->data_handler(httpd, name, buff, data_len);
        }
        httpd->data_handler(httpd, name, buff, 0);  // indicate data complete
        read_till(httpd->client_fd, buff, buff_size, '\n');
        if (!strcmp(buff, "--")) {
            return true;
        }
    }
    return false;
}

void httpd_accept(Httpd *httpd, int client_fd)
{
    char buff[128];

    httpd->client_fd = client_fd;

    if (!read_till(client_fd, buff, sizeof(buff), ' ')) {
        printf("No data\n");
        return;
    }

    if (!strcmp(buff, "GET")) {
        if (read_till(client_fd, buff, sizeof(buff), ' ')) {
            read_all_headers(client_fd);
            httpd->req_handler(httpd, buff, HTTP_GET);
        }
    } else if (!strcmp(buff, "POST")) {
        if (read_till(client_fd, buff, sizeof(buff), ' ')) {
            httpd->req_handler(httpd, buff, HTTP_POST);
            bool result = process_post_data(httpd, buff, sizeof(buff));
            httpd->data_complete_handler(httpd, result);
        }
    } else {
        printf("Method %s not supported\n", buff);
    }

    close(client_fd);
}

/**
 * Return server socket descriptor.
 */
static inline int httpd_listen(uint16_t port)
{
    int server_fd = -1; 
    struct sockaddr_in name;

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("failed to create a socket\n");
        return -1;
    }

    // just for testing
    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        printf("failed to set socket option\n");
        return -1;
    }

    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_fd, (struct sockaddr *)&name, sizeof(name)) == -1) {
        printf("failed to bind address\n");
        return -1;
    }
    
    if (listen(server_fd, 1) == -1) {
        printf("failed to listen\n");
        return -1;
    }

    printf("Serving on port %d ...\n", port);
    return server_fd;
}

void httpd_serve(Httpd *httpd, uint16_t port)
{
    int server_fd = -1; 
    int client_fd = -1;
    struct sockaddr_in name;
    socklen_t name_len = sizeof(name);

    httpd->running = true;

    server_fd = httpd_listen(port);
    if (server_fd == -1) {
        return;
    }

    while (httpd->running) {
        printf("waiting for connection\n");
        client_fd = accept(server_fd, (struct sockaddr *)&name, &name_len);
        if (client_fd == -1) {
            printf("accept failed\n");
            return;
        }
        printf("Connection from %s accepted\n", inet_ntoa(name.sin_addr));
        httpd_accept(httpd, client_fd);
    }
    close(server_fd);
    printf("exit\n");
}

void httpd_stop(Httpd *httpd) 
{
    httpd->running = false;
}

void httpd_send_header(Httpd *httpd, bool ok)
{
    char buff[128];

    if (ok) {
        strcpy(buff, "HTTP/1.0 200 OK\r\n");
    } else {
        strcpy(buff, "HTTP/1.0 404 Not Found\r\n");
    }
    send(httpd->client_fd, buff, strlen(buff), 0);

    strcpy(buff, SERVER_STRING);
    send(httpd->client_fd, buff, strlen(buff), 0);

    sprintf(buff, "Content-Type: text/html\r\n");
    send(httpd->client_fd, buff, strlen(buff), 0);

    strcpy(buff, "\r\n");
    send(httpd->client_fd, buff, strlen(buff), 0);
}

void httpd_send_data(Httpd *httpd, void *data, uint16_t len)
{
    /* printf("data, len=%d sent\n", len); */
    send(httpd->client_fd, data, len, 0);
}
