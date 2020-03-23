#pragma once
#include <functional>
#include <type_traits>
#include <utility>
#include "exception.hpp"

namespace ptl {
namespace detail {

class scope_guard_impl_base
{
public:
    void dismiss() noexcept
    {
        dismissed_ = true;
    }

protected:
    scope_guard_impl_base() noexcept
        : dismissed_(false)
    {}

    static void warn_about_to_crash() noexcept;

    static scope_guard_impl_base makeEmptyScopeGuard() noexcept
    {
        return scope_guard_impl_base{};
    }

    template <typename T> static const T &as_const(const T &t) noexcept
    {
        return t;
    }

    bool dismissed_;
};

template <typename FT, bool InvokeNoexcept> class scope_guard_impl : public scope_guard_impl_base
{
public:
    explicit scope_guard_impl(FT &fn) noexcept(std::is_nothrow_copy_constructible<FT>::value)
        : scope_guard_impl(as_const(fn), make_failsafe(std::is_nothrow_copy_constructible<FT>{}, &fn))
    {}

    explicit scope_guard_impl(const FT &fn) noexcept(std::is_nothrow_copy_constructible<FT>::value)
        : scope_guard_impl(fn, make_failsafe(std::is_nothrow_copy_constructible<FT>{}, &fn))
    {}

    explicit scope_guard_impl(FT &&fn) noexcept(std::is_nothrow_move_constructible<FT>::value)
        : scope_guard_impl(std::move_if_noexcept(fn), make_failsafe(std::is_nothrow_move_constructible<FT>{}, &fn))
    {}

    scope_guard_impl(scope_guard_impl &&other) noexcept(std::is_nothrow_move_constructible<FT>::value)
        : function_(std::move_if_noexcept(other.function_))
    {
        dismissed_ = std::exchange(other.dismissed_, true);
    }

    ~scope_guard_impl() noexcept(InvokeNoexcept)
    {
        if (!dismissed_) {
            execute();
        }
    }

private:
    template <typename FUNCTION>
    explicit scope_guard_impl(FUNCTION &&fn, scope_guard_impl_base &&failsafe)
        : scope_guard_impl_base{}
        , function_(std::forward<FUNCTION>(fn))
    {
        failsafe.dismiss();
    }

    void *operator new(std::size_t) = delete;

    static scope_guard_impl_base make_failsafe(std::true_type, const void *) noexcept
    {
        return makeEmptyScopeGuard();
    }

    template <typename FUNCTION>
    static auto make_failsafe(std::false_type, FUNCTION *fn) noexcept
        -> scope_guard_impl<decltype(std::ref(*fn)), InvokeNoexcept>
    {
        return scope_guard_impl<decltype(std::ref(*fn)), InvokeNoexcept>{std::ref(*fn)};
    }

    void execute() noexcept(InvokeNoexcept)
    {
        if (InvokeNoexcept) {
            using R = decltype(function_());
            auto catcher = []() -> R { warn_about_to_crash(), std::terminate(); };
            catch_exception(function_, catcher);
        }
        else {
            function_();
        }
    }

    FT function_;
};

template <typename F, bool INE> using ScopeGuardImplDecay = scope_guard_impl<typename std::decay<F>::type, INE>;

enum class scope_guard_on_exit
{
};

template <typename FunctionType>
scope_guard_impl<typename std::decay<FunctionType>::type, true> operator+(detail::scope_guard_on_exit, FunctionType &&fn)
{
    return scope_guard_impl<typename std::decay<FunctionType>::type, true>(std::forward<FunctionType>(fn));
}

} // namespace detail
} // namespace ptl

#define SCOPE_EXIT(...)                                                                                                \
    auto ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE) = ptl::detail::scope_guard_on_exit() + [&]() noexcept __VA_ARGS__
