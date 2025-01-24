#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "core/http_helper.h"

int hello_get_handler(struct MHD_Connection *connection) {
    const char *response_str = "Hello World!\n";
    return http_send_response(connection, MHD_HTTP_OK, response_str);
}

int hello_post_handler(struct MHD_Connection *connection, const char *json_body) {
    const char *error_response = "No body in POST request!\n";
    if (json_body == NULL) {
        return http_send_response(connection, MHD_HTTP_BAD_REQUEST, error_response);
    }
    
    char response_str[256];
    snprintf(response_str, sizeof(response_str), "Received JSON body: %s\n", json_body);

    return http_send_response(connection, MHD_HTTP_OK, response_str);
}
