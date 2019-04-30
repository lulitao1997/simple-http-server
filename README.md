
#
uses epoll to watch io events.

#
http1.0 / http 1.1
keep-alive




# Testing

## System Tuning

before testing, make sure that:

- `ulimit -n` is large enough
- max-system-watch

# Results

tested under Linux 5.0, Ryzen 2700x, 16GB Memory



# References

manual page of `epoll`
Computer System A Programmer's Perspective
RFC2616
