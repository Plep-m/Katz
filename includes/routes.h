#ifndef ROUTES_H
#define ROUTES_H

#include <microhttpd.h>

#define MAX_ROUTES 100

struct Route {
    char *url;
    int (*get_handler)(struct MHD_Connection *);
    int (*post_handler)(struct MHD_Connection *, const char *);
    void *dl_handle;
};

#endif // ROUTES_H
