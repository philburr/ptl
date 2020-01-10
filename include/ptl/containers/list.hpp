#pragma once
#include <cstdlib>
#include <cstdint>
#include "ptl/hash/fnv1a.hpp"

namespace ptl {

/* ListBinding is used to store at compile-time, the relationship between a struct/field
 * and the field's offset in its struct.  We actually create a relationship between the fnv1a_32
 * hash of the string of "ListEntry<type>::field" and field's offset with type.  To facilitate
 * its use, use the macro MAKE_LIST_ENTRY_BINDING().  The binding supports recording multiple fields
 * so that struct can have multiple ListEntry fields.
 *
 * Example:
 * struct my_data {
 *      int data;
 *      ListEntry<LIST_BINDING(my_data, entry1)> entry1;
 *      ListEntry<LIST_BINDING(my_data, entry2)> entry2;
 * };
 * MAKE_LIST_BINDING(my_data, entry1);
 * MAKE_LIST_BINDING(my_data, entry2);
 *
 * struct my_list1 {
 *      int some_list_related_data;
 *      List<LIST_BINDING(my_data, entry1)> list;
 * };
 * struct my_list2 {
 *      int some_other_list_related_data;
 *      List<LIST_BINDING(my_data, entry2)> list;
 * };
 *
 * // These will automagically use the correct entry1/entry2 fields in my_data.
 * my_list1.list.push_back(new my_data { 3 });
 * my_list2.list.push_back(new my_data { 4 });
 */

namespace detail {
template <uint32_t ID>
struct ListBinding
{};

} // namespace detail

#define MAKE_LIST_BINDING_STRING(type, field) "ListEntry<" #type ">::" #field
#define MAKE_LIST_BINDING(type, field)                                                                                 \
    template <>                                                                                                        \
    struct ptl::detail::ListBinding<ptl::fnv1a_32(MAKE_LIST_BINDING_STRING(type, field))>                         \
    {                                                                                                                  \
        static_assert(std::is_standard_layout_v<type>, #type " must be standard layout");                              \
        static constexpr size_t offset = offsetof(type, field);                                                        \
    }
#define LIST_BINDING(type, field)                                                                                      \
    type, ptl::detail::ListBinding<ptl::fnv1a_32(MAKE_LIST_BINDING_STRING(type, field))>

template <typename T, typename BIND>
struct ListEntry
{
    ListEntry()
        : next_(this)
        , prev_(this)
    {}

    ~ListEntry()
    {
        remove();
    }

    void remove()
    {
        auto prev = prev_;
        auto next = next_;
        next_->prev_ = prev;
        prev_->next_ = next;
        next_ = prev_ = this;
    }

    bool in_list() const noexcept
    {
        return next_ != this;
    }

    void insert_before(ListEntry<T, BIND> *other)
    {
        this->next_ = other;
        this->prev_ = other->prev_;
        this->next_->prev_ = this;
        this->prev_->next_ = this;
    }

    void insert_after(ListEntry<T, BIND> *other)
    {
        this->prev_ = other;
        this->next_ = other->next_;
        this->prev_->next_ = this;
        this->next_->prev_ = this;
    }

    T *next()
    {
        return from_entry(next_);
    }
    T *prev()
    {
        return from_entry(prev_);
    }

private:
    template <typename Ty, typename B>
    friend struct List;

    ListEntry<T, BIND> *to_entry(T *item) const noexcept
    {
        return reinterpret_cast<ListEntry<T, BIND> *>(reinterpret_cast<uintptr_t>(item) + BIND::offset);
    }
    T *from_entry() const noexcept
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(this) - BIND::offset);
    }

    ListEntry<T, BIND> *next_;
    ListEntry<T, BIND> *prev_;
};

template <typename T, typename BIND>
struct List
{
    List() = default;
    ~List()
    {
        while (!empty()) {
            delete front();
        }
    }

    void push_back(T *item)
    {
        to_entry(item)->insert_before(&head_);
    }

    void push_front(T *item)
    {
        to_entry(item)->insert_after(&head_);
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
        return head_.prev_->from_entry();
    }

    bool empty() const
    {
        return !head_.in_list();
    }

private:
    ListEntry<T, BIND> *to_entry(T *item) const noexcept
    {
        return reinterpret_cast<ListEntry<T, BIND> *>(reinterpret_cast<uintptr_t>(item) + BIND::offset);
    }
    T *from_entry(ListEntry<T, BIND> *entry) const noexcept
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(entry) - BIND::offset);
    }

    ListEntry<T, BIND> head_;
};

} // namespace ptl