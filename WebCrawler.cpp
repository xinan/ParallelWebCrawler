//
// Created by Liu Xinan on 24/9/16.
//

#include <string>
#include <iostream>
#include <cassert>
#include "WebCrawler.h"
#include "HttpRequest.h"
#include "ThreadPool.h"


const std::chrono::microseconds WebCrawler::CRAWLING_DELAY = std::chrono::microseconds(500);

WebCrawler::WebCrawler(const int target_amount, const std::vector<std::string>& starting_urls)
        : target_amount_(target_amount) {
    for (const auto& url : starting_urls) {
        const Link link(url);
        pending_links_[link.getHost()].insert(link.getUrl());
    }

    for (const auto& entry : pending_links_) {
        domain_queue_.push(entry.first);
    }
}

void WebCrawler::start() {
    fprintf(stderr, "Starting a thread pool of %zu threads.\n", number_of_threads_);
    ThreadPool pool(number_of_threads_);

    const auto crawl_job = [this](const std::string& hostname, const std::unordered_set<Link>& links) {
        assert(links.begin() != links.end());

        // Set host and port from the first link and try to open the connection.
        const Link first_link = *links.begin();
        HttpRequest request(first_link.getHost(), first_link.getPort());
        try {
            request.open();
        } catch (std::string& e) {
            return;
        }

        std::unordered_map<std::string, std::unordered_set<Link>> results;

        for (const auto& link : links) {
            if (link.getProtocol() != "http") {
                // Skip non-http urls.
                continue;
            }

            {  // Acquire lock.
                std::unique_lock<std::mutex> lock(this->lock_);

                // Not crawling the same url more than once.
                if (this->visited_.find(link) != this->visited_.end()) {
                    // Skip a link if we have already visited it.
                    continue;
                }

                // Stop then target amount achieved.
                if (this->results_.size() >= this->target_amount_) {
                    return;
                }

                // Add candidate into visited set.
                this->visited_.insert(link);
                fprintf(stderr, "[%3lu%%] Crawling %s\n", this->results_.size() * 100 / this->target_amount_, link.getUrl().c_str());
            }  // Release lock.

            try {
                // Crawl this page and put all its links into the result buffer.
                const WebPage page = request.get(link.getPath());

                // We only care about response code 2xx.
                if (page.getResponseCode()[0] != '2') {
                    continue;
                }

                for (const auto& new_link : page.getLinks()) {
                    results[new_link.first].insert(new_link.second.begin(), new_link.second.end());
                }
                std::this_thread::sleep_for(this->CRAWLING_DELAY);
            } catch (const std::string& e) {
                break;
            }
        }

        const auto response_time = request.getAverageResponseTimeMs();

        if (response_time.count() == 0) {
            return;
        }

        {  // Acquire lock.
            std::unique_lock<std::mutex> lock(this->lock_);

            for (const auto& result : results) {
                // We are not interested in crawling this host one more time.
                if (this->results_.find(result.first) != this->results_.end()) {
                    continue;
                }

                if (this->pending_links_.find(result.first) == this->pending_links_.end()) {
                    this->domain_queue_.push(result.first);
                }

                this->pending_links_[result.first].insert(result.second.cbegin(), result.second.cend());
            }

            this->results_[hostname] = response_time;

            // Notify the main thread that their might be new items pending.
            if (results.size() > 0) {
                this->condition_.notify_all();
            }
        }  // Release lock.
    };

    while (true) {
        {  // Acquire lock.
            std::unique_lock<std::mutex> lock(lock_);

            if (results_.size() >= target_amount_) {
                break;
            }

            while (domain_queue_.empty()) {
                condition_.wait(lock);
            }

            const std::string domain = domain_queue_.front();
            domain_queue_.pop();
            pool.enqueue(crawl_job, domain, pending_links_[domain]);
            pending_links_.erase(domain);
        }  // Release lock.
    }

    fprintf(stderr, "[100%%] Crawling done. Shutting down threads...\n");
    pool.stop();

    // Print results.
    for (const auto& result : results_) {
        printf("http://%s: %llims\n", result.first.c_str(), result.second.count());
    }
}
