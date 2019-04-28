#include "parser.hpp"
#include "view.hpp"
#include <glog/logging.h>

int parser_t::step(int n) {
    for (int i=0; i<n; i++) {
        char t = buf[pos+i];
        DLOG(INFO) << ">>>> " << pos << ", " << n << ", " << t << ", " << crlf_cnt;
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