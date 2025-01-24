#ifndef SERVER_H
#define SERVER_H

#include <microhttpd.h>
#include "routes.h"
#include "utils/config.h"

struct Server {
    struct MHD_Daemon *daemon;
    Config *config;
    struct RouteContainer route_container;
};

struct Server *server_create(Config *config);
void server_destroy(struct Server *server);
int server_start(struct Server *server);
void server_stop(struct Server *server);

#endif // SERVER_H