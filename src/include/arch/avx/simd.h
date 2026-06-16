#ifndef MM_AVX_H
#define MM_AVX_H
#if !defined(MM_SIMD_FLAG) || !MM_SIMD_FLAG

#if defined(__x86_64__) || defined(__i386__) || defined(__amd__) || defined(_M_AMD64)
#    include <immintrin.h>
#endif

#ifdef __AVX__
#    undef  MM_SIMD_FLAG
#    define MM_SIMD_FLAG    0x15

#    define MM_NGROUP       32
#    define MM_KGROUP       5
#    define MM_IDX(v)       (v)

#    define mm_eq(v, m)    _mm256_cmpeq_epi8(v, m)
#    define mm_or(v, m)    _mm256_or_si256(v, m)
#    define mm_xor(v, m)   _mm256_xor_si256(v, m)
#    define mm_and(v, m)   _mm256_and_si256(v, m)

#    define mm_dup(v)      _mm256_set1_epi8(v)
#    define mm_load(v)     _mm256_loadu_si256(v)

#    define mm_set_null()  _mm256_set1_epi8(MM_FULL)
#    define mm_set_del()   _mm256_set1_epi8(MM_DEL)
#    define mm_cmp(v, m)   _mm256_movemask_epi8(mm_eq(v, m))

#    define mm_null_fast(v, z)    mm_cmp(v, z)
#    define mm_cmp_null(v, z)     mm_cmp(v, z)
#    define mm_cmp_del(v, m, z)   mm_cmp(mm_and(v, m), z)
#    define mm_cmp_full(v, m, z) ~mm_cmp_del(v, m, z)
#endif
#endif // MM_SIMD_FLAG
#endif // MM_AVX_H
