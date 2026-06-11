#ifndef MM_NEON_H
#define MM_NEON_H
#if !defined(MM_SIMD_FLAG) && !defined(MODE_GENERIC)
#include <stdint.h>

#if defined(__arm__) || defined(__ARM_ARCH) && __ARM_ARCH == 7
#   define HAVE_ARM32 1
#endif
#if defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#   define HAVE_ARM64 1
#endif

#if  defined(HAVE_ARM32)   || defined(HAVE_ARM64)
#    if defined(__ARMEB__) || defined(__AARCH64EB__)
#        error unsupported ARM target (Big Endian).
#    endif
#    if HAVE_ARM32 && (!defined(__ARM_NEON) || !defined(__ARM_NEON__))
#        error missing `-mfpu=neon/-mfpu=neon-fp-armv8` or perhaps the NEON instruction set is unsupported.
#    else
#        // for Aarch64, set `+simd` in the compiler flags as appropriate. 
#    endif

#    // full 16 lane is default in Aarch64 mode or in missing int64.
#    // set explicitly for Armv7-A/Aarch32 mode with `-D_MODE_NEON128`
#    if HAVE_ARM64 || defined(MODE_NEON128) || !defined(INT64_MAX)
#       define _NO_NEON_64 1
#    endif

#    if defined(_M_ARM64EC)
#         include <arm64_neon.h>
#    else
#        include <arm_neon.h>
#    endif

#    define HAVE_NEON 1
#endif
#endif
#endif // MM_NEON_H
