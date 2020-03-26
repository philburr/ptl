#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>
#include <functional>

// FIXME: find a home:
template <typename LOCK, typename LAMBDA>
void synchronize(LOCK& lock, LAMBDA lambda)
{
    std::scoped_lock sl(lock);
    lambda();
}

namespace ptl::execution {

class Executor
{
public:
    Executor() { current = this; }
    virtual ~Executor() { current = nullptr; }

    virtual void add(std::function<void()> fn) = 0;


    static inline thread_local Executor* current;
};

class QueuedExecutor : public Executor
{
public:
    virtual ~QueuedExecutor() {}

    void run() {
        while (true) {
            std::function<void()> fn;

            {
                std::unique_lock<std::mutex> sl(lock_);
                cv_.wait(sl, [&](){ return finished_ || !work_.empty(); });

                if (work_.empty() && finished_) {
                    return;
                }

                fn = work_.front();
                work_.pop();
            }

            fn();
        }
    }

    void add(std::function<void()> fn) override {
        synchronize(lock_, [&](){
            work_.push(fn);
        });
        cv_.notify_one();
    }

    void stop() {
        synchronize(lock_, [&](){
            finished_ = true;
        });
        cv_.notify_one();
    }

private:
    std::mutex lock_;
    std::condition_variable cv_;

    bool finished_ = false;
    std::queue<std::function<void()>> work_;
};

} // namespace ptl::execution

