#include "core/server.h"
#include "core/routes.h"
#include "utils/config.h"
#include "utils/logging.h"
#include <pthread.h>
#include <unistd.h>

int main() {
    // Load configuration
    Config *config = config_load("../config/config.json");
    if (!config) {
        log_error("Failed to load config");
        return 1;
    }

    // Set log level from config
    log_set_level(config->log_level);

    // Initialize routes
    struct Route *routes = NULL;
    int route_count = 0;

    // Initialize the read-write lock for routes
    if (pthread_rwlock_init(&routes_rwlock, NULL) != 0) {
        log_error("Failed to initialize routes_rwlock");
        config_free(config);
        return 1;
    }

    // Load routes from the configured directory
    load_routes(&routes, &route_count);
    if (routes == NULL || route_count == 0) {
        log_error("No routes were loaded");
        pthread_rwlock_destroy(&routes_rwlock);
        config_free(config);
        return 1;
    }

    // Create the route container
    struct RouteContainer route_container = { routes, route_count };

    // Create server
    struct Server *server = server_create(config);
    if (!server) {
        log_error("Failed to create server");
        pthread_rwlock_destroy(&routes_rwlock);
        config_free(config);
        return 1;
    }

    // Assign the route container to the server
    server->route_container = route_container;

    // Start server
    if (!server_start(server)) {
        log_error("Failed to start server");
        server_destroy(server);
        pthread_rwlock_destroy(&routes_rwlock);
        config_free(config);
        return 1;
    }

    // Start the route monitoring thread
    pthread_t monitor_thread;
    if (pthread_create(&monitor_thread, NULL, monitor_routes, &route_container) != 0) {
        log_error("Failed to start route monitor thread");
        server_stop(server);
        server_destroy(server);
        pthread_rwlock_destroy(&routes_rwlock);
        config_free(config);
        return 1;
    }

    log_info("Server started on port %d", config->port);
    log_info("Route monitoring started");

    // Main loop
    while (1) {
        sleep(1); // Keep the main thread alive
    }

    // Cleanup (this will never be reached in the current loop)
    server_stop(server);
    server_destroy(server);
    pthread_rwlock_destroy(&routes_rwlock);
    config_free(config);

    return 0;
}