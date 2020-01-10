#pragma once
#include "exception.hpp"
#include <functional>
#include <type_traits>
#include <utility>

namespace ptl {
namespace detail {

class ScopeGuardImplBase
{
public:
    void dismiss() noexcept
    {
        dismissed_ = true;
    }

protected:
    ScopeGuardImplBase() noexcept
        : dismissed_(false)
    {}

    static void warnAboutToCrash() noexcept;

    static ScopeGuardImplBase makeEmptyScopeGuard() noexcept
    {
        return ScopeGuardImplBase{};
    }

    template <typename T> static const T &asConst(const T &t) noexcept
    {
        return t;
    }

    bool dismissed_;
};

template <typename FT, bool InvokeNoexcept> class ScopeGuardImpl : public ScopeGuardImplBase
{
public:
    explicit ScopeGuardImpl(FT &fn) noexcept(std::is_nothrow_copy_constructible<FT>::value)
        : ScopeGuardImpl(asConst(fn), makeFailSafe(std::is_nothrow_copy_constructible<FT>{}, &fn))
    {}

    explicit ScopeGuardImpl(const FT &fn) noexcept(std::is_nothrow_copy_constructible<FT>::value)
        : ScopeGuardImpl(fn, makeFailsafe(std::is_nothrow_copy_constructible<FT>{}, &fn))
    {}

    explicit ScopeGuardImpl(FT &&fn) noexcept(std::is_nothrow_move_constructible<FT>::value)
        : ScopeGuardImpl(std::move_if_noexcept(fn), makeFailsafe(std::is_nothrow_move_constructible<FT>{}, &fn))
    {}

    ScopeGuardImpl(ScopeGuardImpl &&other) noexcept(std::is_nothrow_move_constructible<FT>::value)
        : function_(std::move_if_noexcept(other.function_))
    {
        dismissed_ = std::exchange(other.dismissed_, true);
    }

    ~ScopeGuardImpl() noexcept(InvokeNoexcept)
    {
        if (!dismissed_) {
            execute();
        }
    }

private:
    template <typename Fn>
    explicit ScopeGuardImpl(Fn &&fn, ScopeGuardImplBase &&failsafe)
        : ScopeGuardImplBase{}
        , function_(std::forward<Fn>(fn))
    {
        failsafe.dismiss();
    }

    void *operator new(std::size_t) = delete;

    static ScopeGuardImplBase makeFailsafe(std::true_type, const void *) noexcept
    {
        return makeEmptyScopeGuard();
    }

    template <typename Fn>
    static auto makeFailsafe(std::false_type, Fn *fn) noexcept
        -> ScopeGuardImpl<decltype(std::ref(*fn)), InvokeNoexcept>
    {
        return ScopeGuardImpl<decltype(std::ref(*fn)), InvokeNoexcept>{std::ref(*fn)};
    }

    void execute() noexcept(InvokeNoexcept)
    {
        if (InvokeNoexcept) {
            using R = decltype(function_());
            auto catcher = []() -> R { warnAboutToCrash(), std::terminate(); };
            catch_exception(function_, catcher);
        }
        else {
            function_();
        }
    }

    FT function_;
};

template <typename F, bool INE> using ScopeGuardImplDecay = ScopeGuardImpl<typename std::decay<F>::type, INE>;

enum class ScopeGuardOnExit
{
};

template <typename FunctionType>
ScopeGuardImpl<typename std::decay<FunctionType>::type, true> operator+(detail::ScopeGuardOnExit, FunctionType &&fn)
{
    return ScopeGuardImpl<typename std::decay<FunctionType>::type, true>(std::forward<FunctionType>(fn));
}

} // namespace detail
} // namespace ptl

#define SCOPE_EXIT(...)                                                                                                \
    auto ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE) = ptl::detail::ScopeGuardOnExit() + [&]() noexcept __VA_ARGS__
