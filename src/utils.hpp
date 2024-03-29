#ifndef UTILS_HPP
#define UTILS_HPP

#include "server.hpp"
#include <cstring>
#include <unistd.h>
#include <getopt.h>

#include <glog/logging.h>

#include <sys/types.h>
#include <sys/stat.h>

#define F(func) \
do { \
    if ((func) < 0) { \
        LOG(FATAL) << #func << ": " <<  strerror(errno); \
    } \
} while (0)

inline void parse_arguments(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "r:p:t:n:")) != -1) {
        switch (opt) {
        case 'r':
            strcpy(config.root, optarg);
            break;
        case 'p':
            config.port = atoi(optarg);
            break;
        case 't':
            config.timeout = atoi(optarg);
            break;
        case 'n':
            config.worker_num = atoi(optarg);
            break;

        default:
            LOG(FATAL) << "Usage: " << argv[0] << " -r www_root -p port -t timeout -n worker_num\n";
        }
    }

    // check wether the root dir exists
    struct stat info;
    F(stat(config.root, &info));
}

#endif