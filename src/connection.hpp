#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "parser.hpp"
// #include "server.hpp"

#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctime>
#include <sys/epoll.h>
#include <fcntl.h>

#include <glog/logging.h>

#include <vector>
#include <set>


extern int epoll_fd;

struct connection_t {
    int fd;
    time_t accept_time;
    sockaddr_in addr; //peer's socket address
    epoll_event ev;

    connection_t& operator=(const connection_t& ) = delete;
    connection_t(const connection_t& ) = delete;

    connection_t& operator =(connection_t&& rhs) {
        // DLOG_IF(FATAL, fcntl(fd, F_GETFD) != -1 || errno != EBADF)
        //     << "fd " << fd << " still open when assigning other...";
        close();

        fd = rhs.fd;
        accept_time = rhs.accept_time;
        addr = rhs.addr;
        ev = rhs.ev;

        parser = rhs.parser;
        parser.c = this;

        rhs.fd = 0;
        return *this;
    }

    connection_t() {}
    connection_t(int fd, time_t accept_time, sockaddr_in addr, epoll_event ev):
        fd(fd),
        accept_time(accept_time),
        addr(addr),
        ev(ev)
        // parser(this)
    {
        // DLOG(INFO) << "connection constructor: " << this;
    }

    // static std::set<std::pair<time_t, const connection_t*>> S;
    // decltype(S.begin()) set_ptr; // my position in S.
    // iterator in the set stays.

    void close() {
        // if (set_ptr) S.erase(set_ptr);
        if (!fd) return;
        DLOG(INFO) << "dstr: " << fd;
        LOG_IF(FATAL, epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr))
            << "epoll_ctl del: " << strerror(errno);
        LOG_IF(WARNING, ::close(fd)) << "close " << fd << " error.";
        fd = 0;
        recv_buf.clear();
    }

    static void close_expired_connections() {
        time_t now = time(NULL);

    }

    buffer_t recv_buf; // TODO: custom allocator
    parser_t parser;

    int handle_request();
    int handle_response();

};

const int USRBUF_SIZE = 4096;

void peer_finished_request(connection_t *c);
void peer_finished_respond(connection_t *c);

#endif