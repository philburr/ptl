#pragma once

namespace ptl::experimental::coroutine::iosvc {

struct descriptor {
    // posix (or posix-like) specific
    using native_type = int;
    native_type native_descriptor() const { return descriptor_; }
    descriptor(native_type descriptor) : descriptor_(descriptor) {}

protected:
    native_type descriptor_;
};


}
