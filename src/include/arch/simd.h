#ifndef MM_SIMD_H
#define MM_SIMD_H
#define MM_SIMD_FLAG 0

#define MM_FULL     0x7f
#ifdef MM_ZERO
#    define MM_NULL 0
#    define MM_DEL  0x80
#else
#    define MM_NULL 0xff
#    define MM_DEL  0x80
#endif
#include <stdint.h>
#include <inttypes.h>
#include "avx/simd.h"
#include "sse/simd.h"
#include "neon32/simd.h"
#include "neon64/simd.h"
#include "generic/simd.h"

#if 0 == MM_SIMD_FLAG
#    define MM_SUPPORT 0
#else
#    define MM_SUPPORT 1
#    if   0x15 == MM_SIMD_FLAG // AVX256 support
typedef uint32_t mask_t;
typedef __m256i  mm_t;

#    elif 0x25 == MM_SIMD_FLAG // SSE2 support
typedef uint16_t mask_t;
typedef __m128i  mm_t;

#    elif 0x35 == MM_SIMD_FLAG // use neon 32bit inst.
typedef uint64_t  mask_t;
typedef uint8x8_t mm_t;

#        ifndef INT64_MAX
#            error // neon32 requires a 64bit wide integer for mask
#            undef MM_SUPPORT
#            define MM_SUPPORT 0
#        endif

#    elif 0x45 == MM_SIMD_FLAG // use neon 64bit inst.
typedef uint16_t   mask_t;
typedef uint8x16_t mm_t;

#    else // generic
typedef uint64_t mask_t;
typedef uint64_t mm_t;

#    endif
#endif
#endif // MM_SIMD_H
