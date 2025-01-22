#include "http.h"
#include <sys/inotify.h>
#include <limits.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + PATH_MAX))

pthread_rwlock_t routes_rwlock;

void log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fflush(stderr);
}

void* monitor_routes(void* arg) {
    struct RouteContainer *route_container = (struct RouteContainer *)arg;
    int inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        perror("inotify_init");
        return NULL;
    }

    char *watch_dir = "./routes";
    log_debug("Monitoring directory: %s\n", watch_dir);
    int watch_fd = inotify_add_watch(inotify_fd, watch_dir, IN_MODIFY | IN_CREATE | IN_DELETE);
    if (watch_fd < 0) {
        perror("inotify_add_watch");
        close(inotify_fd);
        return NULL;
    }

    char buffer[BUF_LEN];
    while (1) {
        ssize_t len = read(inotify_fd, buffer, BUF_LEN);
        if (len < 0) {
            perror("read");
            break;
        }

        for (char *ptr = buffer; ptr < buffer + len; ) {
            struct inotify_event *event = (struct inotify_event *)ptr;
            if (event->len && strstr(event->name, ".so")) {
                log_debug("[MONITOR] Detected change in route: %s\n", event->name);
                reload_route(route_container, event->name);
            }
            ptr += EVENT_SIZE + event->len;
        }
    }

    close(inotify_fd);
    return NULL;
}

void reload_route(struct RouteContainer *route_container, const char *file_name) {
    pthread_rwlock_wrlock(&routes_rwlock);

    char path[256];
    snprintf(path, sizeof(path), "./routes/%s", file_name);

    if (access(path, F_OK) != 0) {
        log_debug("Route file no longer exists: %s\n", file_name);
        pthread_rwlock_unlock(&routes_rwlock);
        return;
    }

    char route_name[256];
    extract_route_name(file_name, route_name);
    char route_url[256];
    snprintf(route_url, sizeof(route_url), "/%s", route_name);

    for (int i = 0; i < route_container->route_count; i++) {
        if (strcmp(route_container->routes[i].url, route_url) == 0) {
            log_debug("Unloading existing route: %s\n", route_container->routes[i].url);
            if (route_container->routes[i].dl_handle) {
                dlclose(route_container->routes[i].dl_handle);
                route_container->routes[i].dl_handle = NULL;
            }
            free(route_container->routes[i].url);
            memmove(&route_container->routes[i], &route_container->routes[i + 1],
                    (route_container->route_count - i - 1) * sizeof(struct Route));
            route_container->route_count--;
            break;
        }
    }

    if (route_container->route_count < MAX_ROUTES) {
        struct Route new_route = {0};
        if (load_route(&new_route, path)) {
            route_container->routes[route_container->route_count] = new_route;
            route_container->route_count++;
            log_debug("Successfully reloaded route: %s\n", file_name);
        } else {
            log_debug("Failed to reload route: %s\n", file_name);
        }
    } else {
        log_debug("Route container is full, cannot reload: %s\n", file_name);
    }

    pthread_rwlock_unlock(&routes_rwlock);
}

enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection, 
                               const char *url, const char *method, 
                               const char *version, const char *upload_data,
                               size_t *upload_data_size, void **con_cls) {
    struct RouteContainer *route_container = (struct RouteContainer *)cls;

    pthread_rwlock_rdlock(&routes_rwlock);

    if (route_container == NULL || route_container->routes == NULL) {
        log_debug("Routes array is NULL!\n");
        pthread_rwlock_unlock(&routes_rwlock);
        return MHD_NO;
    }

    if (*con_cls == NULL) {
        struct UploadData *data = calloc(1, sizeof(struct UploadData));
        if (!data) {
            log_debug("Failed to allocate memory for UploadData\n");
            pthread_rwlock_unlock(&routes_rwlock);
            return MHD_NO;
        }
        *con_cls = data;
        pthread_rwlock_unlock(&routes_rwlock);
        return MHD_YES;
    }

    struct UploadData *upload = (struct UploadData *)*con_cls;

    if (strcmp(method, "POST") == 0) {
        if (*upload_data_size > 0) {
            char *new_buffer = realloc(upload->buffer, upload->size + *upload_data_size + 1);
            if (!new_buffer) {
                log_debug("Failed to reallocate memory for upload data\n");
                pthread_rwlock_unlock(&routes_rwlock);
                return MHD_NO;
            }

            upload->buffer = new_buffer;
            memcpy(upload->buffer + upload->size, upload_data, *upload_data_size);
            upload->size += *upload_data_size;
            upload->buffer[upload->size] = '\0';

            *upload_data_size = 0;
            pthread_rwlock_unlock(&routes_rwlock);
            return MHD_YES;
        }

        if (upload->buffer) {
            for (int i = 0; i < route_container->route_count; i++) {
                if (strcmp(route_container->routes[i].url, url) == 0 && route_container->routes[i].post_handler) {
                    enum MHD_Result result = route_container->routes[i].post_handler(connection, upload->buffer);
                    free(upload->buffer);
                    free(upload);
                    *con_cls = NULL;
                    pthread_rwlock_unlock(&routes_rwlock);
                    return result;
                }
            }
        }

        const char *response_str = "404 Not Found";
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), (void *)response_str, MHD_RESPMEM_PERSISTENT);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);

        free(upload->buffer);
        free(upload);
        *con_cls = NULL;
        pthread_rwlock_unlock(&routes_rwlock);
        return ret;
    }

    for (int i = 0; i < route_container->route_count; i++) {
        if (strcmp(route_container->routes[i].url, url) == 0 && route_container->routes[i].get_handler) {
            enum MHD_Result result = route_container->routes[i].get_handler(connection);
            pthread_rwlock_unlock(&routes_rwlock);
            return result;
        }
    }

    const char *not_found_response = "404 Not Found";
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(not_found_response), (void *)not_found_response, MHD_RESPMEM_PERSISTENT);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    pthread_rwlock_unlock(&routes_rwlock);
    return ret;
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
        log_debug("Failed to load route: %s\n", dlerror());
        return 0;
    }

    char route_name[256];
    extract_route_name(path, route_name);

    char handler_name[256];
    snprintf(handler_name, sizeof(handler_name), "%s_get_handler", route_name);
    int (*get_handler)(struct MHD_Connection *) = (int (*)(struct MHD_Connection *))dlsym(handle, handler_name);
    if (!get_handler) {
        log_debug("Failed to find %s in %s: %s\n", handler_name, path, dlerror());
        dlclose(handle);
        return 0;
    }

    snprintf(handler_name, sizeof(handler_name), "%s_post_handler", route_name);
    int (*post_handler)(struct MHD_Connection *, const char *) = (int (*)(struct MHD_Connection *, const char *))dlsym(handle, handler_name);
    if (!post_handler) {
        post_handler = NULL;
    }

    route->url = malloc(strlen(route_name) + 2);
    route->url[0] = '/';
    strcpy(route->url + 1, route_name);
    route->get_handler = get_handler;
    route->post_handler = post_handler;
    route->dl_handle = handle;

    log_debug("Registered route: %s\n", route->url);
    return 1;
}

void load_routes(struct Route **routes, int *route_count) {
    DIR *dir = opendir("./routes");
    struct dirent *entry;
    int idx = 0;

    if (dir == NULL) {
        perror("opendir");
        return;
    }

    *routes = calloc(MAX_ROUTES, sizeof(struct Route));
    if (*routes == NULL) {
        perror("calloc");
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == 8 && strstr(entry->d_name, ".so") != NULL) {
            char path[256];
            snprintf(path, sizeof(path), "./routes/%s", entry->d_name);

            if (load_route(&(*routes)[idx], path)) {
                idx++;
            }
        }
    }

    closedir(dir);
    *route_count = idx;
}