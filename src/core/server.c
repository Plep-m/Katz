#include "core/server.h"
#include "core/http_helper.h"
#include "utils/logging.h"
#include <stdlib.h>

struct Server *server_create(Config *config) {
    struct Server *server = malloc(sizeof(struct Server));
    if (!server) {
        log_error("Failed to allocate memory for server");
        return NULL;
    }

    server->config = config;
    server->daemon = NULL;
    server->route_container.routes = NULL;
    server->route_container.route_count = 0;

    return server;
}

void server_destroy(struct Server *server) {
    if (server) {
        if (server->daemon) MHD_stop_daemon(server->daemon);
        free(server);
    }
}

int server_start(struct Server *server) {
    if (!server || !server->config) {
        log_error("Invalid server or config");
        return 0;
    }

    server->daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY | MHD_USE_INTERNAL_POLLING_THREAD,
        server->config->port, NULL, NULL, &handle_request, &server->route_container, MHD_OPTION_END);

    if (!server->daemon) {
        log_error("Failed to start HTTP server");
        return 0;
    }

    log_info("Server started on port %d", server->config->port);
    return 1;
}

void server_stop(struct Server *server) {
    if (server && server->daemon) {
        MHD_stop_daemon(server->daemon);
        server->daemon = NULL;
        log_info("Server stopped");
    }
}