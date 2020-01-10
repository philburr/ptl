#pragma once

#ifndef PTL_FORCE_INLINE
#ifdef __GNUC__
#define PTL_FORCE_INLINE [[gnu::always_inline]] inline
#else
#define PTL_FORCE_INLINE inline
#endif
#endif

#ifndef PTL_OPTIMIZE
#ifdef __GNUC_
#define PTL_OPTIMIZE [[gnu::optimize(3)]]
#else
#define PTL_OPTIMIZE
#endif
#endif

#ifndef PTL_NOOPTIMIZE
#ifdef __GNUC_
#define PTL_NOOPTIMIZE [[gnu::optimize(0)]]
#else
#define PTL_NOOPTIMIZE
#endif
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
