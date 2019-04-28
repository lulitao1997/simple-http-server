#include "server.hpp"

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

// pool_t<connection_t, MAX_CONCURRENT_NUM> pool;
connection_t fd2connection[MAX_FD];

// int server_handle_request(int fd) {

// }

// int server_response(int fd) {

// }

void setup() {

}

