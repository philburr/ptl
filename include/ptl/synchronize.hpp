#pragma once

namespace ptl {

struct null_lock {
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