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

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE

    // epoll_event *events = (epoll_event*)calloc(MAX_CONCURRENT_NUM, sizeof(epoll_event)); // TODO: dynamically increase MAX_CONCURRENT_NUM
    // epoll_event ev;

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
        int nfds = epoll_wait(epoll_fd, events, MAX_CONCURRENT_NUM, -1);
        LOG_IF(FATAL, nfds < 0) << "error in epoll_wait";
        for (int i=0; i<nfds; i++) {
            // LOG_IF(FATAL, events[i].events & EPOLLERR)
            //     << "error in epoll_wait";
            DLOG(INFO) << "events fd: " << events[i].data.fd;

            int efd = events[i].data.fd;

            if (efd== listen_fd) { // Listening socket is ready
                static sockaddr_in addr;
                socklen_t saddrlen = sizeof addr;
                int conn_sock = accept4(listen_fd, (sockaddr*)&addr, &saddrlen, SOCK_NONBLOCK);
                LOG_IF(FATAL, conn_sock < 0) << "accept4";
                // edge-triggered, make sure we wait after consumed all the things in the r/w buffer
                ev.events = EPOLLIN | EPOLLET;
                // ev.data.fd = conn_sock;
                // get a new connection_t
                DLOG(INFO) << "new peer, fd: " << conn_sock;
                // connection_t *c = pool.alloc(conn_sock, time(NULL), addr, ev);
                // ev.data.ptr = c;
                ev.data.fd = conn_sock;
                fd2connection[conn_sock] = connection_t(conn_sock, time(NULL), addr, ev);


                LOG_IF(FATAL, epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) < 0)
                    << "epoll_ctl: conn_sock";

                // server_register_event(conn_sock, ev);
            }
            else { // Existing peer is ready
                if (events[i].events & EPOLLIN) { // ready reading
                    DLOG(INFO) << "in event: " << efd;
                    fd2connection[efd].handle_request();
                }
                else if (events[i].events & EPOLLOUT) { // ready for echoing
                    DLOG(INFO) << "out event: " << efd;
                    fd2connection[efd].handle_response();
                }
                else {
                    LOG(FATAL) << "should not reach here";
                }
            }
        }
    }
}