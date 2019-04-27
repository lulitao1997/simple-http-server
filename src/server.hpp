#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

struct config_t {
    std::string root;
    int port;
    int timeout;
    int worker_num;
};

extern config_t config;

#endif