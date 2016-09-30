//
// Created by Liu Xinan on 23/9/16.
//

#ifndef PARALLELWEBCRAWLER_WEBPAGE_H
#define PARALLELWEBCRAWLER_WEBPAGE_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include "Link.h"

class WebPage {
private:
    static const std::regex RESPONSE_CODE_RE;
    static const std::regex URL_RE;

    std::string url_;
    std::string responseCode_;
    std::string html_;
    std::unordered_map<std::string, std::unordered_set<Link>> links_;

    void parseLinks_();

public:

    /**
     * Constructs a WebPage object from the response.
     *
     * @param response The HTTP GET response.
     * @return A WebPage object.
     */
    WebPage(const std::string& link, const std::string& response);

    /**
     * Gets the response code.
     *
     * @return The response code in string.
     */
    const std::string& getResponseCode() const;

    /**
     * Parses and find out all links in the page.
     *
     * @return A map of host -> set of links under the host.
     */
    const std::unordered_map<std::string, std::unordered_set<Link>>& getLinks() const;
};


#endif //PARALLELWEBCRAWLER_WEBPAGE_H
