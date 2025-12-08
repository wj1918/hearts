#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>

namespace hearts {

// Pre-computed binomial coefficient lookup table for thread-safe access
class BinomialLookup {
public:
    static BinomialLookup& getInstance() {
        static BinomialLookup instance;
        return instance;
    }

    uint64_t choose(int n, int k) const {
        if (k > n || k < 0 || n < 0) return 0;
        if (n >= MAX_N || k >= MAX_K) {
            // Fall back to computation for large values
            return computeChoose(n, k);
        }
        return lookup_[n][k];
    }

private:
    static constexpr int MAX_N = 64;
    static constexpr int MAX_K = 64;
    uint64_t lookup_[MAX_N][MAX_K];

    BinomialLookup() {
        // Initialize lookup table
        for (int n = 0; n < MAX_N; ++n) {
            for (int k = 0; k < MAX_K; ++k) {
                if (k > n) {
                    lookup_[n][k] = 0;
                } else {
                    lookup_[n][k] = computeChoose(n, k);
                }
            }
        }
    }

    static uint64_t computeChoose(int n, int k) {
        if (k > n) return 0;
        if (k == 0 || k == n) return 1;
        if (k > n - k) k = n - k;  // Optimization

        long double accum = 1;
        for (int i = 1; i <= k; i++) {
            accum = accum * (n - k + i) / i;
        }
        return static_cast<uint64_t>(accum + 0.5);
    }

    // Delete copy/move constructors
    BinomialLookup(const BinomialLookup&) = delete;
    BinomialLookup& operator=(const BinomialLookup&) = delete;
};

// Thread-safe task result wrapper
template<typename T>
class TaskResult {
public:
    TaskResult() : ready_(false) {}

    void set(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ = std::move(value);
        ready_ = true;
        cv_.notify_all();
    }

    T get() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return ready_; });
        return value_;
    }

    bool isReady() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return ready_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    T value_;
    bool ready_;
};

// Work-stealing thread pool
class ThreadPool {
public:
    static ThreadPool& getInstance() {
        static ThreadPool instance;
        return instance;
    }

    // Submit a task and get a future for the result
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("submit on stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        return result;
    }

    // Submit multiple tasks and wait for all to complete
    template<typename F>
    std::vector<std::future<typename std::result_of<F()>::type>>
    submitBatch(std::vector<F>& tasks) {
        using return_type = typename std::result_of<F()>::type;
        std::vector<std::future<return_type>> futures;
        futures.reserve(tasks.size());

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            for (auto& task : tasks) {
                auto packaged = std::make_shared<std::packaged_task<return_type()>>(std::move(task));
                futures.push_back(packaged->get_future());
                tasks_.emplace([packaged]() { (*packaged)(); });
            }
        }
        condition_.notify_all();
        return futures;
    }

    size_t getThreadCount() const { return workers_.size(); }

    // Get number of pending tasks
    size_t getPendingCount() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    ThreadPool() : stop_(false) {
        size_t num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;  // Fallback

        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });
                        if (stop_ && tasks_.empty()) {
                            return;
                        }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    // Delete copy/move constructors
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};

// Completion queue for collecting results as they become ready
template<typename T>
class CompletionQueue {
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(value));
        cv_.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    bool tryPop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
};

}  // namespace hearts

#endif  // THREAD_POOL_H
