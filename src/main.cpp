#include <server.hpp>
#include <signal.h>
#include <sys/epoll.h>

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE

}