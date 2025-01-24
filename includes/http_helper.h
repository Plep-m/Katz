#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H

#include <microhttpd.h>

// Function to create and queue a response
enum MHD_Result http_send_response(struct MHD_Connection *connection, int status_code, const char *response_str);

enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection, 
                               const char *url, const char *method, 
                               const char *version, const char *upload_data,
                               size_t *upload_data_size, void **con_cls);
                               
#endif // HTTP_HELPER_H