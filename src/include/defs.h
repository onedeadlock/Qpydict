#ifndef QPy_DEFS_H
#define QPy_DEFS_H

#ifndef OR
#    define OR   ||
#endif
#ifndef AND
#    define AND  &&
#endif

#if defined(_WIN32) OR defined(_WIN64) OR defined(_MSC_VER)
#    include <windows.h>
#endif

#if defined(__clang__) OR defined(__GNUC__) OR defined(__MINGW32__) OR defined(__MINGW64__)
#    define HAVE_GCC_COMPILER   1
#elif defined(_MSC_VER)
#    define HAVE_MSVC_COMPILER  1
#elif defined(INTEL_LLVM_COMPILER)
#    define HAVE_INTEL_COMPILER 1
#endif

#ifndef __has_builtin
#    define __has_builtin(x) 0
#endif
#ifndef __has_attribute
#    define __has_attribute(x) 0 
#endif
#define has_builtin(x)   (__has_builtin(x) OR HAVE_GCC_COMPILER)
#define has_attribute(x) (__has_attribute(x) OR HAVE_GCC_COMPILER)

#if has_builtin(__builtin_expect)
#    define UNLIKELY(x) __builtin_expect(!!(x), 0)
#    define LIKELY(x)   __builtin_expect(!!(x), 1)
#else
#    define UNLIKELY(x) (x)
#    define LIKELY(x)   (x)
#endif

#if has_builtin(__builtin_ctz)
#    define BSR(x) __builtin_ctzll(x)
#elif HAVE_INTEL_COMPILER && defined(_bit_scan_reverse)
#    define BSR(x) _bit_scan_reverse(x)
#elif HAVE_MSVC_COMPILER
static inline __forceinline uint64_t BSR(const uint64_t v)
{
    uint64_t idx;
    return (_BitScanReverse64(&idx, v), idx);
}
#else
#    define BSR(x) // TODO
#endif

#if has_builtin(__builtin_clz)
#    define BSF(x) __builtin_clzll(x)
#elif HAVE_INTEL_COMPILER && defined(_bit_scan_forward)
#    define BSF(x) _bit_scan_forward(x)
#elif HAVE_MSVC_COMPILER
static inline __forceinline uint64_t BSF(const uint64_t v)
{
    uint64_t idx;
    return (_BitScanForward64(&idx, v), idx);
}
#else
#    define BSF(x) // TODO
#endif

#if has_builtin(__builtin_unreachable)
#    define UNREACHABLE() __builtin_unreachable()
#else
#    define UNREACHABLE()
#endif

// enable aligned load on arm (default)
#if SYSARCH_IS_ARM && !defined(ALIGNED_LOAD)
#    define ALIGNED_LOAD 1
#endif

// Round up/down to a multiple of 2^d
#define ALIGN(v, d)    ((v) - ((v) & (d)))
#define ALIGNU(v, d)   (((n) + (d) - 1) & ~((d) - 1))

// set value
#define SETVAL(lv, rv) ((lv) = (rv))

#if has_builtin(__builtin_add_overflow_p) && has_builtin(__builtin_mul_overflow_p)
#   define check_if_safe_add(x, y, hint) !__builtin_add_overflow_p(x, y, hint)
#   define check_if_safe_mul(x, y, hint) !__builtin_mul_overflow_p(x, y, hint) 
#else // TODO
#   define check_if_safe_add(x, y, type) 0
#   define check_if_safe_mul(x, y, type) 0
#endif

#if has_attribute(__always_inline__)
#    define force_inline __attribute__((always_inline))
#elif HAVE_MSVC_COMPILER
#    define force_inline __forceinline
#else 
#    define force_inline 
#endif

#if has_attribute(__warn_unused_result__)
#    define warn_unused __attribute__((warn_unused_result))
#else
#    define warn_unused 
#endif

#if has_attribute(__unused__)
#    define UNUSED(x) x __attribute__((unused))
#else
#    define UNUSED(x) x
#endif

#if has_attribute(__pure__)
#    define PURE(x) x __attribute__((pure))
#else
#    define PURE(x) x
#endif

#ifndef local
#    define local static
#endif
#ifndef local_inline
#    define local_inline static inline force_inline
#endif

#ifndef NO_PYAPI
#    define HASH(self, key) (self)->hash(key)
#    define CMP(self, t, u) (self)->cmp(t, u)
#else
#    define HASH(self, key) PyObject_Hash(key)
#    define CMP(self, t, u) 0
#endif

#undef has_builtin
#undef has_attribute
#endif // QPy_DEFS_H
