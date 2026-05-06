#ifndef QPy_ARM_NEON_H
#define QPy_ARM_NEON_H

#if defined(__aarch64__) || defined(__arm__) || (defined(__ARM_ARCH) && __ARM_ARCH >= 7)
#    define QPy_ARCH_ARM
#    ifdef __ARM_NEON
#        include <arm_neon.h>
#        define QPy_USING_NEON
#    endif
#    if defined(__aarch64__) || (defined(__ARM_ARCH) && (__ARM_ARCH > 7)) || defined(QPy_FORCE_ARM128)
#        #define QPy_USE_ARM128
#    endif
#endif

#if defined(QPy_USING_NEON)
#    if defined(QPy_USE_ARM128)
#        define QPy_NGROUP        16
#        define QPy_PWGROUP        4
#        define QPy_FIXIDX(v)     (v)
#    else // Neon
#        define QPy_NGROUP         8
#        define QPy_PWGROUP        3
#        define QPy_FIXIDX(v)    ((v) >> QPy_PWGROUP)
#    endif
#endif //QPy_ARM_NEON_H
