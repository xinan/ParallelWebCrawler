//
// Created by Liu Xinan on 23/9/16.
//

#ifndef PARALLELWEBCRAWLER_REQUEST_H
#define PARALLELWEBCRAWLER_REQUEST_H

#include <string>
#include <chrono>
#include <regex>
#include "WebPage.h"


class HttpRequest {
private:
    static const std::regex CONTENT_LENGTH_RE;
    static const std::regex CONNECTION_CLOSE_RE;
    static const std::regex CHUNKED_ENCODING_RE;
    static const size_t BUFFER_SIZE;

    const std::string hostname_;
    const std::string port_;

    struct timeval timeout_ = { .tv_sec = 1, .tv_usec = 0 };
    fd_set fds_;

    int sock_ = -1;
    std::chrono::milliseconds total_response_time_ = std::chrono::milliseconds(0);
    uint32_t requests_made_ = 0;

    char buffer_[1024];

    void set_socket_blocking_(bool blocking);
    void write_(std::string &data);
    size_t read_(size_t length);
    std::string readHeader_();
    std::string readChunked_();
    std::string readLength_(size_t length);
    std::string constructGetHeader_(const std::string &path);
public:
    /**
     * Construct a Request object to a host.
     *
     * @param host The host to connect to.
     * @return A new Request object.
     */
    HttpRequest(const std::string& hostname, const std::string& port);

    /**
     * Open the connection to the host.
     */
    void open();

    /**
     * Requests a webpage.
     *
     * @param path The path to GET.
     * @return
     */
    WebPage get(const std::string& path);

    /**
     * Gets the average response time for the connection. Calculated using (cumulated response time) / (number of requests made).
     *
     * @return The average response time in std::chrono::milliseconds.
     */
    std::chrono::milliseconds getAverageResponseTimeMs();

    /**
     * Destructs the request object. Close the socket.
     */
    ~HttpRequest();
};


#endif //PARALLELWEBCRAWLER_REQUEST_H
