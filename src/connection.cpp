#include "connection.hpp"
#include "server.hpp"
#include "parser.hpp"

#include <sys/socket.h>
#include <netdb.h>
#include <glog/logging.h>

#include <iostream>

int connection_t::handle_request() {
    static char usrbuf[USRBUF_SIZE]; // no multi-thread, don't worry.
    #ifndef NDEBUG
    static char hostname[8192], port[8192];
    sockaddr_storage client_addr;
    DLOG_IF(FATAL, getnameinfo((sockaddr*)&addr, sizeof addr, hostname, 8192, port, 8192, 0));
    DLOG(INFO) << "peer " << hostname << ":" << port << " is sending...";
    DLOG(INFO) << "request fd: " << ev.data.fd << ", " << fd;
    #endif

    // parser_t parser(this, recv_buf);

    ssize_t nread;
    int total_read;
    for (;;) {
        // size_t old_size = recv_buf.size();
        // recv_buf.resize(old_size + USRBUF_SIZE);
        // nread = read(fd, &recv_buf[old_size], USRBUF_SIZE); // read as many as possible...
        nread = read(fd, usrbuf, USRBUF_SIZE);
        DLOG(INFO) << "peer " << hostname << ":" << port << " read " << nread << "bytes";
        if (nread < 0) {
            DLOG(INFO) << "peer " << hostname << ":" << port << " errno: " << strerror(errno);
            // recv_buf.resize(old_size);
            if (errno != EAGAIN) return -1;
            else return EAGAIN;
        }

        // recv_buf.resize(old_size + nread);
        recv_buf.insert(recv_buf.end(), usrbuf, usrbuf+nread);
        parser.step(nread);

        if (nread == 0) { // EOF
            DLOG(INFO) << "peer " << hostname << ":" << port << " finished sending";
            return 0;
        }

        #ifndef NDEBUG
        using namespace std;
        cerr << "recv_buf: ";
        for (char c: recv_buf) cerr << c;
        cerr << endl;
        #endif

    }
}

int connection_t::handle_response() {
    // TODO: assemble response according to request...
    #ifndef NDEBUG
    char hostname[8192], port[8192];
    sockaddr_storage client_addr;
    // DLOG_IF(FATAL, getnameinfo((sockaddr*)&addr, sizeof addr, hostname, 8192, port, 8192, 0));
    // DLOG(INFO) << "peer " << hostname << ":" << port << " is recevieing...";
    #endif

    DLOG(INFO) << "respond fd: " << ev.data.fd << ", " << fd;

    ssize_t nsend, total_nsend=0;
    for(;;) {
        // DLOG(INFO) << "recv_buf: " << recv_buf.size() << ", ";
        nsend = write(fd, recv_buf.data()+total_nsend, recv_buf.size()-total_nsend);
        DLOG(INFO) << "respond write: " << nsend;
        if (nsend < 0) {
            if (errno == EAGAIN) {
                return EAGAIN;
            }
            else {
                LOG(WARNING) << "connection closed by peer: " << strerror(errno);
                return -1;
            }
        }
        else if (nsend == 0) {
            // TODO: close connection according to keep-alive.
            peer_finished_respond(this);
            return 0;
        }
        else {
            total_nsend += nsend;
        }
    }
}


// TODO: keep alive....
void peer_finished_request(connection_t *c) {
    // disable read and enable write...
    DLOG(INFO) << "diable in and enable out for " << c->ev.data.fd;
    c->ev.events |= EPOLLOUT;
    c->ev.events &= ~EPOLLIN;
    LOG_IF(WARNING, epoll_ctl(epoll_fd, EPOLL_CTL_MOD, c->fd, &c->ev)) << "epoll_ctl request";
}
void peer_finished_respond(connection_t *c) {
    // disable write and enable read...
    DLOG(INFO) << "enable in and diable out for " << c->ev.data.fd;
    c->ev.events |= EPOLLIN;
    c->ev.events &= ~EPOLLOUT;
    LOG_IF(WARNING, epoll_ctl(epoll_fd, EPOLL_CTL_MOD, c->fd, &c->ev)) << "epoll_ctl respond";
}