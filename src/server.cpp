#include "server.hpp"
#include "mime.hpp"

#include <sys/types.h>

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
#include <unistd.h>
#include <sys/time.h>

#include <glog/logging.h>

config_t config;

int epoll_fd;

int server_open_listen_fd() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    LOG_IF(FATAL, fd < 0) << "Error opening socket";

    int opt = 1;
    LOG_IF(FATAL, setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof opt) < 0)
        << "setsockopt";

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof serv_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(config.port);

    LOG_IF(FATAL, bind(fd, (sockaddr*)(&serv_addr), sizeof serv_addr) < 0)
        << "bind";

    LOG_IF(FATAL, listen(fd, LISTENQ) < 0)
        << "listen";

    LOG(INFO) << "PID: " << getpid() << " started listening on: " << config.port;

    return fd;
}

connection_t fd2connection[MAX_FD];

std::unordered_map<view_t, const char*> mime_map;

void sigint_handler(int signum) {
    if (signum == SIGINT) {
        kill(-getpid(), SIGINT);
        LOG(WARNING) << "worker " << getpid() << " exiting...";
        exit(0);
    }
}

void setup() {
    signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE
    signal(SIGINT, sigint_handler);

    FLAGS_colorlogtostderr = true;
    FLAGS_logbufsecs = 0;

    connection_t::psetting.on_url = on_url;
    connection_t::psetting.on_message_complete = on_message_complete;


    mime_map = {
        {"html", "text/html"},
        {"htm", "text/html"},
        {"txt", "text/plain"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"xml", "text/xml"},

        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"png", "image/png"},
        {"webm", "image/webm"},
        {"svg", "image/svg+xml"},

        {"pdf", "application/pdf"},

        {"mp4", "video/mp4"},
        {"mkv", "video/mkv"},
    };

    for (int i=1; i<config.worker_num; i++) {
        int ret = fork();
        if (ret == 0) break;
        LOG_IF(FATAL, ret < 0) << "fork: " << strerror(errno);
    }
}

mtime_t mtime() {
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (mtime_t)(tv.tv_sec) * 1000 +
           (mtime_t)(tv.tv_usec) / 1000;

}