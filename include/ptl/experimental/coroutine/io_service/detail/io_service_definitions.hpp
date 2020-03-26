#pragma once
#include <functional>

namespace ptl::experimental::coroutine::iosvc {

struct descriptor;
struct descriptor_service_data;

enum class io_kind : int {
    none,
    read,
    write,

    io_kind_count,
};

struct io_service_operation {
    virtual void work() = 0;
};

namespace detail {

template<typename T, T>
struct bound_io_service_operation;

template<typename T, typename R, R(T::*mfn)()>
struct bound_io_service_operation<R(T::*)(), mfn> : iosvc::io_service_operation {
    bound_io_service_operation(T& obj) : obj_(obj) {}

    void work() override
    {
        (obj_.*mfn)();
    }

    T& obj_;
};


}
}