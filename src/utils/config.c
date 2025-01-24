#include "utils/config.h"
#include "utils/logging.h"
#include <stdlib.h>
#include <string.h>

Config *config_load(const char *filename) {
    json_error_t error;
    json_t *root = json_load_file(filename, 0, &error);
    if (!root) {
        log_error("Failed to load config file: %s", error.text);
        return NULL;
    }

    Config *config = malloc(sizeof(Config));
    if (!config) {
        log_error("Failed to allocate memory for config");
        json_decref(root);
        return NULL;
    }

    // Parse server config
    json_t *server = json_object_get(root, "server");
    if (server) {
        config->port = json_integer_value(json_object_get(server, "port"));
        config->route_directory = strdup(json_string_value(json_object_get(server, "route_directory")));
        config->routes_sources = strdup(json_string_value(json_object_get(server, "routes_sources")));
        const char *log_level_str = json_string_value(json_object_get(server, "log_level"));
        if (strcmp(log_level_str, "debug") == 0) config->log_level = LOG_LEVEL_DEBUG;
        else if (strcmp(log_level_str, "info") == 0) config->log_level = LOG_LEVEL_INFO;
        else if (strcmp(log_level_str, "warn") == 0) config->log_level = LOG_LEVEL_WARN;
        else if (strcmp(log_level_str, "error") == 0) config->log_level = LOG_LEVEL_ERROR;
    }

    // Parse routes config
    json_t *routes = json_object_get(root, "routes");
    if (routes) {
        config->max_routes = json_integer_value(json_object_get(routes, "max_routes"));
        config->hot_reload = json_boolean_value(json_object_get(routes, "hot_reload"));
    }

    json_decref(root);
    return config;
}

void config_free(Config *config) {
    if (config) {
        free(config->route_directory);
        free(config->routes_sources);
        free(config);
    }
}