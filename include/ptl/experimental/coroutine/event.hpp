#pragma once

namespace ptl::experimental::coroutine {

namespace detail {

}

class async_event {
public:


private:
    mutable std::atomic<void*> state_;
};

}