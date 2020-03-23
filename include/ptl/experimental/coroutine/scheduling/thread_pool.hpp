#pragma once

#include <experimental/coroutine>
#include <thread>
#include "ptl/mpmc_queue.hpp"

struct dispatch_queue {
    dispatch_queue(size_t capacity) : queue_(capacity) {}
    bool get(std::experimental::coroutine_handle<>& item)
    {
        return queue_.try_pop(item);
    }

private:
    ptl::queue<std::experimental::coroutine_handle<>, ptl::Lockless::MPMC> queue_;
};

struct thread {
    thread()
        : thread_()
        , queue_(25)
    {
        self_ = this;
    }

protected:
    bool execute_one()
    {
        std::experimental::coroutine_handle<> coroutine;
        if (queue_.get(coroutine)) {
            coroutine.resume();
            return true;
        }
        return false;
    }

    bool steal_one(thread& other)
    {
        return other.execute_one();

    }
    void execution()
    {
        while (!finished_) {

        }
    }


private:
    static inline thread_local thread* self_;


    bool finished_;
    std::thread thread_;
    dispatch_queue queue_;

};

class thread_pool {

};