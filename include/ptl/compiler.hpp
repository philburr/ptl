#pragma once

#ifndef PTL_FORCE_INLINE
#ifdef __GNUC__
#define PTL_FORCE_INLINE [[gnu::always_inline]] inline
#else
#define PTL_FORCE_INLINE inline
#endif
#endif

#ifndef PTL_OPTIMIZE
#if defined(__clang__)
#define PTL_OPTIMIZE
#elif defined(__GNUC__)
#define PTL_OPTIMIZE __attribute__((optimize("3")))
#else
#define PTL_OPTIMIZE
#endif
#endif

#ifndef PTL_NOOPTIMIZE
#if defined(__clang__)
#define PTL_NOOPTIMIZE __attribute__((optnone))
#elif defined(__GNUC__)
#define PTL_NOOPTIMIZE __attribute__((optimize("0")))
#else
#define PTL_NOOPTIMIZE
#endif
#endif

#ifndef PTL_WEAK
#ifdef __GNUC__
#define PTL_WEAK __attribute__((weak))
#else
#define PTL_WEAK
#endif
#endif

#ifndef PTL_NORETURN
#define PTL_NORETURN [[noreturn]]
#endif

#define PTL_HAS_EXCEPTIONS 1
#define PTL_VISIBILITY_HIDDEN __attribute__((__visibility__("hidden")))
#define PTL_ERASE PTL_FORCE_INLINE PTL_VISIBILITY_HIDDEN PTL_OPTIMIZE
#define PTL_ERASE_TRYCATCH PTL_ERASE
#define PTL_NOINLINE __attribute__((__noinline__))
#define PTL_COLD __attribute__((__cold__))

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
#define ANONYMOUS_VARIABLE(name) CONCATENATE(name, __COUNTER__)

#if defined(__clang__)
#elif defined(__GNUC_)
static inline int __builtin_COLUMN() { return 0; }
#endif