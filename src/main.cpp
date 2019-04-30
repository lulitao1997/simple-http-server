#include "server.hpp"
#include <signal.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <glog/logging.h>

epoll_event ev, events[MAX_CONCURRENT_NUM];

void sigint_handler(int signum) {
    if (signum == SIGINT) {
        kill(-getpid(), SIGINT);
        LOG(WARNING) << "worker " << getpid() << " exiting...";
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    FLAGS_colorlogtostderr = true;
    FLAGS_logbufsecs = 0;
    setup();

    signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE

    int listen_fd = server_open_listen_fd();
    DLOG(INFO) << "listen_fd: " << listen_fd;

    epoll_fd = epoll_create1(0);
    LOG_IF(FATAL, epoll_fd < 0) << "epoll_create1";

    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    // ev.data.ptr = pool.alloc(listen_fd, 0, ); //UNION!!!!!

    LOG_IF(FATAL, epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
        << "epoll_ctl";

    for (;;) {
        int nfds = epoll_wait(epoll_fd, events, MAX_CONCURRENT_NUM, 20);
        LOG_IF(FATAL, nfds < 0) << "error in epoll_wait";
        for (int i=0; i<nfds; i++) {
            int efd = events[i].data.fd;

            if (efd == listen_fd) { // Listening socket is ready
                static sockaddr_in addr;
                socklen_t saddrlen = sizeof addr;
                int conn_sock = accept4(listen_fd, (sockaddr*)&addr, &saddrlen, SOCK_NONBLOCK);
                LOG_IF(FATAL, conn_sock < 0) << "accept4 " << strerror(errno);
                LOG_IF(FATAL, conn_sock >= MAX_FD) << "conn_sock: " << conn_sock << " is too large.";
                // edge-triggered, make sure we wait after consumed all the things in the r/w buffer
                ev.events = EPOLLIN | EPOLLET;
                // DLOG(INFO) << "new peer, fd: " << conn_sock;
                ev.data.fd = conn_sock;
                fd2connection[conn_sock].construct(conn_sock, time(NULL), addr, ev);

                LOG_IF(FATAL, epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) < 0)
                    << "epoll_ctl: conn_sock";
            }
            else { // Existing peer is ready
                connection_t &c = fd2connection[efd];
                if (events[i].events & EPOLLIN) { // ready reading
                    // DLOG(INFO) << "in event: " << efd << ", " << fd2connection + efd;
                    if (c.handle_request() <= 0) // 0 = client closed, we should also close it.
                        c.close();
                }
                else if (events[i].events & EPOLLOUT) { // ready for echoing
                    // DLOG(INFO) << "out event: " << efd;
                    int ret = c.handle_response();
                    if (ret < 0)
                        c.close();
                    else if (ret == 0)
                        c.peer_finished_respond();

                }
                else {
                    LOG(WARNING) << "should not reach here";
                }
            }
        }
        connection_t::close_expired();
    }
}