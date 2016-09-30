//
// Created by Liu Xinan on 24/9/16.
//

#ifndef PARALLELWEBCRAWLER_WEBCRAWLER_H
#define PARALLELWEBCRAWLER_WEBCRAWLER_H


#include <algorithm>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <queue>
#include "Link.h"


class WebCrawler {
private:
    static const std::chrono::microseconds CRAWLING_DELAY;

    const int target_amount_;
    size_t number_of_threads_ = std::max(std::thread::hardware_concurrency() * 8, 64U);

    std::unordered_map<std::string, std::unordered_set<Link>> pending_links_;
    std::queue<std::string> domain_queue_;
    std::unordered_map<std::string, std::chrono::milliseconds> results_;
    std::unordered_set<Link> visited_;

    std::mutex lock_;
    std::condition_variable condition_;
public:
    /**
     * Create a WebCrawler given a list of starting urls.
     *
     * @param starting_urls
     * @return
     */
    WebCrawler(const int target_amount, const std::vector<std::string>& starting_urls);

    /**
     * Start the crawling.
     */
    void start();
};


#endif //PARALLELWEBCRAWLER_WEBCRAWLER_H
