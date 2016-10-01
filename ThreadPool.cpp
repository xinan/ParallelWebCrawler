//
// Created by Liu Xinan on 24/9/16.
//
// Adapted from https://github.com/progschj/ThreadPool/
//

#include "ThreadPool.h"


ThreadPool::ThreadPool(size_t number_of_threads) {
    auto worker_thread = [this] {
        std::function<void()> task;

        while (true) {
            {  // Acquire lock.
                std::unique_lock<std::mutex> lock(lock_);

                // Wait until we should stop, or task queue is not empty.
                condition_.wait(lock, [this]() { return this->should_stop_ || !this->tasks_.empty(); });

                // If we should stop, we abandon the rest of the task queue and just return.
                if (should_stop_) {
                    break;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
            }  // Release lock.

            // Execute the task.
            task();
        }
        condition_.notify_all();
    };

    // Create a vector of worker threads.
    for (size_t i = 0; i < number_of_threads; ++i) {
        workers_.emplace_back(worker_thread);
    }
}

void ThreadPool::stop() {
    {  // Acquire lock.
        std::unique_lock<std::mutex> lock(lock_);

        should_stop_ = true;
    }  // Release lock.

    condition_.notify_all();
    for (auto& worker : workers_) {
        worker.join();
    }

    workers_.clear();
}

ThreadPool::~ThreadPool() {
    {  // Acquire lock.
        std::unique_lock<std::mutex> lock(lock_);

        should_stop_ = true;
    }  // Release lock.

    condition_.notify_all();
    for (auto& worker : workers_) {
        worker.join();
    }
}
