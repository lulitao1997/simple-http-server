#ifndef SERVER_HPP
#define SERVER_HPP

#include "pool.hpp"
#include "connection.hpp"

#include <string>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>

const int MAX_CONCURRENT_NUM = 1024 * 10;
const int MAX_FD = MAX_CONCURRENT_NUM * 10;
const int LISTENQ = 1024;
const int MAXPATH = 1024;

typedef unsigned long long mtime_t;

struct config_t {
    char root[MAXPATH];
    int port;
    mtime_t timeout;
    int worker_num;

    config_t():
        root("./"),
        port(9999),
        timeout(2000), // 2 seconds.
        worker_num(4)
    {}
};

extern config_t config;
extern connection_t fd2connection[MAX_FD];

extern int epoll_fd;

int server_open_listen_fd();
void setup();

mtime_t mtime();

#endif