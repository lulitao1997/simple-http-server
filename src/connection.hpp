#ifndef CONNECTION_HPP
#define CONNECTION_HPP

// #include "parser.hpp"
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

#include <http_parser.h>


extern int epoll_fd;

using buffer_t = std::vector<char>;

typedef unsigned long long mtime_t;

struct response_t {
    int http_major=0, http_minor=0;
    int status_code=200;
    bool keep_alive=0;
    size_t content_length=0;
    int fd=0;

    const char *mime = "text/html";

    void set_error_response(int err) {
        if (http_major) return;
        http_major = 1; http_minor = 0;
        status_code = err;
        keep_alive = false;
        mime = "text/plain";
        fd = 0;
    }
};

struct connection_t {
    int fd;
    mtime_t accept_time;
    sockaddr_in addr; //peer's socket address
    epoll_event ev;

    connection_t& operator=(const connection_t& ) = delete;
    connection_t(const connection_t& ) = delete;

    void construct(int _fd, int _accept_time, sockaddr_in _addr, epoll_event _ev) {
        close(); // TODO: maybe assert(!fd) ?
        refresh();
        std::tie(fd, accept_time, addr, ev) = std::make_tuple(_fd, _accept_time, _addr, _ev);

    }

    void refresh();

    connection_t() {}

    static std::set<std::pair<time_t, connection_t*>> S;
    decltype(S)::iterator set_ptr; // my position in S. iterator of std::set stays valid after erase/insert.
    // bool response_finished = false;

    void close() {
        if (!fd) return;
        DLOG(INFO) << "dstr: " << fd;

        S.erase(set_ptr);

        LOG_IF(FATAL, epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr))
            << "epoll_ctl del: " << strerror(errno);
        LOG_IF(WARNING, ::close(fd)) << "close " << fd << " error.";
        fd = 0;
        // recv_buf.clear();
    }

    static void close_expired();

    buffer_t send_buf;
    int total_nsend;
    http_parser parser;
    response_t response;
    static http_parser_settings psetting;

    int handle_request();
    int handle_response();
    int set_error_response(int err);
    int assemble_header();
    int send_header();
    int send_file();
    void finish_respond();

};

const int USRBUF_SIZE = 8192;

int on_url(http_parser *p, const char *at, size_t len);
int on_message_complete(http_parser *p);


#endif