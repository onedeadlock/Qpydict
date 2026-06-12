#ifndef MM_NEON_64_H
#define MM_NEON_64_H
#include "../neon.h"

#if defined(HAVE_NEON) && !defined(_NO_NEON_64)
#    undef  MM_SIMD_FLAG
#    define MM_SIMD_FLAG   0x35

#    define MM_NGROUP      8
#    define MM_KGROUP      3
#    define MM_IDX(v)      ((v) >> 3)
#    define HI_            0x8080808080808080ULL

#    define mm_eq(v, m)    vceq_u8(v, m)
#    define mm_or(v, m)    vorr_u8(v, m)
#    define mm_xor(v, m)   veor_u8(v, m)
#    define mm_and(v, m)   vand_u8(v, m)

#    define mm_dup(v)      vdup_n_u8(v)
#    define mm_load(v)     vld1_u8(v)

#    define mm_set_null() mm_dup(MM_NULL)
#    define mm_set_del()  mm_dup(MM_DEL)

#    define mm_mask(v, m)  (vget_lane_u64(vreinterpret_u64_u8(mm_eq(v, m)), 0) & HI_)

#    define mm_null_fast(v, z)     mm_mask(v, z)
#    define mm_mask_null(v, z)     mm_mask(v, z)
#    define mm_mask_del(v, m, z)   mm_mask(mm_and(v, m), z)
#    define mm_mask_full(v, m, z) (mm_mask_del(v, m, z) ^ HI_)
#endif
#endif // MM_NEON_64_H
