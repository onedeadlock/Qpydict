#ifndef MM_NEON_H
#define MM_NEON_H
#include <stdint.h>
#ifndef MM_SIMD_FLAG
#if defined(__ARM_ARCH) && __ARM_ARCH >= 7
#define HAVE_ARM_ARCH_
#endif
#if defined(__aarch64__) || defined(__arm__) || HAVE_ARM_ARCH_
#    if HAVE_ARM_ARCH_ && __ARM_ARCH >= 8 && !defined(NO_NEON64)
#        define _NEON_32 0 // use neon 64 instead
#    endif
#    define _NEON_32 1
#    include <arm_neon.h>
#endif
#undef HAVE_ARM_ARCH_

#if defined(_NEON_32) && _NEON_32 
#    define MM_SIMD_FLAG   0x35
#    define MM_NGROUP      8
#    define MM_KGROUP      3
#    define MM_KGET(v)     ((v) >> MM_KGROUP)
#    define HI_            0x8080808080808080ULL

#    define mm_eq(v, m)    vceq_u8(v, m)
#    define mm_or(v, m)    vorr_u8(v, m)
#    define mm_xor(v, m)   veor_u8(v, m)
#    define mm_and(v, m)   vand_u8(v, m)

#    define mm_dup(v)      vdup_n_u8(v)
#    define mm_load(v)     vld1_u8(v)

#    define mm_set_full()  mm_dup(MM_FULL)
#    define mm_set_empty() mm_dup(MM_NULL)

#    define mm_mask(v, m)  vget_lane_u64(vreinterpret_u64_u8(mm_equal(v, m)), 0) & HI_)

#    define mm_null_fast(v, z)     mm_mask(v, z)
#    define mm_mask_null(v, z)     mm_mask(v, z)
#    define mm_mask_del(v, m, z)   mm_mask(mm_and(v, m), z)
#    define mm_mask_full(v, m, z) (mm_mask_del(v, m, z) ^ HI_)
#endif
#endif // MM_SIMD_FLAG
#endif // MM_NEON_H
