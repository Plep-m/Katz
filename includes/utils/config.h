#ifndef CONFIG_H
#define CONFIG_H

#include <jansson.h>

#include "utils/logging.h"

typedef struct {
    int port;
    char *route_directory;
    char *routes_sources;
    LogLevel log_level;
    int max_routes;
    int hot_reload;
} Config;

Config *config_load(const char *filename);
void config_free(Config *config);

#endif // CONFIG_H