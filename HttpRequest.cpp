//
// Created by Liu Xinan on 23/9/16.
//

#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cassert>
#include <cstring>
#include "HttpRequest.h"


const std::regex HttpRequest::CONTENT_LENGTH_RE = std::regex("Content-Length: (\\d+)\r\n");
const std::regex HttpRequest::CONNECTION_CLOSE_RE = std::regex("Connection: close\r\n");
const std::regex HttpRequest::CHUNKED_ENCODING_RE = std::regex("Transfer-Encoding: chunked\r\n");
const size_t HttpRequest::BUFFER_SIZE = 1024;

HttpRequest::HttpRequest(const std::string& hostname, const std::string& port)
        : hostname_(hostname), port_(port) {}

void HttpRequest::open() {
    addrinfo hints;
    addrinfo *result, *host;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Resolve the hostname.
    if (getaddrinfo(hostname_.c_str(), port_.c_str(), &hints, &result) != 0) {
        fprintf(stderr, "Error resolving hostname: %s at port %s\n", hostname_.c_str(), port_.c_str());
        throw std::string("Hostname resolution failed.");
    }


    // Find the first DNS record that we can connect to.
    for (host = result; host != NULL; host = host->ai_next) {
        if ((sock_ = socket(host->ai_family, host->ai_socktype, host->ai_protocol)) == -1) {
            continue;
        } else {
            // Set socket to non-blocking so that we can use select() to set a timeout.
            set_socket_blocking_(false);
            if (connect(sock_, host->ai_addr, host->ai_addrlen) < 0) {
                // For a non-blocking socket, connect() returns immediately and sets errno to EINPROGRESS.
                if (errno == EINPROGRESS) {
                    FD_ZERO(&fds_);
                    FD_SET(sock_, &fds_);
                    if (select(sock_ + 1, nullptr, &fds_, nullptr, &timeout_) > 0) {
                        int error;
                        socklen_t error_len = sizeof(error);
                        // A failed socket is also writable. So we need to check the socket options.
                        if (getsockopt(sock_, SOL_SOCKET, SO_ERROR, &error, &error_len) >= 0 && !error) {
                            // Set it back to blocking mode.
                            set_socket_blocking_(true);
                            // Set timeout for socket read/write.
                            setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_));
                            setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, &timeout_, sizeof(timeout_));
                            break;
                        }
                    }
                }
            }
            close(sock_);
        }
    }

    freeaddrinfo(result);

    if (host == nullptr) {
        fprintf(stderr, "Error connecting to host: %s at port %s\n", hostname_.c_str(), port_.c_str());
        throw std::string("Connection failed.");
    }
}

WebPage HttpRequest::get(const std::string& path) {
    ++requests_made_;

    {  // Send the GET request header to the server.
        std::string request_header = constructGetHeader_(path);

        write_(request_header);
    }

    auto send_time = std::chrono::steady_clock::now();

    // Read the header only, to decide what to do.
    std::string response = readHeader_();

    auto receive_time = std::chrono::steady_clock::now();
    total_response_time_ += std::chrono::duration_cast<std::chrono::milliseconds>(receive_time - send_time);

    bool chunked_encoding = false;

    std::smatch matches;
    size_t content_length = 0;
    if (std::regex_search(response, matches, CONNECTION_CLOSE_RE)) {
        throw std::string("Connection closed.");
    } else if (std::regex_search(response, matches, CONTENT_LENGTH_RE)) {
        content_length = (size_t) std::stoi(matches[1]);
    } else if (std::regex_search(response, matches, CHUNKED_ENCODING_RE)) {
        chunked_encoding = true;
    } else {
        throw std::string("No content length nor chunked encoding.");
    }

    if (chunked_encoding) {
        response += readChunked_();
    } else {
        response += readLength_(content_length);
    }

    std::string url = "http://" + hostname_ + path;
    return WebPage(url, response);
}

void HttpRequest::set_socket_blocking_(bool blocking) {
    long flag = fcntl(sock_, F_GETFL, nullptr);
    if (blocking) {
        flag &= ~O_NONBLOCK;
    } else {
        flag |= O_NONBLOCK;
    }
    fcntl(sock_, F_SETFL, flag);
}

void HttpRequest::write_(std::string &data) {
    if (send(sock_, data.c_str(), data.size(), 0) != data.size()) {
        fprintf(stderr, "Cannot send request to host: %s\n", hostname_.c_str());
        throw std::string("Cannot send request.");
    }
}

size_t HttpRequest::read_(size_t length) {
    ssize_t bytes_read = recv(sock_, buffer_, length, 0);
    if (bytes_read <= 0) {
        fprintf(stderr, "Cannot read response from host: %s\n", hostname_.c_str());
        throw std::string("Cannot read response");
    }
    return (size_t) bytes_read;
}

std::string HttpRequest::readHeader_() {
    std::string header;
    while (true) {
        read_(1);
        header += buffer_[0];
        if (header.length() > 4 && header.substr(header.length() - 4) == "\r\n\r\n") {
            break;
        }
    }
    return header;
}

std::string HttpRequest::readChunked_() {
    std::string content;
    std::string preamble;
    size_t chunk_size;
    while (true) {
        preamble = "";
        while (true) {
            read_(1);
            preamble += buffer_[0];
            if (preamble.length() > 2 && preamble.substr(preamble.length() - 2) == "\r\n") {
                break;
            }
        }
        chunk_size = (size_t) std::stoi(preamble.substr(0, preamble.length() - 2), nullptr, 16);
        if (chunk_size == 0) {
            break;
        }
        content += readLength_(chunk_size);
        readLength_(2);
    }
    return content;
}

std::string HttpRequest::readLength_(size_t length) {
    size_t total_read = 0;
    size_t bytes_read;
    std::string content;
    while (total_read < length) {
        bytes_read = read_(std::min(BUFFER_SIZE, length - total_read));
        assert(bytes_read > 0);
        content.append(buffer_, bytes_read);
        total_read += bytes_read;
    }
    return content;
}

std::string HttpRequest::constructGetHeader_(const std::string &path) {
    std::string header = "GET " + path + " HTTP/1.1\r\n";
    // Host is always required for HTTP/1.1.
    header += "Host: " + hostname_ + "\r\n";
    // I only accept text, don't send me gzip or any other media type.
    header += "Accept: text/html,application/xhtml+xml,application/xml\r\n";
    // Blame the school if you are unhappy with this crawler.
    header += "User-Agent: Mozilla/5.0 (compatible; Homework/0.1; +https://myaces.nus.edu.sg/cors/jsp/report/ModuleDetailedInfo.jsp?acad_y=2016/2017&sem_c=2&mod_c=CS3103)\r\n";
    // Use persistent connection.
    header += "Connection: keep-alive\r\n";
    header += "\r\n";
    return header;
}

std::chrono::milliseconds HttpRequest::getAverageResponseTimeMs() {
    if (requests_made_ == 0) {
        return std::chrono::milliseconds(0);
    }
    return total_response_time_ / requests_made_;
}

HttpRequest::~HttpRequest() {
    close(sock_);
}
