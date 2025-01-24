#include "core/server.h"
#include "utils/config.h"
#include "utils/logging.h"
#include <pthread.h>

int main() {
    Config *config = config_load("../config/config.json");
    if (!config) {
        log_error("Failed to load config");
        return 1;
    }

    log_set_level(config->log_level);

    struct Server *server = server_create(config);
    if (!server) {
        log_error("Failed to create server");
        config_free(config);
        return 1;
    }

    if (!server_start(server)) {
        log_error("Failed to start server");
        server_destroy(server);
        config_free(config);
        return 1;
    }

    // Main loop
    while (1) {
        sleep(1);
    }

    server_stop(server);
    server_destroy(server);
    config_free(config);

    return 0;
}   