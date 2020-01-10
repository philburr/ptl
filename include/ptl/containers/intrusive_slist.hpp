#pragma once
#include <cstdlib>
#include <cstdint>
#include "ptl/hash/fnv1a.hpp"

namespace ptl {

namespace detail {
template <uint32_t ID>
struct IntrusiveSListBinding
{};

} // namespace detail

#define MAKE_INTRUSIVE_SLIST_BINDING_STRING(type, field) "IntrusiveSListEntry<" #type ">::" #field
#define MAKE_INTRUSIVE_SLIST_BINDING(type, field)                                                                                \
    template <>                                                                                                        \
    struct ptl::detail::IntrusiveSListBinding<ptl::fnv1a_32(MAKE_INTRUSIVE_SLIST_BINDING_STRING(type, field))>                            \
    {                                                                                                                  \
        static_assert(std::is_standard_layout_v<type>, #type " must be standard layout");                              \
        static constexpr size_t offset = offsetof(type, field);                                                        \
    }
#define INTRUSIVE_SLIST_BINDING(type, field)                                                                                     \
    type, ptl::detail::IntrusiveSListBinding<ptl::fnv1a_32(MAKE_INTRUSIVE_SLIST_BINDING_STRING(type, field))>

template <typename T, typename BIND>
struct IntrusiveSListEntry
{
    IntrusiveSListEntry()
        : next_()
    {}

    ~IntrusiveSListEntry()
    {
    }

    intrusive_ptr<T> next()
    {
        return intrusive_ptr<T>(from_entry(next_), true);
    }

private:
    template <typename Ty, typename B>
    friend struct IntrusiveSList;

    T* from_entry() const noexcept
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(this) - BIND::offset);
    }

    intrusive_ptr<IntrusiveSListEntry<T, BIND>> next_;

    friend intrusive_ptr<IntrusiveSListEntry>;
    void intrusive_ptr_add_ref()
    {
        from_entry()->intrusive_ptr_add_ref();
    }
    void intrusive_ptr_release()
    {
        from_entry()->intrusive_ptr_release();
    }
};

template <typename T, typename BIND>
struct IntrusiveSList
{
    IntrusiveSList() = default;
    ~IntrusiveSList() = default;

    void push_back(intrusive_ptr<T> item)
    {
        auto entry = decltype(head_)(to_entry(item.get()), true);

        auto *curr = &head_->next_;
        while (*curr) {
            curr = &((*curr)->next_);
        }
        *curr = std::move(entry);
    }

    template<typename... Args>
    void emplace_back(Args... args)
    {
        auto entry = decltype(head_)(to_entry(new T(std::forward<Args>(args)...)));

        auto *curr = &head_;
        while (*curr) {
            curr = &((*curr)->next_);
        }
        *curr = std::move(entry);
    }

    void push_front(intrusive_ptr<T> item)
    {
        auto entry = decltype(head_)(to_entry(item.get()), true);
        std::exchange(entry->next_, std::exchange(head_, entry));
    }

    intrusive_ptr<T> front()
    {
        if (!head_) {
            return {};
        }
        auto entry = from_entry(head_.get());
        return intrusive_ptr<T>(entry, true);
    }

    intrusive_ptr<T> back()
    {
        auto& last = head_;
        if (head_) {
            auto& curr = head_->next_;
            while (curr) {
                last = curr;
                curr = curr->next_;
            }
        }

        auto entry = from_entry(last.get());
        return intrusive_ptr<T>(true, entry);
    }

    bool empty() const
    {
        return !head_;
    }

private:
    IntrusiveSListEntry<T, BIND> *to_entry(T *item) const noexcept
    {
        return reinterpret_cast<IntrusiveSListEntry<T, BIND> *>(reinterpret_cast<uintptr_t>(item) + BIND::offset);
    }
    T *from_entry(IntrusiveSListEntry<T, BIND> *entry) const noexcept
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(entry) - BIND::offset);
    }

    intrusive_ptr<IntrusiveSListEntry<T, BIND>> head_;
};

} // namespace ptl