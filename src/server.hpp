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

const int MAX_CONCURRENT_NUM = 30000;
const int MAX_FD = 110000;
const int LISTENQ = 1024;

struct config_t {
    std::string root;
    int port;
    int timeout;
    int worker_num;

    config_t():
        root("./"),
        port(9999),
        timeout(200),
        worker_num(1)
    {}
};

extern config_t config;
// extern pool_t<connection_t, MAX_CONCURRENT_NUM> pool;
extern connection_t fd2connection[MAX_FD];

extern int epoll_fd;

int server_open_listen_fd();
// int server_handle_request(int fd);
// int server_response(int fd);
int server_register_event(int fd, const epoll_event& ev);
void setup();

#endif