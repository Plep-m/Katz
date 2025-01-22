#include "routes.h"
#include "http.h"
#include <unistd.h>
#include <pthread.h>

#define PORT 8080

int main() {
    // Initialize routes and route count
    struct Route *routes = NULL;
    int route_count = 0;

    // Initialize the read-write lock for routes
    if (pthread_rwlock_init(&routes_rwlock, NULL) != 0) {
        fprintf(stderr, "Failed to initialize routes_rwlock\n");
        return 1;
    }

    // Load routes from configuration or other source
    load_routes(&routes, &route_count);

    // Check if routes were loaded successfully
    if (routes == NULL || route_count == 0) {
        fprintf(stderr, "No routes were loaded.\n");
        pthread_rwlock_destroy(&routes_rwlock);
        return 1;
    }

    // Create a container for routes and connection information
    struct RouteContainer route_container = { routes, route_count };

    // Start the HTTP server daemon
    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY | MHD_USE_INTERNAL_POLLING_THREAD, 
        PORT, NULL, NULL, &handle_request, &route_container, MHD_OPTION_END);

    if (daemon == NULL) {
        fprintf(stderr, "Failed to start HTTP server\n");
        free(routes);
        pthread_rwlock_destroy(&routes_rwlock);
        return 1;
    }

    printf("Server running on port %d\n", PORT);

    // Start a thread to monitor routes
    pthread_t monitor_thread;
    if (pthread_create(&monitor_thread, NULL, monitor_routes, &route_container) != 0) {
        fprintf(stderr, "Failed to start route monitor thread\n");
        MHD_stop_daemon(daemon);
        free(routes);
        pthread_rwlock_destroy(&routes_rwlock);
        return 1;
    }

    // Main server loop
    while (1) {
        sleep(1); // Keep the server running
    }

    // Cleanup and shutdown
    MHD_stop_daemon(daemon);

    // Free resources associated with routes
    for (int i = 0; i < route_count; i++) {
        if (routes[i].dl_handle) {
            dlclose(routes[i].dl_handle);
        }
        free(routes[i].url);
    }
    free(routes);

    // Destroy the read-write lock
    pthread_rwlock_destroy(&routes_rwlock);

    return 0;
}