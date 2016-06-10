#ifndef __HTTHD_H__
#define __HTTHD_H__
/**
 * Tiny HTTP server implementation.
 * Process only one request at a time.
 *
 * Handle GET and POST requests.
 * For POST request handle only 'multipart/form-data'
 *
 *
 */

#include <stdint.h>
#include <stdbool.h>

typedef enum{
    HTTP_GET = 1,
    HTTP_POST,
} MethodType;

struct _Httpd;

typedef void (*HttpdReqCallback)(struct _Httpd *httpd, const char *url, 
        MethodType method);

typedef void (*HttpdDataCallback)(struct _Httpd *httpd, const char *name, 
        const void *data, uint16_t len);

typedef void (*HttpdDataCompleteCallback)(struct _Httpd *httpd, bool result);


typedef struct _Httpd
{
// public:
    /** 
     * Request handler is called when GET or POST request is received.
     */
    HttpdReqCallback req_handler;

    /**
     * Data handler is called when data for POST request is recevied.
     * 'name' will contain name of the data.
     * This callback will be called several times untill all data for the 'name'
     * is received. The final call will be done with len = 0 to indicate end
     * of data.
     */
    HttpdDataCallback data_handler;

    /**
     * Complete handler is called when no more data in the request.
     * 'result' is true if all data successfully received otherwise false.
     */
    HttpdDataCompleteCallback data_complete_handler;

    /**
     * Can be used to pass data to handlers (callbacks).
     */
    void *user_data;

// private:
    bool running;
    int client_fd;
} Httpd;


void httpd_init(Httpd *httpd);

void httpd_serve(Httpd *httpd, uint16_t port);
void httpd_stop(Httpd *httpd);

void httpd_send_header(Httpd *httpd, bool ok);
void httpd_send_data(Httpd *httpd, void *data, uint16_t len);




#endif // __HTTHD_H__
