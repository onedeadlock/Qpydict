#ifndef MM_GENERIC_SIMD_H
#define MM_GENERIC_SIMD_H

#if !defined(MM_SIMD_FLAG) || !MM_SIMD_FLAG
#    include <stdint.h>

/** Caveat: The operations below should be performed on unsigned int. For smaller than 64 bits types, the constants, from @OD_ - @B7_ must be reduced to fit any of such. */

#if defined(UINT64_MAX) || (0xffffffffffffffff >> 33) > 0
#    undef  MM_SIMD_FLAG
#    define MM_SIMD_FLAG        0x55

#    define MM_NGROUP            8
#    define MM_KGROUP            3
#    define MM_IDX(v)       ((v) >> 3)

#    define OD_             0x100010001000100ULL
#    define EV_             0x1000100010001ULL
#    define LO_             0x101010101010101ULL
#    define HI_             0x8080808080808080ULL
#    define B7_             0x7f7f7f7f7f7f7f7fULL

#    define mm_eq(v, m)    !((v) ^ (m))
#    define mm_or(v, m)     ((v) | (m))
#    define mm_xor(v, m)    ((v) ^ (m))
#    define mm_and(v, m)    ((v) & (m))

#    define mm_dup(v)       ((v) * LO_)
#    define mm_load(v)      (*(uint64_t *)(void *)(v))

#    define mm_set_null()       mm_dup(MM_NULL)
#    define mm_set_del()        mm_dup(MM_DEL)
  
#    define mm_zmovemask_fast(v) mm_and((v - LO_), mm_and(~v,  HI_)) // Has Zero test (after benchmark, this is not much faster (x1.01) than my version below, but i'll still keep it here, and in use)
#    define mm_zmovemask(v)      mm_and(mm_or(mm_or(v, EV_) - OD_, mm_or(v, OD_) - EV_), mm_and(~v, HI_)) // with correct indices
#    // _mm_movemask
#    define mm_movemask(v)       mm_zmovemask(mm_xor(~mm_and(x, HI_), B7_))

#    define mm_mask(v, m)        mm_zmovemask(mm_xor(v, m))

#    if defined(MM_NULL) && (0xff == MM_NULL)
#        define mm_mask_null(v, ...)  mm_and(mm_and(v, (v) >> 7), HI_)
#        define mm_null_fast(v, ...)  mm_mask_null(v)
#        define mm_mask_del(v,  ...)  mm_and(v, HI_)
#    else
#        define mm_mask_null(v, ...)  mm_mask(v, 0ULL)
#        define mm_null_fast(v, ...)  mm_zmovemask_fast(v)
#        define mm_mask_del(v,  ...)  mm_mask(mm_and(v, B7_), 0ULL)
#    endif
#    define mm_mask_full(v, ...) mm_and(~mm_mask_del(v), HI_)
#endif
#endif // MM_SIMD_FLAG
#endif // MM_GENERIC_SIMD_H
