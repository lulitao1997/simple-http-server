#include <server.hpp>
#include <signal.h>
#include <sys/epoll.h>
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

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE

    epoll_event *events = (epoll_event*)calloc(MAX_CONCURRENT_NUM, sizeof(struct epoll_event)); // TODO: dynamically increase MAX_CONCURRENT_NUM
    epoll_event ev;

    int listen_fd = server_open_listen_fd();

    int epoll_fd = epoll_create1(0);
    LOG_IF(FATAL, epoll_fd < 0) << "epoll_create1";

    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;

    LOG_IF(FATAL, epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
        << "epoll_ctl";


    for (;;) {
        int nfds = epoll_wait(epoll_fd, events, MAX_CONCURRENT_NUM, -1);
        LOG_IF(FATAL, nfds < 0) << "error in epoll_wait";
        for (int i=0; i<nfds; i++) {
            LOG_IF(FATAL, events[i].events & EPOLLERR)
                << "error in epoll_wait";

            if (events[i].data.fd == listen_fd) { // Listening socket is ready
                int conn_sock = accept(listen_fd, (sockaddr*)addr, &addrlen);
                LOG_IF(FATAL, conn_sock < 0) << "accept";
                setnonblocking(conn_sock);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;

                LOG_IF(FATAL, epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) < 0)
                    << "epoll_ctl: conn_sock";

            }
            else { // Existing peer is ready
                if (events[i].events & EPOLLIN) { // ready reading
                    server_handle_request(events[i].data.fd);
                }
                else if (events[i].events & EPOLLOUT) { // ready for echoing
                    server_response(events[i].data.fd);
                }
                else {
                    LOG(FATAL) << "should not reach here";
                }
            }
        }
    }
}