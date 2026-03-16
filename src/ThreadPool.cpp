#include "../include/ThreadPool.hpp"
#include <mutex>
#include <utility>

ThreadPool::ThreadPool(size_t numThreads) : running(true) {
    for (size_t i = 0; i < numThreads; i++) {
        workers.emplace_back([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    cv.wait(lock, [this]() {
                        return !tasks.empty() || !running;
                    });
                    if (!running && tasks.empty()) return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    running = false;
    cv.notify_all();
    for (auto &t : workers)
        if (t.joinable()) t.join();
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(std::move(task));
    }
    cv.notify_all();
}
