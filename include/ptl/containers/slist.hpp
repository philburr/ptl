#pragma once
#include <cstdlib>
#include <cstdint>
#include "ptl/hash/fnv1a.hpp"

namespace ptl {

namespace detail {
template <uint32_t ID>
struct SListBinding
{};

} // namespace detail

#define MAKE_SLIST_BINDING_STRING(type, field) "SListEntry<" #type ">::" #field
#define MAKE_SLIST_BINDING(type, field)                                                                                \
    template <>                                                                                                        \
    struct ptl::detail::SListBinding<ptl::fnv1a_32(MAKE_SLIST_BINDING_STRING(type, field))>                            \
    {                                                                                                                  \
        static_assert(std::is_standard_layout_v<type>, #type " must be standard layout");                              \
        static constexpr size_t offset = offsetof(type, field);                                                        \
    }
#define SLIST_BINDING(type, field)                                                                                     \
    type, ptl::detail::SListBinding<ptl::fnv1a_32(MAKE_SLIST_BINDING_STRING(type, field))>

template <typename T, typename BIND>
struct SListEntry
{
    SListEntry()
        : next_(nullptr)
    {}

    ~SListEntry()
    {
        assert(next_ == nullptr);
    }

    bool in_list() const noexcept
    {
        return next_ != nullptr;
    }

    T *next()
    {
        return from_entry(next_);
    }

private:
    template <typename Ty, typename B>
    friend struct SList;

    SListEntry<T, BIND> *to_entry(T *item) const noexcept
    {
        return reinterpret_cast<ListEntry<T, BIND> *>(reinterpret_cast<uintptr_t>(item) + BIND::offset);
    }
    T *from_entry() const noexcept
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(this) - BIND::offset);
    }

    SListEntry<T, BIND> *next_;
};

template <typename T, typename BIND>
struct SList
{
    SList() = default;
    ~SList()
    {
        auto *curr = head_.next_;
        while (curr) {
            head_.next_ = curr->next_;
            delete curr;
            curr = head_.next_;
        }
    }

    void push_back(T *item)
    {
        assert(!item->in_list());
        auto **curr = &head_.next_;
        while (*curr) {
            curr = &((*curr)->next_);
        }
        *curr = to_entry(item);
    }

    void push_front(T *item)
    {
        assert(!item->in_list());
        item->next_ = head_.next_;
        head_.next_ = item;
    }

    T *front()
    {
        if (empty()) {
            return nullptr;
        }
        return head_.next_->from_entry();
    }

    T *back()
    {
        if (empty()) {
            return nullptr;
        }
        auto *prev = (SList<T, BIND> *)nullptr;
        auto *curr = head_.next_;
        while (curr) {
            prev = curr;
            curr = head_.next_;
        }
        return prev->from_entry();
    }

    bool empty() const
    {
        return !head_.in_list();
    }

private:
    SListEntry<T, BIND> *to_entry(T *item) const noexcept
    {
        return reinterpret_cast<SListEntry<T, BIND> *>(reinterpret_cast<uintptr_t>(item) + BIND::offset);
    }
    T *from_entry(SListEntry<T, BIND> *entry) const noexcept
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(entry) - BIND::offset);
    }

    SListEntry<T, BIND> head_;
};

} // namespace ptl