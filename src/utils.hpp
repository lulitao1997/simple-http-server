#ifndef UTILS_HPP
#define UTILS_HPP

#include "server.hpp"
#include <unistd.h>
#include <getopt.h>

#include <glog/logging.h>

#include <sys/types.h>
#include <sys/stat.h>

void parse_arguments(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "r:p:t:n:")) != -1) {
        switch (opt) {
        case 'r':
            config.root = std::string(optarg);
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
    LOG_IF(FATAL, stat(config.root.c_str(), &info) != 0)
        << "root dir \"" << config.root << "\" does not exist.\n";
}

#endif