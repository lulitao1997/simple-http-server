#include "server.hpp"
#include "utils.hpp"
#include <netinet/tcp.h>
#include <glog/logging.h>

epoll_event ev, events[MAX_CONCURRENT_NUM];

int main(int argc, char *argv[]) {
    parse_arguments(argc, argv);
    setup();

    int listen_fd = server_open_listen_fd();
    DLOG(INFO) << "listen_fd: " << listen_fd;

    epoll_fd = epoll_create1(0);
    LOG_IF(FATAL, epoll_fd < 0) << "epoll_create1";

    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;

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

                // very important! eliminate 40ms delay,
                // see https://www.fanhaobai.com/2017/11/40ms-delay-and-tcp-nodelay.html
                static int enable = 1;
                setsockopt(conn_sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof enable);

                LOG_IF(FATAL, conn_sock < 0) << "accept4 " << strerror(errno);
                LOG_IF(FATAL, conn_sock >= MAX_FD) << "conn_sock: " << conn_sock << " is too large.";

                // edge-triggered, make sure we wait after consumed all the things in the r/w buffer
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                fd2connection[conn_sock].construct(conn_sock, time(NULL), addr, ev);

                LOG_IF(FATAL, epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) < 0)
                    << "epoll_ctl: conn_sock";
            }
            else { // Existing peer is ready
                connection_t &c = fd2connection[efd];
                if (events[i].events & EPOLLIN) { // ready reading
                    if (c.handle_request() <= 0) // 0 = client closed properly, we should also close it.
                        c.close();
                }
                else if (events[i].events & EPOLLOUT) { // ready for echoing
                    int ret = c.handle_response();
                    if (ret < 0)
                        c.close();
                    else if (ret == 0)
                        c.finish_respond();
                }
                else {
                    LOG(WARNING) << "should not reach here";
                }
            }
        }
        connection_t::close_expired();
    }
}