#ifndef KATZ_H
#define KATZ_H

#include <microhttpd.h>
#include <stdbool.h>

// Public API functions
int katz_init(const char *config_path);
void katz_cleanup();
void katz_start();
void katz_stop();

enum MHD_Result katz_send_response(struct MHD_Connection *connection, int status_code, const char *response_str);

#endif // KATZ_H