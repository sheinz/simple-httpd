#include "httpd.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>


Httpd httpd;


static char index_html[] = "\
<html><body><h1>Hello world</h1>\
<form action=\"upload\" method=\"post\" enctype=\"multipart/form-data\">\
Param1 <input type=\"text\" name=\"param_1\"><br>\
Param2 <input type=\"text\" name=\"param_2\"><br>\
Param3 <input type=\"text\" name=\"param_3\"><br>\
<input type=\"file\" name=\"firmware\" id=\"firmware\"><br>\
<input type=\"submit\" value=\"Upload\">\
</form></body></html>\r\n\r\n";


static void req_handler(Httpd *httpd, const char *url, MethodType method)
{
    switch (method) {
        case HTTP_GET:
            printf("Http get request for url %s received\n", url);
            if (!strcmp(url, "/")) {
                httpd_send_header(httpd, true);
                httpd_send_data(httpd, index_html, strlen(index_html));
            } else {
                httpd_send_header(httpd, false);
            }
            break;
        case HTTP_POST:
            printf("Http post request for url %s received\n", url);
            if (!strcmp(url, "/upload")) {
                httpd_send_header(httpd, true);
            } else {
                httpd_send_header(httpd, false);
            }
            httpd->user_data = 0;
            break;
        default:
            printf("Unknown method\n");
    }
}

static void data_handler(Httpd *httpd, const char *name, const void *data, 
        uint16_t len)
{
    if (!httpd->user_data) {
        httpd->user_data = fopen(name, "w");
    }

    if (len) {
        fwrite(data, 1, len, httpd->user_data);
    } else {
        fclose(httpd->user_data);
        httpd->user_data = 0;
    }
}

static void data_complete_handler(Httpd *httpd, bool result)
{
    if (result) {
        char page[] = "<html><body>\
<h2>Successfuly uploaded.</h2></body></html>";
        httpd_send_data(httpd, page, strlen(page));
    } else {
        char page[] = "<html><body>\
<h2>Fail to upload.</h2></body></html>";
        httpd_send_data(httpd, page, strlen(page));
    }
}

void keyboard_interrupt_handler(int sig)
{
    printf("Stopping\n");
    httpd_stop(&httpd);         
}

int main(int argc, char *argv[])
{
    signal(SIGINT, keyboard_interrupt_handler);

    httpd.req_handler = req_handler;
    httpd.data_handler = data_handler;
    httpd.data_complete_handler = data_complete_handler;

    httpd_init(&httpd);
    httpd_serve(&httpd, 8080);
    return 0;
}
