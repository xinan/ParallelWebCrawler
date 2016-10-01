//
// Created by Liu Xinan on 23/9/16.
//

#include <algorithm>
#include <regex>
#include <iostream>
#include "Link.h"

const std::regex Link::FULL_URL_RE = std::regex("(\\w+):\\/\\/([A-z0-9\\-\\.]+)(:([0-9]+))?(\\/.*)?", std::regex::icase);
const std::regex Link::PARTIAL_URL_RE = std::regex("(/?.*)");

Link::Link(const std::string& url, const std::string& referrer_url) {
    std::smatch matches;
    if (std::regex_match(url, matches, FULL_URL_RE)) {
        protocol_ = matches[1].str();
        host_ = matches[2].str();
        port_ = matches[4].str();
        path_ = matches[5].str();
    } else if (!referrer_url.empty() && std::regex_match(url, matches, PARTIAL_URL_RE)) {
        // A partial URL is either absolute or relative but does not have a domain name.
        // So we get infer it from the referrer.
        Link referrer(referrer_url);
        protocol_ = referrer.getProtocol();
        host_ = referrer.getHost();
        port_ = referrer.getPort();
        path_ = matches[1].str();
        if (path_[0] != '/') {
            path_ = referrer.getPath().substr(0, referrer.getPath().rfind('/') + 1) + path_;
        }
    } else {
        fprintf(stderr, "Invalid url supplied: %s\n", url.c_str());
        throw std::string("Invalid url.");
    }

    // Make protocol and host name both lowercase.
    std::transform(protocol_.cbegin(), protocol_.cend(), protocol_.begin(), ::tolower);
    std::transform(host_.cbegin(), host_.cend(), host_.begin(), ::tolower);

    if (port_.empty()) {
        port_ = "80";
    }

    if (path_.empty()) {
        path_ = "/";
    }

    url_ = protocol_ + "://" + host_ + ":" + port_ + path_;
}

std::string Link::getProtocol() const {
    return protocol_;
}

std::string Link::getHost() const {
    return host_;
}

std::string Link::getPort() const {
    return port_;
}

std::string Link::getPath() const {
    return path_;
}

std::string Link::getUrl() const {
    return url_;
}

std::string Link::getBaseUrl() const {
    return protocol_ + "://" + host_;
}

bool Link::operator==(const Link &rhs) const {
    return url_ == rhs.url_;
}

bool Link::operator!=(const Link &rhs) const {
    return !(rhs == *this);
}
