#ifndef MM_SIMD_H
#define MM_SIMD_H
#define MM_SIMD_FLAG 0

#define MM_AVX     0x15
#define MM_SSE     0x25
#define MM_NEON    0x35
#define MM_NEON128 0x45

#ifdef MM_ZERO
#    define MM_NULL 0
#    define MM_DEL  0x80
#else
#    define MM_NULL 0xff
#    define MM_DEL  0x80
#endif

#include <stdint.h>   // uint64_t
#include "avx/simd.h"
#include "sse/simd.h"

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#    define MM_SUPPORT 0
#    error target is Big Endian but Big Endian is unsupported
# elif defined(__BIG_ENDIAN__)
#    define MM_SUPPORT 0
#    error target is Big Endian but Big Endian is unsupported
#else
#include "neon/64/simd.h"
#include "neon/128/simd.h"
#include "generic/simd.h"
#endif

#if MM_SIMD_FLAG  ==  0
#   define MM_SUPPORT 0
#else
#   define MM_SUPPORT 1
#endif

 // use AVX for simd op
#if MM_AVX == MM_SIMD_FLAG
typedef uint32_t mask_t;
typedef __m256i  mm_t;
#endif

 // use SSE for simd op
#if MM_SSE == MM_SIMD_FLAG
typedef uint16_t mask_t;
typedef __m128i  mm_t;
#endif

// use NEON 8 byte lane only
#if MM_NEON == MM_SIMD_FLAG
typedef uint64_t  mask_t;
typedef uint8x8_t mm_t;
#endif

// use NEON full 16 byte lane only
#if MM_NEON128 == MM_SIMD_FLAG
typedef uint16_t   mask_t;
typedef uint8x16_t mm_t;
#endif

// generic fallback with int64 (8 lane)
#if MM_SIMD_FLAG != 0 && MM_SIMD_FLAG > 0x45
typedef uint64_t mask_t;
typedef uint64_t mm_t;
#endif
#endif // MM_SIMD_H
