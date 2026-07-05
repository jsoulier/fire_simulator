#pragma once

#include <cassert>
#include <chrono>
#include <future>
#include <utility>

template<typename T>
class Future
{
public:
    Future() = default;

    Future(std::future<T>&& future)
        : Handle{std::move(future)}
    {
    }

    Future& operator=(std::future<T>&& future)
    {
        Handle = std::move(future);
        return *this;
    }

    bool IsReady() const
    {
        static constexpr std::chrono::seconds kTime{0};
        return Handle.valid() && Handle.wait_for(kTime) == std::future_status::ready;
    }

    T Get()
    {
        assert(IsReady());
        return Handle.get();
    }

private:
    std::future<T> Handle;
};
