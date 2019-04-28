#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "parser.hpp"

#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctime>
#include <sys/epoll.h>

#include <vector>


struct connection_t {
    int fd;
    time_t accept_time;
    sockaddr_in addr; //peer's socket address
    epoll_event ev;

    connection_t() {}
    connection_t(int fd, time_t accept_time, sockaddr_in addr, epoll_event ev):
        fd(fd),
        accept_time(accept_time),
        addr(addr),
        ev(ev),
        parser(this, recv_buf)
    {}

    buffer_t recv_buf; // TODO: custom allocator
    parser_t parser;

    int handle_request();
    int handle_response();
};

const int USRBUF_SIZE = 4096;

void peer_finished_request(connection_t *c);
void peer_finished_respond(connection_t *c);

#endif