#pragma once
#include <type_traits>
#include <cstdint>
#include <atomic>

namespace ptl {

template <typename T, typename VT = uint16_t>
struct PackedPtr
{
    static_assert(sizeof(VT) <= sizeof(uint16_t), "Illegal value type in PackedPtr");
    static_assert(alignof(T *) == 8);
    static_assert(sizeof(T *) == 8);

    PackedPtr() noexcept
        : data_(0)
    {}

    VT value() const noexcept
    {
        return reinterpret_cast<VT>(data_ >> high_base);
    }
    void set_value(VT v) noexcept
    {
        data_ = (((uintptr_t)v << high_base) & high_mask) | (data_ & ~high_mask);
    }

    uint8_t small() const noexcept
    {
        return reinterpret_cast<uint8_t>(data_ & low_mask);
    }

    void set_small(uint8_t v) const noexcept
    {
        data_ = (data_ & ~low_mask) | (v & low_mask);
    }

    T *get() const noexcept
    {
        return reinterpret_cast<T *>(data_ & ptr_mask);
    }
    void set(T *ptr) noexcept
    {
        data_ = (data_ & ~ptr_mask) | (reinterpret_cast<uintptr_t>(ptr) & ptr_mask);
    }

private:
    static constexpr uintptr_t low_mask = uintptr_t(-1) >> 61;
    static constexpr size_t high_base = 48;
    static constexpr uintptr_t high_mask = uintptr_t(-1) << 48;
    static constexpr uintptr_t ptr_mask = ~(low_mask | high_mask);

    uintptr_t data_;
};

template <typename T>
struct PackedPtr<T, bool>
{
    static_assert(alignof(T *) == 8);
    static_assert(sizeof(T *) == 8);

    PackedPtr() noexcept
        : data_(0)
    {}

    bool value() const noexcept
    {
        return (data_ >> high_base) & 1;
    }
    void set_value(bool v) noexcept
    {
        data_ = (((uintptr_t)v << high_base) & high_mask) | (data_ & ~high_mask);
    }
    uint8_t small() const noexcept
    {
        return reinterpret_cast<uint8_t>(data_ & low_mask);
    }

    void set_small(uint8_t v) const noexcept
    {
        data_ = (data_ & ~low_mask) | (v & low_mask);
    }

    T *get() const noexcept
    {
        return reinterpret_cast<T *>(data_ & ptr_mask);
    }
    void set(T *ptr) noexcept
    {
        data_ = (reinterpret_cast<uintptr_t>(ptr) & ptr_mask) | (data_ & ~ptr_mask);
    }

private:
    static constexpr uintptr_t low_mask = uintptr_t(-1) >> 61;
    static constexpr size_t high_base = 63;
    static constexpr uintptr_t high_mask = uintptr_t(-1) << 63;
    static constexpr uintptr_t ptr_mask = ~(low_mask | high_mask);

    uintptr_t data_;
};

template <typename VT>
struct PackedPtr<std::exception_ptr, VT>
{
    static_assert(sizeof(VT) <= sizeof(uint16_t), "Illegal value type in PackedPtr");
    static_assert(sizeof(std::exception_ptr) == 8);
    static_assert(alignof(std::exception_ptr) == 8);

    PackedPtr() noexcept
        : data_(0)
    {}

    VT value() const noexcept
    {
        return reinterpret_cast<VT>(data_ >> high_base);
    }
    void set_value(VT v) noexcept
    {
        data_ = (((uintptr_t)v << high_base) & high_mask) | (data_ & ~high_mask);
    }

    uint8_t small() const noexcept
    {
        return reinterpret_cast<uint8_t>(data_ & low_mask);
    }

    void set_small(uint8_t v) const noexcept
    {
        data_ = (data_ & ~low_mask) | (v & low_mask);
    }

    std::exception_ptr get() const noexcept
    {
        return reinterpret_cast<std::exception_ptr>(data_ & ptr_mask);
    }
    void set(std::exception_ptr ptr) noexcept
    {
        uintptr_t ptr_data = *(uintptr_t*)&ptr;
        data_ = (ptr_data & ptr_mask) | (data_ & ~ptr_mask);
    }

private:
    static constexpr uintptr_t low_mask = uintptr_t(-1) >> 61;
    static constexpr size_t high_base = 48;
    static constexpr uintptr_t high_mask = uintptr_t(-1) << 48;
    static constexpr uintptr_t ptr_mask = ~(low_mask | high_mask);

    uintptr_t data_;
};

// This version is specialized to put the most accessed field (value) at the lowest bits
template <>
struct PackedPtr<std::exception_ptr, uint16_t>
{
    static_assert(sizeof(std::exception_ptr) == sizeof(uintptr_t));

    PackedPtr() noexcept
        : data_(0)
    {}

    uint16_t value() const noexcept
    {
        return static_cast<uint16_t>(data_ & low_mask);
    }
    void set_value(uint16_t v) noexcept
    {
        data_ = (((uintptr_t)v) & low_mask) | (data_ & ~low_mask);
    }

    uint8_t small() const noexcept
    {
        return static_cast<uint8_t>((data_ & high_mask) >> high_base);
    }

    void set_small(uint8_t v) noexcept
    {
        data_ = (data_ & ~high_mask) | (((uintptr_t)v << high_base) & high_mask);
    }

    std::exception_ptr get() const noexcept
    {
        uintptr_t ptr = reinterpret_cast<uintptr_t>((data_ & ptr_mask) >> ptr_shift);
        // avoid conversion constructor
        return *(std::exception_ptr *)(&ptr);
    }
    void set(std::exception_ptr ptr) noexcept
    {
        uintptr_t data = *(uintptr_t *)&ptr;
        data_ = ((data << ptr_shift) & ptr_mask) | (data_ & ~ptr_mask);
    }

#if BUILD_UNITTEST
    uintptr_t internal() const noexcept
    {
        return data_;
    }
#endif

private:
    static constexpr uintptr_t low_mask = uintptr_t(-1) >> 48;
    static constexpr size_t high_base = 61;
    static constexpr uintptr_t high_mask = uintptr_t(-1) << high_base;
    static constexpr size_t ptr_shift = 16 - 3;
    static constexpr uintptr_t ptr_mask = ~(low_mask | high_mask);

    uintptr_t data_;
};

template <>
struct PackedPtr<std::exception_ptr, bool>
{
    static_assert(sizeof(std::exception_ptr) == sizeof(uintptr_t));

    PackedPtr() noexcept
        : data_(0)
    {}

    uint16_t value() const noexcept
    {
        return static_cast<bool>(data_ >> 63);
    }
    void set_value(bool v) noexcept
    {
        data_ = ((uintptr_t)v << 63) | (data_ & (-1ULL >> 16));
    }

    std::exception_ptr get() const noexcept
    {
        uintptr_t ptr = reinterpret_cast<uintptr_t>(data_ & (uintptr_t(-1) >> 16));
        // avoid conversion constructor
        return *(std::exception_ptr *)(&ptr);
    }
    void set(std::exception_ptr ptr) noexcept
    {
        uintptr_t data = *(uintptr_t *)&ptr;
        data_ = (data & (uintptr_t(-1) >> 16)) | (data_ & (uintptr_t(-1) << 63));
    }

private:
    static constexpr uintptr_t low_mask = uintptr_t(-1) >> 61;
    static constexpr size_t high_base = 63;
    static constexpr uintptr_t high_mask = uintptr_t(-1) << 63;
    static constexpr uintptr_t ptr_mask = ~(low_mask | high_mask);

    uintptr_t data_;
};

template <typename T, typename VT = uint16_t>
struct AtomicPackedPtr
{
    static_assert(sizeof(VT) <= sizeof(uint16_t), "Illegal value type in PackedPtr");

    AtomicPackedPtr() noexcept
        : data_(0)
    {}

    VT value() const noexcept
    {
        return reinterpret_cast<VT>(data_.load() >> 48);
    }
    void set_value(VT v) noexcept
    {
        uintptr_t expected, desired;

        expected = data_.load();
        do {
            desired = ((uintptr_t)v << 48) | (expected & (-1ULL >> 16));
        } while (!data_.compare_exchange_weak(expected, desired, std::memory_order_release));
    }

    T *get() const noexcept
    {
        return reinterpret_cast<T *>(data_.load() & (-1ULL >> 16));
    }
    void set(T *ptr) noexcept
    {
        uintptr_t expected, desired;

        expected = data_.load();
        do {
            desired = (reinterpret_cast<uintptr_t>(ptr) & (-1ULL >> 16)) | (expected & (-1ULL << 48));
        } while (!data_.compare_exchange_weak(expected, desired, std::memory_order_release));
    }

    std::pair<T*, VT> get_add(VT delta) const noexcept
    {
        uintptr_t expected, desired;
        VT old;

        expected = data_.load();
        do {
            old = expected >> 48;
            VT new_value = old + delta;
            desired = (expected & (-1ULL >> 16)) | (new_value << 48);
        } while (!data_.compare_exchange_weak(expected, desired, std::memory_order_release));
        return { reinterpret_cast<T *>(expected & (-1ULL >> 16)), old };
    }

private:
    using reference = typename std::add_lvalue_reference<T>::type;
    std::atomic<uintptr_t> data_;
};

template <typename T>
struct AtomicPackedPtr<T, bool>
{
    AtomicPackedPtr() noexcept
        : data_(0)
    {}

    bool value() const noexcept
    {
        return (data_.load() >> 63);
    }
    void set_value(bool v) noexcept
    {
        uintptr_t expected, desired;

        expected = data_.load();
        do {
            desired = ((uintptr_t)v << 63) | (expected & (-1ULL >> 16));
        } while (!data_.compare_exchange_weak(expected, desired, std::memory_order_release));
    }

    T *get() const noexcept
    {
        return reinterpret_cast<T *>(data_.load() & (-1ULL >> 16));
    }
    void set(T *ptr) noexcept
    {
        uintptr_t expected, desired;

        expected = data_.load();
        do {
            desired = (reinterpret_cast<uintptr_t>(ptr) & (-1ULL >> 16)) | (expected & (-1ULL << 63));
        } while (!data_.compare_exchange_weak(expected, desired, std::memory_order_release));
    }

private:
    using reference = typename std::add_lvalue_reference<T>::type;
    std::atomic<uintptr_t> data_;
};

static_assert(sizeof(PackedPtr<void>) == 8, "PackedPtr should be the size of a pointer");
static_assert(std::is_standard_layout<PackedPtr<void>>::value, "PackedPtr should be a POD");
static_assert(sizeof(AtomicPackedPtr<void>) == 8, "AtomicPackedPtr should be the size of a pointer");
static_assert(std::is_standard_layout<AtomicPackedPtr<void>>::value, "AtomicPackedPtr should be a POD");

} // namespace ptl