#ifndef MM_NEON_128_H
#define MM_NEON_128_H
#include "../neon.h"

#if  defined(HAVE_NEON) && defined(_NO_NEON_64)
#    undef  MM_SIMD_FLAG
#    define MM_SIMD_FLAG   0x45

#    define MM_NGROUP      16
#    define MM_KGROUP      4
#    define MM_IDX(v)      (v)
#    define HI_            0x8080808080808080ULL

#    define mm_eq(v, m)    vceqq_u8(v, m)
#    define mm_or(v, m)    vorrq_u8(v, m)
#    define mm_xor(v, m)   veorq_u8(v, m)
#    define mm_and(v, m)   vandq_u8(v, m)

#    define mm_dup(v)      vdupq_n_u8(v)
#    define mm_load(v)     vld1q_u8(v)

#    define mm_set_null() mm_dup(MM_NULL)
#    define mm_set_del()  mm_dup(MM_DEL)

#    define mm_cmp(v, m)  mm_movemask(mm_eq(v, m))

#    define mm_null_fast(v, z)    mm_cmp(v, z)
#    define mm_cmp_null(v, z)     mm_cmp(v, z)
#    define mm_cmp_del(v, m, z)   mm_cmp(mm_and(v, m), z)
#    define mm_cmp_full(v, m, z) (mm_cmp_del(v, m, z) ^ HI_)
#
static inline uint16_t
mm_movemask(const uint8x16_t v)
{
    // Neon equivalent of _mm_movemask_epi8
    static const uint8x16_t w = {1, 2, 4, 8, 16, 32, 64, 128,
                                 1, 2, 4, 8, 16, 32, 64, 128};
    uint64x2_t sum;

#  if HAVE_ARM64
    sum = vandq_u8(v, w);
    sum = vpaddq_u8(sum, sum);
    sum = vpaddq_u8(sum, sum);
    sum = vpaddq_u8(sum, sum);
    return vgetq_lane_u16(vreinterpretq_u16_u8(sum), 0);
#  else
    // generic fallback: add and widen lane
    sum = vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vandq_u8(v, w))));
    // pack both halves into a word
    return vgetq_lane_u64(sum, 0) | (vgetq_lane_u64(sum, 1) << 8);
#  endif
}
#endif
#endif // MM_NEON_128_H
