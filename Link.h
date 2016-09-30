//
// Created by Liu Xinan on 23/9/16.
//

#ifndef PARALLELWEBCRAWLER_LINK_H
#define PARALLELWEBCRAWLER_LINK_H

#include <string>
#include <regex>


class Link {
private:
    static const std::regex FULL_URL_RE;
    static const std::regex PARTIAL_URL_RE;

    std::string url_;
    std::string protocol_;
    std::string host_;
    std::string port_;
    std::string path_;

public:
    /**
     * Construct a Link object given the url.
     *
     * @param url The url in string.
     * @return A Link object.
     */
    Link(const std::string& url, const std::string& referrer = "");

    /**
     * Gets the protocol of the url.
     *
     * @return The protocol in string.
     */
    std::string getProtocol() const;

    /**
     * Gets the domain of the url.
     *
     * @return The host domain in string.
     */
    std::string getHost() const;

    /**
     * Gets the port of the host in the url.
     *
     * @return The port number in string.
     */
    std::string getPort() const;

    /**
     * Gets the path of the resource.
     *
     * @return The path part of the url.
     */
    std::string getPath() const;

    /**
     * Gets the normalized url.
     *
     * @return The url, normalized.
     */
    std::string getUrl() const;

    /**
     * Gets the base url which is protocol://domain.name/.
     *
     * @return
     */
    std::string getBaseUrl() const;

    bool operator==(const Link &rhs) const;

    bool operator!=(const Link &rhs) const;
};

namespace std {
    template <>
    class hash<Link> {
    public:
        long operator()(const Link& obj) const {
            return std::hash<std::string>()(obj.getUrl());
        }
    };

    template <>
    class equal_to<Link> {
    public:
        bool operator()(const Link& lhs, const Link& rhs) const {
            return lhs.getUrl() == rhs.getUrl();
        }
    };
}


#endif //PARALLELWEBCRAWLER_LINK_H
