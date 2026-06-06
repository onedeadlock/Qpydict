#ifndef MM_SIMD_H
#define MM_SIMD_H
#define MM_SIMD_FLAG 0

#define MM_AVX    0x15
#define MM_SSE    0x25
#define MM_NEON   0x35
#define MM_NEON64 0x45

#define MM_FULL     0x7f
#ifdef MM_ZERO
#    define MM_NULL 0
#    define MM_DEL  0x80
#else
#    define MM_NULL 0xff
#    define MM_DEL  0x80
#endif
#include <stdint.h>   // uint64_t
#include "../defs.h" // BSR
#include "avx/simd.h"
#include "sse/simd.h"
#include "neon32/simd.h"
#include "neon64/simd.h"
#include "generic/simd.h"

#if 0 == MM_SIMD_FLAG
#    define MM_SUPPORT 0
#else
#    define MM_SUPPORT 1
#    if   MM_AVX == MM_SIMD_FLAG // using AVX
typedef uint32_t mask_t;
typedef __m256i  mm_t;
#    elif MM_SSE == MM_SIMD_FLAG // using SSE2
typedef uint16_t mask_t;
typedef __m128i  mm_t;
#    elif MM_NEON == MM_SIMD_FLAG // using neon 32bit
typedef uint64_t  mask_t;
typedef uint8x8_t mm_t;
#        ifndef INT64_MAX
#            error // neon32 requires a 64bit wide integer for mask
#            undef  MM_SUPPORT
#            define MM_SUPPORT 0
#        endif
#    elif MM_NEON64 == MM_SIMD_FLAG // using neon 64bit
typedef uint16_t   mask_t;
typedef uint8x16_t mm_t;
#    else // generic with quadword int
typedef uint64_t mask_t;
typedef uint64_t mm_t;
#    endif
#endif

// scan bitmask from mm_mask*
#define mm_scan(v) MM_KGET(BSR(v))

#endif
#endif // MM_SIMD_H
