#pragma once

namespace ptl::asio {

struct descriptor {
    // posix (or posix-like) specific
    using native_type = int;
    native_type native_descriptor() const { return descriptor_; }

protected:
    descriptor(native_type descriptor) : descriptor_(descriptor) {}
    native_type descriptor_;
};


}
