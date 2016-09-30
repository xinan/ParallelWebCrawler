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
    for (auto url : starting_urls) {
        Link link(url);
        pending_links_[link.getHost()].insert(link.getUrl());
    }

    for (auto entry : pending_links_) {
        domain_queue_.push(entry.first);
    }
}

void WebCrawler::start() {
    fprintf(stderr, "Starting a thread pool of %zu threads.\n", number_of_threads_);
    ThreadPool pool(number_of_threads_);

    auto crawl_job = [this](const std::string& hostname, const std::unordered_set<Link>& links) {
        assert(links.begin() != links.end());

        Link first_link = *links.begin();
        HttpRequest request(first_link.getHost(), first_link.getPort());
        try {
            request.open();
        } catch (std::string& e) {
            return;
        }

        std::unordered_map<std::string, std::unordered_set<Link>> results;

        for (auto& link : links) {
            if (link.getProtocol() != "http") {
                // Skip non-http urls.
                continue;
            }

            {  // Acquire lock.
                std::unique_lock<std::mutex> lock(this->lock_);

                if (this->visited_.find(link) != this->visited_.end()) {
                    // Skip a link if we have already visited it.
                    continue;
                }

                if (this->results_.size() >= this->target_amount_) {
                    return;
                }

                fprintf(stderr, "[%3lu%%] Crawling %s\n", this->results_.size() * 100 / this->target_amount_, link.getUrl().c_str());
            }  // Release lock.

            try {
                // Crawl this page and put all its links into the result buffer.
                WebPage page = request.get(link.getPath());
                for (auto new_link : page.getLinks()) {
                    results[new_link.first].insert(new_link.second.begin(), new_link.second.end());
                }
                std::this_thread::sleep_for(this->CRAWLING_DELAY);
            } catch (const std::string& e) {
                break;
            }
        }

        auto response_time = request.getAverageResponseTimeMs();

        if (response_time.count() == 0) {
            return;
        }

        {  // Acquire lock.
            std::unique_lock<std::mutex> lock(this->lock_);

            for (auto result : results) {
                if (this->pending_links_.find(result.first) == this->pending_links_.end()) {
                    this->domain_queue_.push(result.first);
                }
                this->pending_links_[result.first].insert(result.second.cbegin(), result.second.cend());
            }

            this->results_[hostname] = response_time;

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

            if (domain_queue_.empty()) {
                condition_.wait(lock);
            }

            std::string domain = domain_queue_.front();
            domain_queue_.pop();
            pool.enqueue(crawl_job, domain, pending_links_[domain]);
            pending_links_.erase(domain);
        }  // Release lock.
    }

    fprintf(stderr, "Crawling done. Shutting down threads...\n");
    pool.stop();

    for (auto result : results_) {
        printf("http://%s: %llims\n", result.first.c_str(), result.second.count());
    }
}
