//
// Created by Liu Xinan on 23/9/16.
//

#include <regex>
#include <iostream>
#include "WebPage.h"


const std::regex WebPage::RESPONSE_CODE_RE = std::regex("HTTP/\\d.\\d (\\d{3}) [A-Z]+\r\n");
const std::regex WebPage::URL_RE = std::regex("<\\s*A\\s+[^>]*href\\s*=\\s*\"([^\"\\s]*)\"", std::regex::icase);


WebPage::WebPage(const std::string& url, const std::string& response)
        : url_(url) {
    std::string header;
    size_t found;

    // Split the header and the html.
    if ((found = response.find("\r\n\r\n")) != std::string::npos) {
        header = response.substr(0, found);
        html_ = response.substr(found, std::string::npos);
    }

    // Parse the response code.
    std::sregex_token_iterator code_itr(header.begin(), header.end(), RESPONSE_CODE_RE, 1);
    std::sregex_token_iterator end_itr;
    if (code_itr != end_itr) {
        responseCode_ = *code_itr;
    } else {
        responseCode_ = "???";
    }
    if (responseCode_[0] == '2') {
        parseLinks_();
    }
}

const std::string& WebPage::getResponseCode() const {
    return responseCode_;
}

const std::unordered_map<std::string, std::unordered_set<Link>>& WebPage::getLinks() const {
    return links_;
}

void WebPage::parseLinks_() {
    std::sregex_token_iterator url_itr(html_.begin(), html_.end(), URL_RE, 1);
    std::sregex_token_iterator end_itr;
    for (auto itr = url_itr; itr != end_itr; ++itr) {
        std::string url = *itr;
        try {
            Link link = Link(url, url_);
            std::string host = link.getHost();
            links_[host].insert(link);
        } catch (const std::string& e) {
            continue;
        }
    }
}
