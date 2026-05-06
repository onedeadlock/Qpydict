#ifndef QPy_ARCH_I386_H
#define QPy_ARCH_I386_H

#if defined(__x86_64__) || defined(__i386__) || defined(__amd__)
#    include <immintrin.h>
#    define QPY_USING_SSE
#endif

#if QPy_FORCE_AVX && (defined(__AVX__) || defined(__AVX2__))
#    define QPy_NGROUP              32
#    define QPy_PWGROUP             5
#    define QPy_FIXIDX(v)           (v)
#    define QPy_mm_load(v)           _mm256_loadu_si256((const __m256i *)(void *)(v))
#    define QPy_mm_duplicate(x)      _mm256_set1_epi8((uint8_t)(x))
#    define QPy_mm_set_zero()        _mm256_setzero_si256()
#    define QPy_mm_set_occupied()    _mm256_set1_epi8((uint8_t)QPy_USED)
#    define QPy_mm_test_equal(v, m)  _mm256_movemask_epi8(_mm256_cmpeq_epi8(v, m))
#    define QPy_mm_test_empty(v, z)  QPy_mm_test_equal(v, z)
#    define QPy_mm_test_empty_or_deleted(v, m, z) QPy_mm_test_equal(_mm256_and_si256(v, m), z)
#    define QPy_mm_test_occupied(v, m, z)        ~QPy_mm_test_empty_or_deleted(v, m, z)
#
#elif defined(__SSE2__)
#    define QPy_NGROUP               16
#    define QPy_PWGROUP              4
#    define QPy_FIXIDX(v)            (v)
#    define QPy_mm_load(v)           _mm_loadu_si128((const __m128i *)(v))
#    define QPy_mm_duplicate(x)      _mm_set1_epi8((uint8_t)(x))
#    define QPy_mm_set_zero()        _mm_setzero_si128();
#    define QPy_mm_set_occupied()    _mm_set1_epi8((uint8_t)QPY_USED)
#    define QPy_mm_test_equal(v, m)  _mm_movemask_epi8(_mm_cmpeq_epi8((v), (m)))
#    define QPy_mm_test_empty(v, z)  QPy_mm_test_haseq__(v, z)
#    define QPy_mm_test_empty_or_deleted(v, m, z) QPy_mm_test_haseq__(_mm_and_si128(v, m), z)
#    define QPy_mm_test_occupied(v, m, z)        ~QPy_mm_test_emptydel__(v, m, z)
#endif
#
#endif //QPY_I386_AVX_H
