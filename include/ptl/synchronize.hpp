#pragma once

namespace ptl {

struct NullLock {
    void lock() {}
    void unlock() {}
};


template <typename LOCK, typename LAMBDA>
void synchronize(LOCK& lock, LAMBDA lambda)
{
    std::scoped_lock sl(lock);
    lambda();
}

}