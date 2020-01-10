#pragma once
#include <type_traits>
#include "ptl/expected.hpp"

namespace ptl {

template<typename T>
class exceptable {
    static_assert(!std::is_reference<T>::value, "exceptable<T> cannot contain a reference");

public:
    exceptable() = default;
    ~exceptable() = default;

    explicit exceptable(const T& v) : value_(v) {}
    explicit exceptable(T&& v) : value_(std::move(v)) {}
    explicit exceptable(std::exception_ptr ex) : value_(std::move(ex)) {}

    T& value() & {
        throw_if_failed();
        return value_.value();
    }

    bool has_value() const {
        return value_.is_expected();
    }
    bool has_exception() const {
        return value_.is_unexpected();
    }

private:
    void throw_if_failed() {
        if (value_.is_unexpected()) {
            throw value_.unexpected();
        }
    }

    ptl::expected<T, std::exception_ptr> value_;
};

template<>
class exceptable<void> {
public:
    ~exceptable() = default;

    explicit exceptable() : exception_() {}
    explicit exceptable(std::exception_ptr ex) : exception_(std::move(ex)) {}

    void value() {
        throw_if_failed();
    }

    bool has_exception() const {
        return exception_ != nullptr;
    }

private:
    void throw_if_failed() {
        if (exception_ != nullptr) {
            throw exception_;
        }
    }

    std::exception_ptr exception_;
};

template<typename T>
class exceptable<T&> {
public:
    exceptable() = default;
    ~exceptable() = default;

    explicit exceptable(T& v) : value_(std::addressof(v)) {}
    explicit exceptable(std::exception_ptr ex) : value_(std::move(ex)) {}

    T& value() & {
        throw_if_failed();
        return *value_.value();
    }
    T&& value() && {
        throw_if_failed();
        return std::move(value_.value());
    }

    bool has_value() const {
        return value_.is_expected();
    }
    bool has_exception() const {
        return value_.is_unexpected();
    }

private:
    void throw_if_failed() {
        if (value_.is_unexpected()) {
            throw value_.unexpected();
        }
    }

    ptl::expected<T*, std::exception_ptr> value_;
};


}