#ifndef ROUTES_H
#define ROUTES_H

#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>

#if !defined(__USE_UNIX98) && !defined(__USE_XOPEN2K)
typedef union {
    struct {
        int __lock;
        unsigned int __nr_readers;
        unsigned int __readers_wakeup;
        unsigned int __writer_wakeup;
        unsigned int __nr_readers_queued;
        unsigned int __nr_writers_queued;
        int __writer;
        int __shared;
        unsigned long int __pad1;
        unsigned long int __pad2;
    } __data;
    char __size[56];
    long int __align;
} pthread_rwlock_t;

typedef union {
    char __size[8];
    long int __align;
} pthread_rwlockattr_t;
#endif

extern pthread_rwlock_t routes_rwlock;

#define MAX_ROUTES 100

struct RouteContainer {
    struct Route *routes;
    int route_count;
};

struct UploadData {
    char *buffer;
    size_t size;
};

struct Route {
    char *url;
    int (*get_handler)(struct MHD_Connection *);
    int (*post_handler)(struct MHD_Connection *, const char *);
    int (*put_handler)(struct MHD_Connection *, const char *);
    int (*delete_handler)(struct MHD_Connection *);
    int (*patch_handler)(struct MHD_Connection *, const char *);
    void *dl_handle;
};

void extract_route_name(const char *file_name, char *route_name);
int load_route(struct Route *route, const char *path);
void load_routes(struct Route **routes, int *route_count);
void* monitor_routes(void* arg);
void reload_route(struct RouteContainer *route_container, const char *file_name);

#endif // ROUTES_H