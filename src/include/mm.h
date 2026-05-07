#ifndef QPy_ARCH_GENERIC_H
#define QPy_ARCH_GENERIC_H
#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__) || defined(__amd__)
#    include <immintrin.h>
#    define QPy_MM_SSE
#    if QPy_FORCE_AVX && (defined(__AVX__) || defined(__AVX2__))
#        define QPy_NGROUP              32
#        define QPy_PWGROUP             5
#        define QPy_FIXIDX(v)           (v)
#        define mm_load(v)           _mm256_loadu_si256((const __m256i *)(void *)(v))
#        define mm_duplicate(x)      _mm256_set1_epi8((uint8_t)(x))
#        define mm_set_zero()        _mm256_setzero_si256()
#        define mm_set_occupied()    _mm256_set1_epi8((uint8_t)QPy_USED)
#        define mm_test_equal(v, m)  _mm256_movemask_epi8(_mm256_cmpeq_epi8(v, m))
#        define mm_test_empty(v, z)  mm_test_equal(v, z)
#        define mm_test_empty_or_deleted(v, m, z) mm_test_equal(_mm256_and_si256(v, m), z)
#        define mm_test_occupied(v, m, z)        ~mm_test_empty_or_deleted(v, m, z)
#
#    elif defined(__SSE2__)
#        define QPy_NGROUP               16
#        define QPy_PWGROUP              4
#        define QPy_FIXIDX(v)            (v)
#        define mm_load(v)           _mm_loadu_si128((const __m128i *)(v))
#        define mm_duplicate(x)      _mm_set1_epi8((uint8_t)(x))
#        define mm_set_zero()        _mm_setzero_si128();
#        define mm_set_occupied()    _mm_set1_epi8((uint8_t)QPY_USED)
#        define mm_test_equal(v, m)  _mm_movemask_epi8(_mm_cmpeq_epi8((v), (m)))
#        define mm_test_empty(v, z)  mm_test_haseq__(v, z)
#        define mm_test_empty_or_deleted(v, m, z) mm_test_haseq__(_mm_and_si128(v, m), z)
#        define mm_test_occupied(v, m, z)        ~mm_test_emptydel__(v, m, z)
#    endif
#
#elif defined(__aarch64__) || defined(__arm__) || (defined(__ARM_ARCH) && __ARM_ARCH >= 7)
#    define QPy_ARCH_ARM
#    ifdef __ARM_NEON
#        include <arm_neon.h>
#        define QPy_MM_NEON
#    endif
#    if defined(__aarch64__) || (defined(__ARM_ARCH) && (__ARM_ARCH > 7)) || defined(QPy_FORCE_ARM128)
#        define QPy_NEON_ARM128
#        define QPy_NGROUP        16
#        define QPy_PWGROUP        4
#        define QPy_FIXIDX(v)     (v)
#    else // Neon
#        define QPy_NGROUP         8
#        define QPy_PWGROUP        3
#        define QPy_FIXIDX(v)    ((v) >> QPy_PWGROUP)
#    endif
#
#elif defined(__LP64__) || defined(UINT64_MAX) || (0xffffffffffffffffull >> 33) > 0
#    define QPy_NGROUP           8
#    define QPy_PWGROUP          3
#    define QPy_FIXIDX(v)        ((v) >> QPy_PWGROUP)
#    define OD_                  0x100010001000100ull
#    define EV_                  0x1000100010001ull
#    define mm_load(v)           (*(uint64_t *)(void *)(v))
#    define mm_duplicate(x)      ((x) * 0x101010101010101ull)
#    define mm_set_empty()       0
#    define mm_set_occupied()    (QPy_USED * 0x101010101010101ull)
#    define mm_fast_test_zero(v) (((v) - 0x101010101010101ull) & (~(v) & 0x8080808080808080ull))
#    define mm_wi_test_zero(v)   (((((v) | EV_) - OD_) | (((v) | OD_) - EV_)) & (~(v) & 0x8080808080808080ull))
#    define mm_test_equal(v, m)  mm_wi_test_zero((v) ^ (m))
#    define mm_test_empty(v, z)  mm_fast_test_zero(v)
#    define mm_test_empty_or_deleted(v, m, z) mm_wi_test_zero(((v) & (m))
#    define mm_test_occupied(v, m, z)        (mm_wi_test_zero((v) & (m)) ^ 0x8080808080808080ull)
#else
#    define QPy_MM_UNSUPPORTED
#endif
#endif //QPY_ARCH_GENERIC_H
