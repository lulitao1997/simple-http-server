#include "connection.hpp"
#include "server.hpp"
// #include "parser.hpp"
#include "mime.hpp"

#include <sys/socket.h>
#include <netdb.h>
#include <glog/logging.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/sendfile.h>

#include <http_parser.h>

#include <iostream>

http_parser_settings connection_t::psetting;

void connection_t::refresh() {
    // refresh accept time
    if (fd) S.erase(set_ptr);

    accept_time = mtime();
    bool insert_success;
    tie(set_ptr, insert_success) = S.insert(std::make_pair(accept_time, this));
    LOG_IF(FATAL, !insert_success) << "insert";

    // clear send_buf
    send_buf.clear();
    total_nsend = 0;
    if (response.fd) LOG_IF(WARNING, ::close(response.fd)) << "close: " << strerror(errno);
    response = response_t{};

    // reset parser states.
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = this;
}

int connection_t::handle_request() {
    static char usrbuf[USRBUF_SIZE]; // no multi-thread, don't worry.

    ssize_t nread;
    int total_read;
    for (;;) {
        nread = read(fd, usrbuf, USRBUF_SIZE);
        if (nread < 0) {
            if (errno != EAGAIN) {
                LOG(WARNING) << "peer " << fd << ": " << strerror(errno);
                return -1;
            }
            else return EAGAIN;
        }
        DLOG(INFO) << ">>> MESSGAE: " << usrbuf;

        int nparsed = http_parser_execute(&parser, &psetting, usrbuf, nread);
        if (nparsed < nread) {
            response.set_error_response(400);
            on_message_complete(&parser);
            return EAGAIN; // TODO: too ugly...
        }

        if (nread == 0) { // EOF
            return 0;
        }
    }
}

const int HEADER_MAX_LEN = 200;
const char *SERVER_STR = "simple-http-server";
const char *SERVER_VER = "0.1";

// TODO: maybe split it up so it can execute when reading??
int connection_t::assemble_header() {
    send_buf.clear();
    send_buf.resize(HEADER_MAX_LEN);
    // use vector, less malloc()
    const response_t& r = response;

    int len = sprintf(send_buf.data(),
        "HTTP/%d.%d %d %s\r\n",
        r.http_major, r.http_minor, r.status_code, http_status_str((http_status)r.status_code)
    );
    // len += sprintf(send_buf.data()+len,
    //     "Date: %s\r\n",
    // );
    len += sprintf(send_buf.data()+len,
        "Server: %s/%s\r\n", SERVER_STR, SERVER_VER
    );
    len += sprintf(send_buf.data()+len,
        "Content-Type: %s\r\n", r.mime
    );
    if (r.status_code == 200) {
        len += sprintf(send_buf.data()+len,
            "Content-Length: %zu\r\n",
            r.content_length
        );
    }
    if (r.http_major == 1 && r.http_minor ==1)
        len += sprintf(send_buf.data()+len,
            "Connection: %s\r\n",
            (r.keep_alive ? "keep-alive" : "close")
        );

    len += sprintf(send_buf.data()+len, "\r\n");

    if (r.status_code != 200) {
        len += sprintf(send_buf.data()+len,
                       "error: %d %s\r\n",
                       r.status_code, http_status_str((http_status)r.status_code));
    }

    send_buf.resize(len);
    return 0;
}

int connection_t::handle_response() {
    if (send_buf.empty()) assemble_header();

    if (total_nsend < send_buf.size()) {
        int ret = send_header();
        if (ret == 0) {
            if (response.status_code == 200) return send_file();
            else return -1;
        }
        else return ret;
    }
    else return send_file();
    return 0;
}

int connection_t::send_file() {
    if (response.status_code != 200)
        return 0;

    ssize_t nsend, total_nsend=0;
    for(;;) {
        int nsend = sendfile(fd, response.fd, nullptr, response.content_length);
        if (nsend < 0) {
            if (errno == EAGAIN)
                return EAGAIN;
            else {
                DLOG(WARNING) << "connection closed by peer: " << fd << " " << strerror(errno);
                return -1;
            }
        }
        else if (nsend == 0) {
            return 0;
        }
        else {
            total_nsend += nsend;
        }
    }
}

int connection_t::send_header() {
    ssize_t nsend;
    for(;;) {
        nsend = write(fd, send_buf.data()+total_nsend, send_buf.size()-total_nsend);
        if (nsend < 0) {
            if (errno == EAGAIN) {
                return EAGAIN;
            }
            else {
                DLOG(WARNING) << "connection closed by peer: " << fd << strerror(errno);
                return -1;
            }
        }
        else if (nsend == 0)
            return 0;
        else
            total_nsend += nsend;
    }
}

int on_url(http_parser *p, const char *at, size_t len) {
    static char path_buf[MAXPATH] = "";
    static int dir_fd = 0;
    auto c = (connection_t*)p->data;
    response_t& r = c->response;

    if (!dir_fd) {
        dir_fd = open(config.root, O_DIRECTORY);
        LOG_IF(FATAL, dir_fd < 0) << "open dir: " << strerror(errno);
    }

    DLOG(INFO) << "PATH: " << std::string(at, at+len) << "|";
    int plen = 0;
    const char *dot_pos = nullptr;
    for (; plen+1<len; plen++) {
        path_buf[plen] = at[plen+1];
        if (at[plen+1] == '.') dot_pos = at+plen+1;
        if (at[plen+1] == '?')
            break;
    }
    path_buf[plen] = '\0';

    auto check = [&]() {
        if (r.fd < 0) {
            LOG(WARNING) << "open file " << config.root << (plen ? path_buf : "./") << ": " << strerror(errno);
            r.set_error_response(404);
            return -1;
        }
        return 0;
    };

    struct stat st;
    r.fd = openat(dir_fd, plen ? path_buf : "./", O_RDONLY);

    if (check()) return 0;

    LOG_IF(FATAL, fstat(r.fd, &st)) << "stat: " << strerror(errno);

    if (S_ISDIR(st.st_mode)) {
        int old_fd = r.fd;
        r.fd = openat(old_fd, "index.html", O_RDONLY);
        r.mime = mime_map["html"];
        close(old_fd);
    }

    if (check()) return 0;

    // get extension
    if (dot_pos) r.mime = mime_map[(view_t){dot_pos+1, at+len}];
    LOG_IF(FATAL, fstat(r.fd, &st)) << "stat: " << strerror(errno);
    r.content_length = st.st_size;

    return 0;
}

int on_message_complete(http_parser *p) {
    auto c = (connection_t*)p->data;
    c->ev.events |= EPOLLOUT;
    c->ev.events &= ~EPOLLIN;
    LOG_IF(WARNING, epoll_ctl(epoll_fd, EPOLL_CTL_MOD, c->fd, &c->ev)) << "epoll_ctl request";

    auto& r = c->response;
    if (!r.http_major) {
        r.keep_alive = http_should_keep_alive(p) ;
        r.http_major = p->http_major;
        r.http_minor = p->http_minor;
    }

    return 0;
}

void connection_t::finish_respond() {
    if (response.keep_alive) {
        ev.events |= EPOLLIN;
        ev.events &= ~EPOLLOUT;
        LOG_IF(WARNING, epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev))
            << "epoll_ctl respond";
        refresh();
    }
    else {
        close();
        DLOG(INFO) << "close after respond: " << fd;
    }
}

std::set<std::pair<time_t, connection_t*>> connection_t::S;

void connection_t::close_expired() {
    mtime_t now = mtime();
    #ifndef NDEBUG
    int closed_num = 0;
    #endif
    while(!S.empty() && S.begin()->first + config.timeout < now) {
        DLOG_IF(FATAL, !S.begin()->second->fd) << "invalid element.";
        S.begin()->second->close();

        #ifndef NDEBUG
        closed_num++;
        #endif
    }

    #ifndef NDEBUG
    DLOG_IF(INFO, closed_num) << "closed " << closed_num << " connections, " << S.size() << "left.";
    #endif
}