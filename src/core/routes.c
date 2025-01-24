#include <sys/inotify.h>
#include <limits.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "core/routes.h"
#include "utils/logging.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + PATH_MAX))


void* monitor_routes(void* arg) {
    struct RouteContainer *route_container = (struct RouteContainer *)arg;
    if (!route_container || !route_container->config) {
        log_error("route_container or config is NULL in monitor_routes");
        return NULL;
    }

    Config *config = route_container->config;

    // Log the state of route_container
    log_debug("Monitoring route container with %d routes", route_container->route_count);
    for (int i = 0; i < route_container->route_count; i++) {
        log_debug("Route %d: URL = %s, dl_handle = %p", i, route_container->routes[i].url, route_container->routes[i].dl_handle);
    }

    int inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        log_error("Failed to initialize inotify: %s", strerror(errno));
        return NULL;
    }

    // Watch the routes_sources directory for .c file changes
    char *source_watch_dir = config->routes_sources;
    log_info("Monitoring source directory: %s", source_watch_dir);
    int source_watch_fd = inotify_add_watch(inotify_fd, source_watch_dir, IN_MODIFY | IN_CREATE | IN_DELETE);
    if (source_watch_fd < 0) {
        log_error("Failed to add inotify watch on source directory: %s", strerror(errno));
        close(inotify_fd);
        return NULL;
    }

    // Watch the route_directory for .so file changes
    char *route_watch_dir = config->route_directory;
    log_info("Monitoring route directory: %s", route_watch_dir);
    int route_watch_fd = inotify_add_watch(inotify_fd, route_watch_dir, IN_MODIFY | IN_CREATE | IN_DELETE);
    if (route_watch_fd < 0) {
        log_error("Failed to add inotify watch on route directory: %s", strerror(errno));
        close(inotify_fd);
        return NULL;
    }

    char buffer[BUF_LEN];
    while (1) {
        ssize_t len = read(inotify_fd, buffer, BUF_LEN);
        if (len < 0) {
            log_error("Failed to read inotify events: %s", strerror(errno));
            break;
        }

        for (char *ptr = buffer; ptr < buffer + len; ) {
            struct inotify_event *event = (struct inotify_event *)ptr;
            if (event->len) {
                if (event->wd == source_watch_fd && strstr(event->name, ".c")) {
                    // Detected change in a .c file in the source directory
                    log_info("Detected change in source file: %s", event->name);

                    // Trigger a make command
                    log_info("Triggering make...");
                    int result = system("make");
                    if (result != 0) {
                        log_error("Failed to execute make command");
                    } else {
                        log_info("Make command executed successfully");

                        // Extract the route name from the .c file name
                        char route_name[256];
                        extract_route_name(event->name, route_name);

                        // Construct the corresponding .so file name
                        char so_file_name[256];
                        snprintf(so_file_name, sizeof(so_file_name), "libroute_%s.so", route_name);

                        // Trigger reload_route for the corresponding .so file
                        log_info("Reloading route: %s", so_file_name);
                        reload_route(route_container, so_file_name, config);
                    }
                } else if (event->wd == route_watch_fd && strstr(event->name, ".so")) {
                    // Detected change in a .so file in the route directory
                    log_info("Detected change in route: %s", event->name);
                    reload_route(route_container, event->name, config);
                }
            }
            ptr += EVENT_SIZE + event->len;
        }
    }

    close(inotify_fd);
    log_info("Stopped monitoring directories: %s and %s", source_watch_dir, route_watch_dir);
    return NULL;
}


void reload_route(struct RouteContainer *route_container, const char *file_name, Config *config) {
    if (!route_container) {
        log_error("route_container is NULL");
        return;
    }

    pthread_rwlock_wrlock(&routes_rwlock);

    // Log the state of route_container
    log_debug("Route container has %d routes", route_container->route_count);
    for (int i = 0; i < route_container->route_count; i++) {
        log_debug("Route %d: URL = %s, dl_handle = %p", i, route_container->routes[i].url, route_container->routes[i].dl_handle);
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", config->route_directory, file_name);
    if (access(path, F_OK) != 0) {
        log_warn("Route file no longer exists: %s", file_name);
        pthread_rwlock_unlock(&routes_rwlock);
        return;
    }

    char route_name[256];
    extract_route_name(file_name, route_name);
    char route_url[256];
    snprintf(route_url, sizeof(route_url), "/%s", route_name);

    log_debug("Attempting to reload route: %s", route_url);

    // Unload the existing route
    for (int i = 0; i < route_container->route_count; i++) {
        if (route_container->routes[i].url == NULL) {
            log_error("Route URL is NULL at index %d", i);
            continue;
        }

        log_debug("Checking route: %s", route_container->routes[i].url);
        if (strcmp(route_container->routes[i].url, route_url) == 0) {
            log_info("Unloading existing route: %s", route_container->routes[i].url);

            // Free resources for the existing route
            if (route_container->routes[i].dl_handle) {
                log_debug("Closing dl_handle for route: %s", route_container->routes[i].url);
                dlclose(route_container->routes[i].dl_handle);
            }
            if (route_container->routes[i].url) {
                log_debug("Freeing URL for route: %s", route_container->routes[i].url);
                free(route_container->routes[i].url);
            }

            // Shift remaining routes to fill the gap
            for (int j = i; j < route_container->route_count - 1; j++) {
                route_container->routes[j] = route_container->routes[j + 1];
            }
            route_container->route_count--;
            log_info("Successfully unloaded route: %s", route_url);
            break;
        }
    }

    // Load the new route
    if (route_container->route_count < MAX_ROUTES) {
        struct Route new_route = {0};
        if (load_route(&new_route, path)) {
            route_container->routes[route_container->route_count] = new_route;
            route_container->route_count++;
            log_info("Successfully reloaded route: %s", file_name);
        } else {
            log_error("Failed to reload route: %s", file_name);
        }
    } else {
        log_warn("Route container is full, cannot reload: %s", file_name);
    }

    pthread_rwlock_unlock(&routes_rwlock);
}

void extract_route_name(const char *file_name, char *route_name) {
    const char *start = strstr(file_name, "libroute_");
    if (start) {
        start += strlen("libroute_");
        const char *end = strstr(start, ".so");
        if (end) {
            size_t len = end - start;
            strncpy(route_name, start, len);
            route_name[len] = '\0';
        }
    }
}

int load_route(struct Route *route, const char *path) {
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        log_error("Failed to load route: %s", dlerror());
        return 0;
    }

    char route_name[256];
    extract_route_name(path, route_name);

    char get_handler_name[256];
    snprintf(get_handler_name, sizeof(get_handler_name), "%s_get_handler", route_name);
    int (*get_handler)(struct MHD_Connection *) = (int (*)(struct MHD_Connection *))dlsym(handle, get_handler_name);

    char post_handler_name[256];
    snprintf(post_handler_name, sizeof(post_handler_name), "%s_post_handler", route_name);
    int (*post_handler)(struct MHD_Connection *, const char *) = (int (*)(struct MHD_Connection *, const char *))dlsym(handle, post_handler_name);

    char put_handler_name[256];
    snprintf(put_handler_name, sizeof(put_handler_name), "%s_put_handler", route_name);
    int (*put_handler)(struct MHD_Connection *, const char *) = (int (*)(struct MHD_Connection *, const char *))dlsym(handle, put_handler_name);

    char delete_handler_name[256];
    snprintf(delete_handler_name, sizeof(delete_handler_name), "%s_delete_handler", route_name);
    int (*delete_handler)(struct MHD_Connection *) = (int (*)(struct MHD_Connection *))dlsym(handle, delete_handler_name);

    char patch_handler_name[256];
    snprintf(patch_handler_name, sizeof(patch_handler_name), "%s_patch_handler", route_name);
    int (*patch_handler)(struct MHD_Connection *, const char *) = (int (*)(struct MHD_Connection *, const char *))dlsym(handle, patch_handler_name);

    if (!get_handler && !post_handler && !put_handler && !delete_handler && !patch_handler) {
        log_error("No valid handlers found for route: %s", route_name);
        dlclose(handle);
        return 0;
    }

    route->url = malloc(strlen(route_name) + 2);
    if (!route->url) {
        log_error("Failed to allocate memory for route URL: %s", route_name);
        dlclose(handle);
        return 0;
    }
    route->url[0] = '/';
    strcpy(route->url + 1, route_name);
    route->get_handler = get_handler;
    route->post_handler = post_handler;
    route->put_handler = put_handler;
    route->delete_handler = delete_handler;
    route->patch_handler = patch_handler;
    route->dl_handle = handle;

    log_info("Registered route: %s", route->url);
    return 1;
}

void load_routes(struct Route **routes, int *route_count, Config *config) {
    if (!config) {
        log_error("Config is NULL in load_routes");
        return;
    }

    DIR *dir = opendir(config->route_directory);
    struct dirent *entry;
    int idx = 0;

    if (dir == NULL) {
        log_error("Failed to open routes directory: %s", strerror(errno));
        return;
    }

    *routes = calloc(MAX_ROUTES, sizeof(struct Route));
    if (*routes == NULL) {
        log_error("Failed to allocate memory for routes");
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == 8 && strstr(entry->d_name, ".so") != NULL) {
            char path[256];
            snprintf(path, sizeof(path), "%s/%s", config->route_directory, entry->d_name);

            if (load_route(&(*routes)[idx], path)) {
                idx++;
            }
        }
    }

    closedir(dir);
    *route_count = idx;
    log_info("Loaded %d routes", *route_count);
}