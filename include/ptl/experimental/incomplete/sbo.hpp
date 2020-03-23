#pragma once

template<typename T>
struct small_buffer_storage
{
    small_buffer_storage() {
        if constexpr (is_stack_allocated) {
            memset(stack_allocated, 0, sizeof(T));
        } else {
            heap_allocated = nullptr;
        }
    }

    small_buffer_storage(const T& other) {
        
    }

private:
    static constexpr bool is_stack_allocated = sizeof(T) <= sizeof(stack_allocated);
    union {
        T* heap_allocated;
        std::aligned_storage<4 * sizeof(uintptr_t)>::type stack_allocated;
    };
};