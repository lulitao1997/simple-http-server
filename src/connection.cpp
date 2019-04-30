#include "connection.hpp"
#include "server.hpp"
#include "parser.hpp"

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
    response = response_t{};

    // reset parser states.
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = this;
}

int connection_t::handle_request() {
    static char usrbuf[USRBUF_SIZE]; // no multi-thread, don't worry.
    #ifndef NDEBUG
    static char hostname[8192], port[8192];
    sockaddr_storage client_addr;
    // DLOG_IF(FATAL, getnameinfo((sockaddr*)&addr, sizeof addr, hostname, 8192, port, 8192, 0));
    // DLOG(INFO) << "peer " << hostname << ":" << port << " is sending...";
    // DLOG(INFO) << "request fd: " << ev.data.fd << ", " << fd;
    #endif

    // parser_t parser(this, recv_buf);

    ssize_t nread;
    int total_read;
    for (;;) {
        nread = read(fd, usrbuf, USRBUF_SIZE);
        if (nread < 0) {
            if (errno != EAGAIN) {
                LOG(WARNING) << "connection reset by peer: " << fd;
                return -1;
            }
            else return EAGAIN;
        }

        int nparsed = http_parser_execute(&parser, &psetting, usrbuf, nread);
        if (nparsed < nread) {
            // TODO: Bad request
            return -1;
        }


        if (nread == 0) { // EOF
            // DLOG(INFO) << "peer " << hostname << ":" << port << " finished sending";
            // LOG_IF(WARNING, close(fd));
            return 0;
        }

        // #ifndef NDEBUG
        // using namespace std;
        // cerr << "recv_buf: ";
        // for (char c: recv_buf) cerr << c;
        // cerr << endl;
        // #endif

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
        "Content-Type: %s\r\n", "text/html" // TODO: mime type according to file extension...
    );
    len += sprintf(send_buf.data()+len,
        "Content-Length: %zu\r\n",
        r.content_length
    );
    if (r.http_major == 1 && r.http_minor ==1)
        len += sprintf(send_buf.data()+len,
            "Connection: %s\r\n",
            (r.keep_alive ? "keep-alive" : "close")
        );

    len += sprintf(send_buf.data()+len, "\r\n");

    if (r.status_code != 200) {
        len += sprintf(send_buf.data()+len, "error: %s\r\n", http_status_str((http_status)r.status_code));
    }

    send_buf.resize(len);
    return 0;
}

int connection_t::handle_response() {
    if (send_buf.empty()) assemble_header();

    if (total_nsend < send_buf.size()) {
        int ret = send_header();
        DLOG(INFO) << "header: " << total_nsend << ", " << send_buf.size() << ", " << ret << fd;
        DLOG(INFO) << "header: " << std::string(send_buf.begin(), send_buf.end());
        if (ret == 0)
            return send_file();
        else return ret;
    }
    else return send_file();
    return 0;
}

int connection_t::send_file() {
    if (response.status_code != 200)
        return 0;

    #ifndef NDEBUG
    char hostname[8192], port[8192];
    sockaddr_storage client_addr;
    DLOG_IF(FATAL, getnameinfo((sockaddr*)&addr, sizeof addr, hostname, 8192, port, 8192, 0));
    // DLOG(INFO) << "peer " << hostname << ":" << port << " is recevieing...";
    #endif

    // DLOG(INFO) << "respond fd: " << ev.data.fd << ", " << fd;

    // send_buf.insert(send_buf.end(), sample_response, sample_response + strlen(sample_response));

    // DLOG(INFO) << "sending file... " << response.status_code << ", " << response.fd;
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
    // TODO: assemble response according to request...
    #ifndef NDEBUG
    char hostname[8192], port[8192];
    sockaddr_storage client_addr;
    // DLOG_IF(FATAL, getnameinfo((sockaddr*)&addr, sizeof addr, hostname, 8192, port, 8192, 0));
    // DLOG(INFO) << "peer " << hostname << ":" << port << " is recevieing...";
    #endif

    // if (!sample_response_len) sample_response_len = strlen(sample_response);

    // DLOG(INFO) << "respond fd: " << ev.data.fd << ", " << fd;

    // send_buf.insert(send_buf.end(), sample_response, sample_response + strlen(sample_response));

    ssize_t nsend;
    for(;;) {
        // nsend = write(fd, sample_response+total_nsend, sample_response_len-total_nsend);
        nsend = write(fd, send_buf.data()+total_nsend, send_buf.size()-total_nsend);
        if (nsend < 0) {
            if (errno == EAGAIN) {
                return EAGAIN;
            }
            else {
                DLOG(WARNING) << "connection closed by peer: " << strerror(errno);
                return -1;
            }
        }
        else if (nsend == 0) {
            // peer_finished_respond();
            return 0;
        }
        else {
            total_nsend += nsend;
        }
    }
}

int on_url(http_parser *p, const char *at, size_t len) {
    static char path_buf[MAXPATH] = "";
    static int dir_fd = 0;
    static const char *index_html = "/index.html";
    auto c = (connection_t*)p->data;
    response_t& r = c->response;

    // r.http_major = p->http_major;
    // r.http_minor = p->http_minor;
    // LOG(INFO) << "sldkfjsldkfj" << p->http_major;

    if (!dir_fd) {
        dir_fd = open(config.root, O_DIRECTORY);
        LOG_IF(FATAL, dir_fd < 0) << "open dir: " << strerror(errno);
    }

    if (len == 1 && at[0] == '/') {
        at = index_html;
        len = 11;
    }

    for (int i=1; i<len; i++)
        path_buf[i-1] = at[i];
    path_buf[len-1] = '\0';

    struct stat st;
    r.fd = openat(dir_fd, path_buf, O_RDONLY);
    if (r.fd < 0) {
        LOG(WARNING) << "open file " << path_buf << ": " << strerror(errno);
        r.http_major = 1;
        r.http_minor = 0;
        r.status_code = 404;
        r.keep_alive = false;
        return 0;
        // return -1;
    }
    LOG_IF(FATAL, fstat(r.fd, &st)) << "stat: " << strerror(errno);
    // LOG(INFO) << "r.fd: " << r.fd;
    r.content_length = st.st_size;

    return 0;
}

// TODO: keep alive....
int on_message_complete(http_parser *p) {
    // disable read and enable write...
    // DLOG(INFO) << "request finished.....";
    LOG(INFO) << "laksjdflkjldks" << p->http_major;

    auto c = (connection_t*)p->data;
    c->ev.events |= EPOLLOUT;
    c->ev.events &= ~EPOLLIN;
    LOG_IF(WARNING, epoll_ctl(epoll_fd, EPOLL_CTL_MOD, c->fd, &c->ev)) << "epoll_ctl request";

    auto& r = c->response;
    r.keep_alive = http_should_keep_alive(p) ;
    if (!r.http_major) {
        r.http_major = p->http_major;
        r.http_minor = p->http_minor;
    }

    return 0;
}

void connection_t::peer_finished_respond() {
    DLOG(INFO) << "peer " << fd << " finished respond: " << response.keep_alive;

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
    // DLOG(INFO) << S.begin()->first << ", " << now;
    while(!S.empty() && S.begin()->first + config.timeout < now) {
        DLOG_IF(FATAL, !S.begin()->second->fd) << "invalid element.";
        S.begin()->second->close();

        #ifndef NDEBUG
        closed_num++;
        #endif
    }

    DLOG_IF(INFO, closed_num) << "closed " << closed_num << " connections, " << S.size() << "left.";
}