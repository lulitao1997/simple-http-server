#include "parser.hpp"
#include "view.hpp"
#include "connection.hpp"
#include <glog/logging.h>

int parser_t::step(int n) {
    // DLOG_IF(FATAL, c == nullptr) << "fucked";
    // DLOG(INFO) << "after: recv_buf.size()" << c->recv_buf.size() << ", " << c->accept_time << ", " << c;
    for (int i=0; i<n; i++) {
        char t = c->recv_buf[pos+i];
        // DLOG(INFO) << ">>>> " << pos << ", " << n << ", " << t << ", " << crlf_cnt;
        if (t == '\r') continue;
        if (t == '\n') {
            if (crlf_cnt) {
                DLOG(INFO) << "request end encountered..";
                on_request_end(c);
            }
            crlf_cnt++;
        }
        else crlf_cnt = 0;
    }
    pos += n;
    return 0;
}