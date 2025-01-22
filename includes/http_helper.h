#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H

#include <microhttpd.h>

// Function to create and queue a response
enum MHD_Result http_send_response(struct MHD_Connection *connection, int status_code, const char *response_str);

#endif // HTTP_HELPER_H