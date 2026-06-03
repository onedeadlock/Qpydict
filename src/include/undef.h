#ifndef QPY_CLEANUP_H
#define QPY_CLEANUP_H

#ifdef MM_SIMD_FLAG
#    undef MM_SIMD_FLAG
#    undef MM_FULL
#    undef MM_NULL
#    undef MM_DEL
#    undef MM_NULL
#    undef MM_DEL
#    undef MM_NGROUP
#    undef MM_KGROUP
#    undef MM_KGET
#    undef OD_
#    undef EV_
#    undef LO_
#    undef HI_
#    undef mm_or
#    undef mm_xor
#    undef mm_and
#    undef mm_dup
#    undef mm_load
#    undef mm_set_full
#    undef mm_set_empty
#    undef mm_has_zero_fast
#    undef mm_has_zero
#    undef mm_mask
#    undef mm_null_fast
#    undef mm_mask_null
#    undef mm_mask_del
#    undef mm_mask_full
#endif
#endif //QPY_CLEANUP_H
