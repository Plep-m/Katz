#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int hello_get_handler(struct MHD_Connection *connection) {
    const char *response_str = "Hello World!\n";
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_str), (void *)response_str, MHD_RESPMEM_PERSISTENT);

    if (!response) {
        fprintf(stderr, "Failed to create response\n");
        return MHD_NO;
    }

    enum MHD_Result result = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return result;
}

int hello_post_handler(struct MHD_Connection *connection, const char *json_body) {
    if (json_body == NULL) {
        const char *response_str = "No body in POST request!\n";
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), (void *)response_str, MHD_RESPMEM_PERSISTENT);
        enum MHD_Result result = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return result;
    }
    
    char response_str[256];
    snprintf(response_str, sizeof(response_str), "Received JSON body: %s\n", json_body);

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_str), (void *)response_str, MHD_RESPMEM_PERSISTENT);
    enum MHD_Result result = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return result;
}