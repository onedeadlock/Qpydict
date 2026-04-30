#ifndef QPy_DEFS_H
#define QPy_DEFS_H
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    #include <windows.h>
#endif

#if defined(__clang__) || defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#    define QPy_GCL_CC 1
#elif defined(_MSC_VER)
#    define QPy_MSVC_CC 1
#elif defined(INTEL_LLVM_COMPILER)
#    define QPy_INTEL_CC
#endif

#if QPy_ARCH_ARM && !defined(QPy_ALIGNED_LOAD)
    #define QPy_ALIGNED_LOAD 1
#endif

// BSR
#if   QPy_INTEL_CC
#    define QPy_BSR(mask) _bit_scan_reverse(mask)
#elif QPy_GCL_CC
#    define QPy_BSR(mask) __builtin_ctzll(mask)
#elif QPy_MSVC_CC
extern inline __forceinline uint64_t QPy_BSR(const uint64_t v)
{
    unsigned long idx;
    return (_BitScanReverse64(&idx, v), idx);
}
#else
// TODO: implement fallback for BSR
#endif

// Scan mask
#define QPy_scan_mask(v) QPy_FIXIDX(QPy_BSR(v))

// Round up/down to a multiple of 2^d
#define QPy_RNDDOWN(v, d) ((v) - ((v) & (d)))
#define QPy_RNDUP(v,  u)  (((n) + (u) - 1) & ~((u) - 1))

// Intrinsics
#if defined(QPy_GCL_CC) || defined(QPy_INTEL_CC)
#    define QPy_PTR_INLINE(type) __attribute__((nonnull, always_inline)) static inline type
#    define QPy_INLINE(type)     __attribute__((always_inline))         static inline type
#    define QPy_LIKELY(expr)     __builtin_expect(!!(expr), 1)
#    define QPy_UNLIKELY(expr)   __builtin_expect(!!(expr), 0)
#else
#    if defined(QPy_MSVC_CC)
#        define QPy_PTR_INLINE(type) static inline __forceinline type
#        define QPy_INLINE(type) QPy_PTR_INLINE(type)
#    else
#        define QPy_PTR_INLINE()
#        define QPy_INLINE()
#    endif
#    define QPy_LIKELY()
#    define QPy_UNLIKELY()
#endif

#define QPy_SETVAL(lv, rv)      ((lv) = (rv))
#define QPy_SETEXC(type, msg)   (PyErr_SetString(type, msg), QPy_Err)
#define QPy_RAISE_BADARG(msg)   QPy_SETEXC(PyExc_TypeError, msg)
#define QPy_RAISE_OVERFLOW(msg) QPy_SETEXC(PyExc_OverflowError, msg)
#if Py_VERSION_HEX >= 0x030e00000
#    define QPy_ITERNEXT(iter, arg) (PyIter_NextItem(iter, arg) > 0)
#else
#    define QPy_ITERNEXT(iter, arg) QPy_SETVAL(*arg, PyIter_Next(iter))
#endif
#define QPy_TUPLE_GETITEM(tuple, item, i) !(item = PyTuple_GetItem(tuple, i))

#define QPy_CACHE(self)   ((self)->cache)
#define QPy_ENTRIES(self) ((self)->entries)
#define QPy_SIZE(self)    ((self)->nentries)
#define QPy_LEN(self)     ((self)->used_entries)
#define QPy_GSIZE(self)   ((self)->group_size)
#define QPy_TMPCACHE(self)    NULL
#
#endif // QPy_DEFS_H
