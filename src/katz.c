#include "public/katz.h"
#include "core/server.h"
#include "core/routes.h"
#include "utils/config.h"
#include "utils/logging.h"
#include <pthread.h>
#include <unistd.h>

static struct Server *server = NULL;
static Config *config = NULL;
pthread_rwlock_t routes_rwlock;

int katz_init(const char *config_path) {
    config = config_load(config_path);
    if (!config) {
        log_error("Failed to load config");
        return 0;
    }
    log_set_level(config->log_level);

    // Initialize routes_rwlock
    if (pthread_rwlock_init(&routes_rwlock, NULL) != 0) {
        log_error("Failed to initialize routes_rwlock");
        config_free(config);
        return 0;
    }

    struct Route *routes = NULL;
    int route_count = 0;
    load_routes(&routes, &route_count, config);

    // Initialize route_container
    struct RouteContainer route_container = { routes, route_count, config };

    // Log the state of route_container
    log_debug("Initialized route container with %d routes", route_container.route_count);
    for (int i = 0; i < route_container.route_count; i++) {
        log_debug("Route %d: URL = %s, dl_handle = %p", i, route_container.routes[i].url, route_container.routes[i].dl_handle);
    }

    server = server_create(config);
    if (!server) {
        log_error("Failed to create server");
        pthread_rwlock_destroy(&routes_rwlock);
        config_free(config);
        return 0;
    }
    server->route_container = route_container;

    // Start the route monitoring thread if hot_reload is enabled
    if (config->hot_reload) {
        pthread_t monitor_thread;
        if (pthread_create(&monitor_thread, NULL, monitor_routes, &server->route_container) != 0) {
            log_error("Failed to start route monitor thread");
            server_destroy(server);
            pthread_rwlock_destroy(&routes_rwlock);
            config_free(config);
            return 0;
        }
        log_info("Route monitoring started");
    }

    return 1;
}

void katz_cleanup() {
    if (server) {
        server_stop(server);
        server_destroy(server);
    }
    if (config) {
        config_free(config);
    }
    pthread_rwlock_destroy(&routes_rwlock);
}

void katz_start() {
    if (server && !server_start(server)) {
        log_error("Failed to start server");
    }
}

void katz_stop() {
    if (server) {
        server_stop(server);
    }
}

enum MHD_Result katz_send_response(struct MHD_Connection *connection, int status_code, const char *response_str) {
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_str), (void *)response_str, MHD_RESPMEM_PERSISTENT);
    if (!response) {
        return MHD_NO;
    }
    enum MHD_Result result = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return result;
}