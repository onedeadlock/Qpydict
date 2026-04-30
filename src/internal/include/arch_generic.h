#ifndef QPy_ARCH_GEN_H
#define QPy_ARCH_GEN_H
#include <stdint.h>

#if !defined(QPy_USING_NEON) && !defined(QPy_USING_SSE) && (defined(__LP64__) || defined(UINT64_MAX) || (0xffffffffffffffffull >> 33) > 0)
#    define QPy_NGROUP               8
#    define QPy_PWGROUP              3
#    define QPy_FIXIDX(v)            ((v) >> QPy_PWGROUP)
#    define QPy_OD_                  0x100010001000100ull
#    define QPy_EV_                  0x1000100010001ull
#    define QPy_mm_load(v)           (*(uint64_t *)(void *)(v))
#    define QPy_mm_duplicate(x)      ((x) * 0x101010101010101ull)
#    define QPy_mm_set_empty()       0
#    define QPy_mm_set_occupied()    (QPy_USED * 0x101010101010101ull)
#    define QPy_mm_fast_test_zero(v) (((v) - 0x101010101010101ull) & (~(v) & 0x8080808080808080ull))
#    define QPy_mm_wi_test_zero(v)   (((((v) | QPy_EV_) - QPy_OD_) | (((v) | QPy_OD_) - QPy_EV_)) & (~(v) & 0x8080808080808080ull))
#    define QPy_mm_test_equal(v, m)  QPy_mm_wi_test_zero((v) ^ (m))
#    define QPy_mm_test_empty(v, z)  QPy_mm_fast_test_zero(v)
#    define QPy_mm_test_empty_or_deleted(v, m, z) QPy_mm_wi_test_zero(((v) & (m))
#    define QPy_mm_test_occupied(v, m, z)        (QPy_mm_wi_test_zero((v) & (m)) ^ 0x8080808080808080ULL)
//
#else
#if !defined(_WIN32) && !defined(_WIN64)
#error
#else
// error
#endif
#endif
#endif //QPY_ARCH_GEN_H
