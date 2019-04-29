#ifndef PARSER_HPP_
#define PARSER_HPP_

#include <cstring>
#include <functional>
#include <vector>

struct parse_result_t {

};

struct connection_t;

using buffer_t = std::vector<char>;

void peer_finished_request(connection_t *c);

struct parser_t {
    connection_t *c;
    // const buffer_t *buf;
    int pos;

    parser_t(connection_t *c=nullptr):
        c(c), pos(0)
        {}

    parse_result_t res;

    int step(int n);
    int crlf_cnt = 0;
    std::function<void(connection_t*)> on_request_end = peer_finished_request;
};

#endif