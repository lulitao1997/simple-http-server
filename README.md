
# simple-web-server

this is a simple web server, using IO multiplexing model.
it uses `epoll` to handle concurrent events.

# Supported Features

- HTTP GET
- Connection Header (keep-alive / close)
- 200 OK / 400 Bad Request / 404 Not Found / 501 Not Implemented
- simple error page

# Build & Testing

**you need to install glog (package named `libgoogle-glog-dev` under ubuntu) for logging.**
after that:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

then start webserver under sample_website directory:

```bash
src/SERVER -r ../sample_website -p 9999
```

browse `127.0.0.1:9999` in your browser, local contents are serverd by this server.



# Testing

before testing, make sure that:

- `ulimit -n` > 1000k
- `sysctl fs.inotify.max_user_watches` > 10k

## Results

tested under Linux 5.0, Ryzen 2700x@3.7GHz, 16GB Memory, server uses 4 processes.

tested using [`wrk`](https://github.com/wg/wrk)

### Connection: keep-alive

```
Running 5s test @ http://127.0.0.1:9999
  2 threads and 10000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    24.54ms    4.40ms  55.76ms   61.61%
    Req/Sec   102.09k    20.60k  142.86k    47.62%
  964738 requests in 5.10s, 213.45MB read
Requests/sec: 189056.92
Transfer/sec:     41.83MB
```

```
Running 5s test @ http://127.0.0.1:9999
  2 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   315.72us  115.11us   1.89ms   69.38%
    Req/Sec   134.30k     8.42k  152.24k    70.00%
  1334778 requests in 5.04s, 295.32MB read
Requests/sec: 265087.32
Transfer/sec:     58.65MB

```


### Connection: close

```
Running 5s test @ http://127.0.0.1:9999
  2 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   508.16us  312.69us   2.44ms   64.82%
    Req/Sec    34.19k     1.95k   37.53k    63.00%
  340156 requests in 5.03s, 73.64MB read
Requests/sec:  67645.70
Transfer/sec:     14.64MB
```

```
Running 5s test @ http://127.0.0.1:9999
  2 threads and 10000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    32.23ms   21.97ms 292.59ms   80.93%
    Req/Sec    30.46k     1.62k   34.25k    65.38%
  281007 requests in 5.02s, 60.83MB read
Requests/sec:  55922.15
Transfer/sec:     12.11MB
```

it uses 16MB of memory.

# Others
when debugging, i find while result with `Connection: close` is normal, it can only handle ~200 requests/s with `Connection: keep-alive`.

 Investigation shows that it's caused by nagle congestion control algorithm. Disabling it (`main.cpp:38`) makes it capable of handling ~200k requests/s.

# References

- manual page of `epoll`
- Computer System A Programmer's Perspective Chapter 7
