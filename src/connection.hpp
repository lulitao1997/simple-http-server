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

    const char *mime = nullptr;

    void set_error_response(int err) {
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

    // connection_t& operator =(connection_t&& rhs) {
    void construct(int _fd, int _accept_time, sockaddr_in _addr, epoll_event _ev) {
        // DLOG_IF(FATAL, fcntl(fd, F_GETFD) != -1 || errno != EBADF)
        //     << "fd " << fd << " still open when assigning other...";
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
        // DLOG(INFO) << "befor erase: " << S.size();
        // S.erase(set_ptr);
        // DLOG(INFO) << "after erase: " << S.size();

        LOG_IF(FATAL, epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr))
            << "epoll_ctl del: " << strerror(errno);
        LOG_IF(WARNING, ::close(fd)) << "close " << fd << " error.";
        fd = 0;
        // recv_buf.clear();
    }

    static void close_expired();

    // buffer_t recv_buf, send_buf; // TODO: custom allocator
    // parser_t parser;
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
    void peer_finished_respond();

};

const int USRBUF_SIZE = 8192;

// void peer_finished_request(connection_t *c);

int on_url(http_parser *p, const char *at, size_t len);
int on_message_complete(http_parser *p);
// void peer_finished_respond(connection_t *c);


#endif