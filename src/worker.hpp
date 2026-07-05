#pragma once

#include <SDL3/SDL_assert.h>

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

#include "future.hpp"

class Worker
{
public:
    Worker();
    ~Worker();
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    template<typename Function>
    Future<std::invoke_result_t<Function>> Submit(Function function);
    bool IsRunning();

private:
    std::thread Thread;
    std::mutex Mutex;
    std::condition_variable Condition;
    std::deque<std::function<void()>> Jobs;
    bool Running;
    bool Stop;
};

template<typename Function>
Future<std::invoke_result_t<Function>> Worker::Submit(Function function)
{
    auto task = std::make_shared<std::packaged_task<std::invoke_result_t<Function>()>>(std::move(function));
    std::future<std::invoke_result_t<Function>> future = task->get_future();
    {
        std::lock_guard<std::mutex> lock(Mutex);
        Jobs.emplace_back([task]() { (*task)(); });
    }
    Condition.notify_one();
    return future;
}
