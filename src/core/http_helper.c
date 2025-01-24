#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "core/routes.h"
#include "core/http_helper.h"
#include "utils/logging.h"

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

    // Handle POST requests
    if (strcmp(method, "POST") == 0) {
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

        const char *response_str = "405 Method Not Allowed";
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), (void *)response_str, MHD_RESPMEM_PERSISTENT);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
        MHD_destroy_response(response);
        free(upload->buffer);
        free(upload);
        *con_cls = NULL;
        pthread_rwlock_unlock(&routes_rwlock);
        return ret;
    }

    // Handle GET requests
    if (strcmp(method, "GET") == 0) {
        for (int i = 0; i < route_container->route_count; i++) {
            if (strcmp(route_container->routes[i].url, url) == 0 && route_container->routes[i].get_handler) {
                enum MHD_Result result = route_container->routes[i].get_handler(connection);
                pthread_rwlock_unlock(&routes_rwlock);
                return result;
            }
        }
    }

    // Handle PUT requests
    if (strcmp(method, "PUT") == 0) {
        for (int i = 0; i < route_container->route_count; i++) {
            if (strcmp(route_container->routes[i].url, url) == 0 && route_container->routes[i].put_handler) {
                enum MHD_Result result = route_container->routes[i].put_handler(connection, upload_data);
                pthread_rwlock_unlock(&routes_rwlock);
                return result;
            }
        }
    }

    // Handle DELETE requests
    if (strcmp(method, "DELETE") == 0) {
        for (int i = 0; i < route_container->route_count; i++) {
            if (strcmp(route_container->routes[i].url, url) == 0 && route_container->routes[i].delete_handler) {
                enum MHD_Result result = route_container->routes[i].delete_handler(connection);
                pthread_rwlock_unlock(&routes_rwlock);
                return result;
            }
        }
    }

    // Handle PATCH requests
    if (strcmp(method, "PATCH") == 0) {
        for (int i = 0; i < route_container->route_count; i++) {
            if (strcmp(route_container->routes[i].url, url) == 0 && route_container->routes[i].patch_handler) {
                enum MHD_Result result = route_container->routes[i].patch_handler(connection, upload_data);
                pthread_rwlock_unlock(&routes_rwlock);
                return result;
            }
        }
    }

    // Handle unsupported methods
    const char *response_str = "405 Method Not Allowed";
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_str), (void *)response_str, MHD_RESPMEM_PERSISTENT);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
    MHD_destroy_response(response);
    pthread_rwlock_unlock(&routes_rwlock);
    return ret;
}
