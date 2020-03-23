#pragma once
#include <cstdint>
#include <new>
#include <type_traits>

namespace ptl {

struct Alignment
{
    static constexpr size_t PTR = sizeof(void*);
    static constexpr size_t SSE = 16;
    static constexpr size_t AVX = 32;
    static constexpr size_t CACHE_LINE = 64;
};

namespace detail {
    void* allocate_aligned_memory(size_t align, size_t size)
    {
        assert(align >= sizeof(void*));
        
        if (size == 0) {
            return nullptr;
        }
        void* ptr = nullptr;
        int r = posix_memalign(&ptr, align, size);
        if (r != 0) {
            return nullptr;
        }
        return ptr;
    }
    void deallocate_aligned_memory(void* ptr) noexcept
    {
        free(ptr);
    }
}

template<typename T, size_t A = Alignment::AVX>
class aligned_allocator
{
public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    using propagate_on_container_move_assignment = std::true_type;

    constexpr aligned_allocator() noexcept {}
    constexpr aligned_allocator(const aligned_allocator& other) noexcept {}
    template<typename U>
    constexpr aligned_allocator(const aligned_allocator<U>& other) noexcept {}

    /*constexpr*/ ~aligned_allocator() {}

    [[nodiscard]] T* allocate(size_type n)
    {
        constexpr size_type alignment = static_cast<size_type>(A);
        void* ptr = detail::allocate_aligned_memory(alignment, n * sizeof(T));
        if (ptr == nullptr) {
            throw std::bad_alloc();
        }
        return reinterpret_cast<T*>(ptr);
    }
    void deallocate(T* p, size_type n )
    {
        detail::deallocate_aligned_memory(p);
    }
};

}