#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

const int MAX_CONCURRENT_NUM = 110000;
const int LISTENQ = 1024;

struct config_t {
    std::string root;
    int port;
    int timeout;
    int worker_num;
};

extern config_t config;

int server_open_listen_fd();
void server_handle_request(int fd);
void server_reponse(int fd);
void setup();

#endif