#include "core/server.h"
#include "utils/config.h"
#include "utils/logging.h"
#include <stdio.h>

void test_server() {
    Config *config = config_load("config/config.json");
    if (!config) {
        log_error("Failed to load config");
        return;
    }

    struct Server *server = server_create(config);
    if (!server) {
        log_error("Failed to create server");
        config_free(config);
        return;
    }

    if (!server_start(server)) {
        log_error("Failed to start server");
        server_destroy(server);
        config_free(config);
        return;
    }

    sleep(5); // Let the server run for 5 seconds

    server_stop(server);
    server_destroy(server);
    config_free(config);
}

int main() {
    test_server();
    return 0;
}