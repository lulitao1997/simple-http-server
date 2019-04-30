#include "parser.hpp"
#include "connection.hpp"
#include <glog/logging.h>

int parser_t::step(const char *buf, int n) {
    // DLOG_IF(FATAL, c == nullptr) << "fucked";
    // DLOG(INFO) << "after: recv_buf.size()" << c->recv_buf.size() << ", " << c->accept_time << ", " << c;
    for (int i=0; i<n; i++) {
        // char t = c->recv_buf[pos+i];
        char t = buf[i];

        if (is_first_line) {
            if (t == ' ') {

            }
        }

        if (t == '\r') continue;
        if (t == '\n') {
            is_first_line = false;
            if (crlf_cnt) {
                // DLOG(INFO) << "request end encountered..";
                // on_request_end(c);
            }
            crlf_cnt++;
        }
        else crlf_cnt = 0;
    }
    pos += n;
    return 0;
}