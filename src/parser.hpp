#ifndef PARSER_HPP_
#define PARSER_HPP_

#include <cstring>
#include <functional>
#include <vector>

struct parse_result_t {

};

struct connection_t;


struct view_t {
    const char *pos = nullptr;
    size_t len = 0;
};

inline bool eq(view_t v, const char *c) {
    for (int i=0; i<v.len; i++)
        if (v.pos[i] != c[i])
            return false;
    return true;
}

struct parser_t {
    connection_t *c;
    int pos;

    parser_t(connection_t *c=nullptr):
        c(c), pos(0)
        {}

    parse_result_t res;

    int step(const char *pos, int n);
    int crlf_cnt = 0;
    bool is_first_line = true; int first_line_space_cnt = 0;
    int last_crlf_pos = 0;

    view_t method, uri, version;

    int keep_alive = 0;
};

#endif