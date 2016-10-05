//
// Created by Liu Xinan on 24/9/16.
//

#ifndef PARALLELWEBCRAWLER_THREADPOOL_H
#define PARALLELWEBCRAWLER_THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <future>


class ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex lock_;
    std::condition_variable condition_;
    bool should_stop_ = false;
public:
    /**
     * Create a thread pool of a specified size.
     *
     * @param number_of_threads The number of threads that the thread pool should have.
     * @return A thread pool.
     */
    ThreadPool(size_t number_of_threads);

    /**
     * Adds a task into the queue.
     *
     * @param f A function to execute.
     * @param args Arguments for the function.
     * @return The return value of the function.
     */
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

    /**
     * Stops the thread pool.
     */
    void stop();

    /**
     * Destructs the thread pool. Stops all workers.
     */
    ~ThreadPool();
};


template<class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;
    const auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> return_value = task->get_future();

    {  // Acquire lock.
        std::unique_lock<std::mutex> lock(lock_);

        // If the thread pool has already stopped, throw an exception.
        if (should_stop_) {
            throw std::runtime_error("The thread pool has already stopped.");
        }

        tasks_.emplace([task] { (*task)(); });
    }  // Release lock.

    condition_.notify_one();

    return return_value;
}


#endif //PARALLELWEBCRAWLER_THREADPOOL_H
