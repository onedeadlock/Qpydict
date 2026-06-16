#ifndef MM_SSE_H
#define MM_SSE_H
#if !defined(MM_SIMD_FLAG) || !MM_SIMD_FLAG

#if defined(__x86_64__) || defined(__i386__) || defined(__amd__)
#    include <immintrin.h>
#endif

#ifdef __SSE2__
#    undef  MM_SIMD_FLAG
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

#    define mm_set_null()  _mm_set1_epi8(MM_NULL)
#    define mm_set_del()   _mm_set1_epi8(MM_DELL)
#    define mm_cmp(v, m)   _mm_movemask_epi8(mm_eq(v, m))

#    define mm_null_fast(v, z)    mm_cmp(v, z)
#    define mm_cmp_null(v, z)     mm_cmp(v, z)
#    define mm_cmp_del(v, m, z)   mm_cmp(mm_and(v, m), z)
#    define mm_cmp_full(v, m, z) ~mm_cmp_del(v, m, z)
#endif
#endif // MM_SIMD_FLAG
#endif // MM_SSE_H
