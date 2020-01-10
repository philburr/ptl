#pragma once
#include <type_traits>
#include <atomic>
#include "ptl/compiler.hpp"

namespace ptl {

template<typename T>
class intrusive_ptr;

template<typename T>
class intrusive_wptr
{
public:
    using element_type = T;

    constexpr intrusive_wptr() noexcept
        : ptr_(nullptr)
    {}

    ~intrusive_wptr()
    {
        if (ptr_ != nullptr) {
            if constexpr (has_intrusive_ptr_api::value) {
                ptr_->intrusive_wptr_release();
            } else {
                intrusive_wptr_release(ptr_);
            }
        }
    }

    // copy
    intrusive_wptr(const intrusive_wptr& other)
        : ptr_(other.ptr_)
    {
        if (ptr_ != nullptr) {
            if constexpr (has_intrusive_ptr_api::value) {
                ptr_->intrusive_wptr_add_ref();
            } else {
                intrusive_wptr_add_ref(ptr_);
            }
        }
    }
    intrusive_wptr& operator=(const intrusive_wptr& other)
    {
        intrusive_wptr(other).swap(*this);
        return *this;
    }

    // conversion copy
    template<typename U>
    intrusive_wptr(const intrusive_wptr<U>& other, typename std::enable_if<std::is_convertible_v<U, T>>::type = 0)
        : ptr_(other.ptr_)
    {
        if (ptr_ != nullptr) {
            if constexpr (has_intrusive_ptr_api::value) {
                ptr_->intrusive_wptr_add_ref();
            } else {
                intrusive_wptr_add_ref(ptr_);
            }
        }
    }
    template<typename U, typename = typename std::enable_if<std::is_convertible_v<U, T>>::type>
    intrusive_wptr& operator=(const intrusive_wptr<U>& other)
    {
        intrusive_wptr(other).swap(*this);
        return *this;
    }

    // move
    intrusive_wptr(intrusive_wptr&& other)
        : ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }
    intrusive_wptr& operator=(intrusive_wptr&& other)
    {
        intrusive_wptr(std::move(other)).swap(*this);
        return *this;
    }

    // conversion move
    template<typename U>
    intrusive_wptr(intrusive_wptr<U>&& other, typename std::enable_if<std::is_convertible_v<U, T>>::type = 0)
        : ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }
    template<typename U, typename = typename std::enable_if<std::is_convertible_v<U, T>>::type>
    intrusive_wptr& operator=(intrusive_wptr<U>&& other)
    {
        intrusive_wptr(std::move(other)).swap(*this);
        return *this;
    }

    void swap(intrusive_wptr& other) noexcept
    {
        std::swap(ptr_, other.ptr_);
    }

    void reset()
    {
        intrusive_wptr().swap(*this);
    }

    intrusive_ptr<T> lock();

private:
    friend class intrusive_ptr<T>;

    intrusive_wptr(T* ptr)
        : ptr_(ptr)
    {
        intrusive_wptr_add_ref(ptr_);
    }

    struct has_intrusive_ptr_api {

        template <typename U>
        static constexpr decltype(std::declval<U>().intrusive_ptr_add_ref(), bool()) test_intrusive_ptr_api(int)
        {
            return true;
        }

        template <typename U>
        static constexpr bool test_intrusive_ptr_api(...) {
            return false;
        }

        static constexpr bool value = test_intrusive_ptr_api<T>(int());
    };

    T* ptr_;
};

template<typename T>
class intrusive_ptr
{
public:
    using element_type = T;

    constexpr intrusive_ptr() noexcept
        : ptr_(nullptr)
    {}

    ~intrusive_ptr()
    {
        if (ptr_ != nullptr) {
            if constexpr (has_intrusive_ptr_api::value) {
                ptr_->intrusive_ptr_release();
            } else {
                intrusive_ptr_release(ptr_);
            }
        }
    }

    intrusive_ptr(bool add_ref, T* ptr)
        : ptr_(ptr)
    {
        if (ptr_ != nullptr and add_ref) {
            if constexpr (has_intrusive_ptr_api::value) {
                ptr_->intrusive_ptr_add_ref();
            } else {
                intrusive_ptr_add_ref(ptr_);
            }
        }
    }
    intrusive_ptr(T* ptr)
        : intrusive_ptr(false, ptr)
    {
    }

    intrusive_ptr& operator=(T* ptr)
    {
        intrusive_ptr(ptr).swap(*this);
        return *this;
    }

    // copy
    intrusive_ptr(const intrusive_ptr& other)
        : ptr_(other.ptr_)
    {
        if (ptr_ != nullptr) {
            if constexpr (has_intrusive_ptr_api::value) {
                ptr_->intrusive_ptr_add_ref();
            } else {
                intrusive_ptr_add_ref(ptr_);
            }
        }
    }
    intrusive_ptr& operator=(const intrusive_ptr& other)
    {
        intrusive_ptr(other).swap(*this);
        return *this;
    }

    // conversion copy
    template<typename U>
    intrusive_ptr(const intrusive_ptr<U>& other, typename std::enable_if<std::is_convertible_v<U, T>>::type = 0)
        : ptr_(other.ptr_)
    {
        if (ptr_ != nullptr) {
            if constexpr (has_intrusive_ptr_api::value) {
                ptr_->intrusive_ptr_add_ref();
            } else {
                intrusive_ptr_add_ref(ptr_);
            }
        }
    }
    template<typename U, typename = typename std::enable_if<std::is_convertible_v<U, T>>::type>
    intrusive_ptr& operator=(const intrusive_ptr<U>& other)
    {
        intrusive_ptr(other).swap(*this);
        return *this;
    }

    // move
    intrusive_ptr(intrusive_ptr&& other)
        : ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }
    intrusive_ptr& operator=(intrusive_ptr&& other)
    {
        intrusive_ptr(std::move(other)).swap(*this);
        return *this;
    }

    // conversion move
    template<typename U>
    intrusive_ptr(intrusive_ptr<U>&& other, typename std::enable_if<std::is_convertible_v<U, T>>::type = 0)
        : ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }
    template<typename U, typename = typename std::enable_if<std::is_convertible_v<U, T>>::type>
    intrusive_ptr& operator=(intrusive_ptr<U>&& other)
    {
        intrusive_ptr(std::move(other)).swap(*this);
        return *this;
    }

    operator bool() const
    {
        return ptr_ != nullptr;
    }

    void reset()
    {
        intrusive_ptr().swap(*this);
    }

    void reset(T* ptr)
    {
        intrusive_ptr(ptr).swap(*this);
    }

    void swap(intrusive_ptr& other) noexcept
    {
        std::swap(ptr_, other.ptr_);
    }

    T* get() const noexcept
    {
        return ptr_;
    }

    T* operator->() const noexcept {
        return ptr_;
    }

    intrusive_wptr<T> weak()
    {
        return intrusive_wptr<T>(ptr_);
    }

private:
    T* ptr_;

    struct has_intrusive_ptr_api {

        template <typename U>
        static constexpr decltype(std::declval<U>().intrusive_ptr_add_ref(), bool()) test_intrusive_ptr_api(int)
        {
            return true;
        }

        template <typename U>
        static constexpr bool test_intrusive_ptr_api(...) {
            return false;
        }

        static constexpr bool value = test_intrusive_ptr_api<T>(int());
    };

};

template<typename T>
inline intrusive_ptr<T> intrusive_wptr<T>::lock()
{
    bool success;
    if constexpr(has_intrusive_ptr_api::value) {
        success = ptr_->intrusive_wptr_lock();
    } else {
        success = intrusive_wptr_lock(ptr_);
    }

    if (success) {
        return intrusive_ptr<T>(ptr_);
    }
    return intrusive_ptr<T>();
}

struct intrusive_strong_reference
{
    intrusive_strong_reference() : references{ 1 } { }
    
    std::atomic<uint32_t> references;
};

inline
void intrusive_strong_ptr_add_ref(intrusive_strong_reference* ptr)
{
    ptr->references++;
}

template<typename T>
inline
void intrusive_strong_ptr_release(intrusive_strong_reference* ptr, T* derived)
{
    if (0 == --ptr->references) {
        delete derived;
    }
}


union intrusive_weak_references {
    intrusive_weak_references() : references{ reference_count(1, 1) } { }
    
    std::atomic<uint64_t> references;                   // both strong and weak
    static inline uint32_t strong_count(uint64_t v)
    {
        return static_cast<uint32_t>(v);
    }
    static inline uint32_t weak_count(uint64_t v)
    {
        return static_cast<uint32_t>(v >> 32);
    }
    static inline uint64_t reference_count(uint32_t strong, uint32_t weak)
    {
        return (static_cast<uint64_t>(weak) << 32) | strong;
    }
};

inline
void intrusive_weak_ptr_add_ref(intrusive_weak_references* ptr)
{
    ptr->references++;
}

template<typename T>
inline
void intrusive_weak_ptr_release(intrusive_weak_references* ptr, T* derived)
{
    auto value = ptr->references.load();
    decltype(value) desired;

    do {
        auto strong = intrusive_weak_references::strong_count(value) - 1;
        auto weak = intrusive_weak_references::weak_count(value);
        if (strong == 0)
            weak--;

        desired = intrusive_weak_references::reference_count(strong, weak);
    } while (!ptr->references.compare_exchange_weak(value, desired));

    if (intrusive_weak_references::strong_count(desired) == 0) {
        if (intrusive_weak_references::weak_count(desired) == 0) {
            delete ptr;
        } else {
            derived->~T();
        }
    }
}

inline
void intrusive_weak_ptr_add_weak_ref(intrusive_weak_references* ptr)
{
    ptr->references += intrusive_weak_references::reference_count(0, 1);
}

inline
void intrusive_weak_ptr_release_weak(intrusive_weak_references* ptr)
{
    auto value = ptr->references.load();
    decltype(value) desired;

    do {
        auto strong = intrusive_weak_references::strong_count(value);
        auto weak = intrusive_weak_references::weak_count(value) - 1;

        desired = intrusive_weak_references::reference_count(strong, weak);
    } while (!ptr->references.compare_exchange_weak(value, desired));

    if (desired == 0) {
        ::operator delete(ptr);
    }
}

inline
bool intrusive_weak_ptr_lock(intrusive_weak_references* ptr)
{
    auto value = ptr->references.load();
    decltype(value) desired;

    do {
        auto strong = intrusive_weak_references::strong_count(value) + 1;
        auto weak = intrusive_weak_references::weak_count(value);

        if (strong == 1) {
            return false;
        }

        desired = intrusive_weak_references::reference_count(strong, weak);
    } while (!ptr->references.compare_exchange_weak(value, desired));
    return true;
}

}