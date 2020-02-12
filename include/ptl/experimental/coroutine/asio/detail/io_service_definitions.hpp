#pragma once

namespace ptl::experimental::coroutine::asio {

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

}