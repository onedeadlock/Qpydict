#ifndef QMAP_SIMD_H
#define QMAP_SIMD_H

#include "./i386_avx.h"
#ifndef QMAP_USING_AVX
    #include "./i386_sse.h"
#ifndef QMAP_USING_SSE2
    #include "./armch_neon.h"
#ifndef QMAP_USING_NEON
    #include "./bit_64_32.h"
#ifndef QMAP_USING_BIT64_32
    #error "Qmap requires atleast avx, sse2, neon or int64 to work but none was enabled"
#endif
#endif
#endif
#endif

#endif //QMAP_SIMD_H
