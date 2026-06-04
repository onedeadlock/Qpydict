#ifndef MM_NEON64_H
#define MM_NEON64_H
#include <stdint.h>
#ifndef MM_SIMD_FLAG
#if defined(__ARM_ARCH) && __ARM_ARCH >= 7
#define HAVE_ARM_ARCH_
#endif
#if defined(__aarch64__) || defined(__arm__) || HAVE_ARM_ARCH_
#    include <arm_neon.h>
#endif
#undef HAVE_ARM_ARCH_

#ifdef __ARM_NEON
#    define MM_SIMD_FLAG   0x35
#    define MM_NGROUP      16
#    define MM_KGROUP      4
#    define MM_KGET(v)     (v)
#    define HI_            0x8080808080808080ULL

#    define mm_eq(v, m)    vceqq_u8(v, m)
#    define mm_or(v, m)    vorrq_u8(v, m)
#    define mm_xor(v, m)   veorq_u8(v, m)
#    define mm_and(v, m)   vandq_u8(v, m)

#    define mm_dup(v)      vdupq_n_u8(v)
#    define mm_load(v)     vld1q_u8(v)

#    define mm_set_full()  mm_dup(MM_FULL)
#    define mm_set_empty() mm_dup(MM_NULL)

#    define mm_mask(v, m)  mm_movemask(mm_equal(v, m))

#    define mm_null_fast(v, z)     mm_mask(v, z)
#    define mm_mask_null(v, z)     mm_mask(v, z)
#    define mm_mask_del(v, m, z)   mm_mask(mm_and(v, m), z)
#    define mm_mask_full(v, m, z) (mm_mask_del(v, m, z) ^ HI_)
// neon equivalent of _mm_movemask
static inline uint16_t
mm_movemask(const uint8x16_t v)
{
    static const uint8x16_t w = {1, 2, 4, 8, 16, 32, 64, 128,
                                 1, 2, 4, 8, 16, 32, 64, 128};
    uint64x2_t sum;

#  if defined(__aarch64__) OR (__ARM_ARCH >= 8)
    // vpaddq_u8 is faster and work on the entire 64x2 lane. unfortunately, it is missing in Arm < 7
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
#endif
}
#endif
#endif // MM_SIMD_FLAG
#endif // MM_NEON64_H
