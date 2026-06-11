#ifndef QPy_DEFS_H
#define QPy_DEFS_H

#ifndef OR
#    define OR   ||
#endif
#ifndef AND
#    define AND  &&
#endif
#ifndef NOT
#    define NOT  !
#endif

#if defined(__clang__) OR defined(__GNUC__) OR defined(__MINGW32__) OR defined(__MINGW64__)
#    define HAVE_GCC   1
#elif defined(_MSC_VER)
#    define HAVE_MSVC  1
#elif defined(INTEL_LLVM_COMPILER)
#    define HAVE_INTEL 1
#endif

#ifndef __has_builtin
#    define __has_builtin(x) 0
#endif
#ifndef __has_attribute
#    define __has_attribute(x) 0 
#endif
#define has_builtin(x)   (__has_builtin(x)   OR HAVE_GCC)
#define has_attribute(x) (__has_attribute(x) OR HAVE_GCC)

#if has_builtin(__builtin_expect)
#    define UNLIKELY(x) __builtin_expect(!!(x), 0)
#    define LIKELY(x)   __builtin_expect(!!(x), 1)
#else
#    define UNLIKELY(x) (x)
#    define LIKELY(x)   (x)
#endif

#if has_builtin(__builtin_clzll)
#    define BSF(x)    __builtin_clzll(x)
#    define BSR(x)    __builtin_ctzll(x)
#    define POPCNT(x) __builtin_popcountll(x)
#elif HAVE_INTEL_COMPILER OR HAVE_MSVC_COMPILER
#    if defined(_bit_scan_forward)
#        define BSF(x) _bit_scan_forward(x)
#    else
#       define BSF BSF
//
static inline __forceinline uint64_t BSF(const uint64_t v)
{
    uint64_t idx;
    return (_BitScanForward64(&idx, v), idx);
}
//
#    endif
#    if defined(_bit_scan_reverse)
#        define BSR(x) _bit_scan_reverse(x)
#    else
#       define BSR BSR
//
static inline __forceinline uint64_t BSR(const uint64_t v)
{
    uint64_t idx;
    return (_BitScanReverse64(&idx, v), idx);
}
//
#    endif
#else
#     // TODO
#    define BSF(x)
#    define BSR(x) 
#    define POPCNT(x) 
#endif

#if has_builtin(__builtin_unreachable)
#    define UNREACHABLE() __builtin_unreachable()
#elif HAVE_MSVC_COMPILER
#    define UNREACHABLE() __assume(0)
#else
#    define UNREACHABLE()
#endif

// align to a multiple of d, where d is a power of 2. v is unchange if d already satisfies the alignment
#define ALIGN(v, d)  ((v)  - ((v) & (d) - 1)) // down
#define ALIGNU(v, d) (((n) + (d) - 1) & ~((d) - 1)) // up

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
#    define pure__ __attribute__((pure))
#else
#    define pure__
#endif

#ifndef local
#    define local static
#endif
#ifndef local_inline
#    define local_inline static inline force_inline
#endif

// logging
#ifndef NDEBUG
#    if NO_PyAPI
#        define LOG(file, fmt, ...)  fprintf(f, fmt, __VA_ARGS__)
#    else
#        define LOG(...) // TODO
#    endif
#else
#    define LOG(...)
#endif

#undef has_builtin
#undef has_attribute
#endif // QPy_DEFS_H
