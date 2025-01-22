#include "http_helper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum MHD_Result http_send_response(struct MHD_Connection *connection, int status_code, const char *response_str) {
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_str), (void *)response_str, MHD_RESPMEM_PERSISTENT);

    if (!response) {
        fprintf(stderr, "Failed to create response\n");
        return MHD_NO;
    }

    enum MHD_Result result = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return result;
}
