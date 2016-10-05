# ParallelWebCrawler
A multi-threaded web crawler written in C++11.

## Build
```
mkdir build
cd build
cmake ..
make
```

## Usage
```
./ParallelWebCrawler <target amount> <seed file>
```

## Highlights
* Logs messages to stderr, outputs to stdout, easy to redirect output as a file.
* Constructed proper HTTP/1.1 headers, sent using basic socket library.
* Request time is controlled by some crawling delay.
* Crawler stops after the target amount of base urls and their response times are collected.
* The crawler is multi-threaded. I created a thread pool for that.
* Each url is only visited once.
* Well documented. Exceptions handled.
* Implemented HTTP/1.1 chunked encoding handling.
* Uses persistent connection to crawl multiple pages on the same host with a single connection to reduce overhead.
* Does bread-first search on domains.
* Using const references while I can.
* Have timeouts for socket connection, as well as read and write.
* Modularized and object-oriented.
* Synchronization of threads using C++11 `std::mutex` and `std::condition_variable`.
