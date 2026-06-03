#ifndef MM_SSE_H
#define MM_SSE_H
#include <stdint.h>
#ifndef MM_SIMD_FLAG
#if defined(__x86_64__) || defined(__i386__) || defined(__amd__)
#    include <immintrin.h>
#endif

#ifdef __SSE2__
#    define MM_SIMD_FLAG    0x25
#    define MM_NGROUP       16
#    define MM_KGROUP       4
#    define MM_KGET(v)      (v)

#    define mm_eq(v, m)    _mm_cmpeq_epi8(v, m)
#    define mm_or(v, m)    _mm_or_si128(v, m)
#    define mm_xor(v, m)   _mm_xor_si128(v, m)
#    define mm_and(v, m)   _mm_and_si128(v, m)

#    define mm_dup(v)      _mm_set1_epi8(v)
#    define mm_load(v)     _mm_loadu_si128(v)

#    define mm_set_full()  _mm_set1_epi8(MM_FULL)
#    define mm_set_empty() _mm_set1_epi8(MM_NULL) //_mm_setzero_si128()
#    define mm_mask(v, m)  _mm_movemask_epi8(mm_eq(v, m))

#    define mm_null_fast(v, z)    mm_mask(v, z)
#    define mm_mask_null(v, z)    mm_mask(v, z)
#    define mm_mask_del(v, m, z)  mm_mask(mm_and(v, m), z)
#    define mm_mask_full(v, m, z) ~mm_mask_del(v, m, z)
#endif
#endif // MM_SIMD_FLAG
#endif // MM_SSE_H
