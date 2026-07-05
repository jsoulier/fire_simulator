#include <functional>
#include <mutex>
#include <thread>
#include <utility>

#include "worker.hpp"

Worker::Worker()
    : Running{false}
    , Stop{false}
{
    Thread = std::thread([&]()
    {
        while (true)
        {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(Mutex);
                Condition.wait(lock, [this]()
                {
                    return Stop || !Jobs.empty();
                });
                if (Stop && Jobs.empty())
                {
                    return;
                }
                job = std::move(Jobs.front());
                Jobs.pop_front();
                Running = true;
            }
            job();
            {
                std::lock_guard<std::mutex> lock(Mutex);
                Running = false;
            }
        }
    });
}

Worker::~Worker()
{
    {
        std::lock_guard<std::mutex> lock(Mutex);
        Stop = true;
    }
    Condition.notify_all();
    Thread.join();
}

bool Worker::IsRunning()
{
    std::lock_guard<std::mutex> lock(Mutex);
    return Running || !Jobs.empty();
}