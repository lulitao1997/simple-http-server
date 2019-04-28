#ifndef MESSAGE_HPP_
#define MESSAGE_HPP_

#include <functional>
#include <connection.hpp>

struct message_t {
    connection_t c;

    std::function<int(int)> send_callback, recv_callback;
};

#endif