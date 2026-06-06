#ifndef MM_GENERIC_SIMD_H
#define MM_GENERIC_SIMD_H
#include <stdint.h>
#include "flag.h"

#ifndef MM_SIMD_FLAG
#if defined(__LP64__) || defined(UINT64_MAX) || (0xffffffffffffffffull >> 33) > 0
#    define MM_SIMD_FLAG 0x45
#    define MM_NGROUP            8
#    define MM_KGROUP            3
#    define MM_KGET(v)      ((v) >> MM_KGROUP)
#    define OD_             0x100010001000100ULL
#    define EV_             0x1000100010001ULL
#    define LO_             0x101010101010101ULL
#    define HI_             0x8080808080808080ULL
#    define mm_eq(v, m)    !((v) ^ (m))
#    define mm_or(v, m)     ((v) | (m))
#    define mm_xor(v, m)    ((v) ^ (m))
#    define mm_and(v, m)    ((v) & (m))

#    define mm_dup(v)       ((v) * LO_)
#    define mm_load(v)      (*(uint64_t *)(void *)(v))

#    define mm_set_full()       mm_dup(MM_FULL)
#    define mm_set_null()       mm_dup(MM_NULL)
#    define mm_has_zero_fast(v) mm_and((v - LO_), mm_and(~v,  HI_)) // (v - LO_) & (~v & HI_)
#    define mm_has_zero(v)      mm_and(mm_or(mm_or(v, EV_) - OD_, mm_or(v, OD_) - EV_), mm_and(~v, HI_)) // (((v | EV_) - OD_) | ((v | OD_) - EV_)) & (~v & HI_) 

#    define mm_mask(v, m)         mm_has_zero(mm_xor(v, m))
#    define mm_null_fast(v, z)    mm_has_zero_fast(mm_xor(v, z))
#    define mm_mask_null(v, z)    mm_mask(v, z)
#    define mm_mask_del(v, m, z)  mm_mask(mm_and(v, m))
#    define mm_mask_full(v, m, z) mm_xor(mm_mask(mm_and(v, m), z), HI_) // has_zero(v & m) ^ HI_
#endif
#endif // MM_SIMD_FLAG
#endif // MM_GENERIC_SIMD_H
