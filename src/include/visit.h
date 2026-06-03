#ifndef QPy_VISIT_H
#define QPy_VISIT_H
#include <assert.h>
#include "mm.h"
#include "defs.h"
#include "types.h"

#define dict_ qpydict_
#define Dict  QPyDictObject

#define DICT_VAR(m) ____var_QPy##m##____
#define DICT_VNXT(k)        ((k) += NGROUP)
#define DICT_VSLOT(h, p, g) (((hash) + (p)) & (g))
#define DICT_VNXTSLOT(p, k) ((p) += (k))
#define DICT_VCALL(mth, v, ...)                  \
    mth((v),                                     \
        DICT_VAR(j),   DICT_VAR(mask), mm_test_has_entry,\
        mm_scan_mask,  DICT_VNXT,      DICT_VAR(p),      \
        DICT_VAR(k),   DICT_VAR(g),    DICT_VSLOT,       \
        DICT_VNXTSLOT, __VA_ARGS__                       \
       )

#define DICT_FOR_(v, j, m, cmp, scan, next, ...)   \
    for (size_t j=0; v.i < v.size; next(v.i)) \
        for (mask_t m=(v.mm=mm_load(v.grp+v.i), cmp(v.mm)); (m) AND (j=scan(m)+v.i); m &= m - 1)

#define DICT_FOR_MSK_(v, j, m, cmp, _s, next, ...) \
    for (size_t j=0; v.i < v.size; next(v.i))\
        for (mask_t m=(v.mm=mm_load(v.grp+v.i), cmp(v.mm)); m;)

#define DICT_VPFOR_(v, j, m, _c, _s, next, p, k, g, slot, next_slot, hash, ...) \
    for (size_t p=0, k=0, j=0, g=v.size-1; (1); v.i=slot(hash, p, g), next_slot(p, k), next(k))

#define DICT_VPFOR_MSK_(v, j, m, _c, scan, _t, _p, _k, _g, _s, _n, cmp, xmm, ...) \
    for (mask_t m=(v.mm=mm_load(v.grp+v.i), cmp(v.mm, xmm)); (m) AND (j=scan(m)+v.i); m &= m - 1)

#define dict_for_each(v)      DICT_VCALL(DICT_FOR_, v)
#define dict_for_each_mask(v) DICT_VCALL(DICT_FOR_MSK_, v)
#define dict_for_each_p(v, h) DICT_VCALL(DICT_VPFOR_, v, h)
#define dict_for_each_mask_p(v, x) DICT_CALL(DICT_VPFOR_MSK_, v, mm_test_equal, x)

struct visit_t
{
    ssize_t   i;    // current group index
    ssize_t   size; // aggr. group size
    mm_t      mm;   // save each mask temporarily
    cache_t  *grp;  // current group
    khpair_t *kh;   // array of key-value pairs 
    Type **   val;  // array of values
};

local_inline struct visit_t *
vget_empty(void)
{
    static struct visit_t v = {0, 0}; // TODO
    return &v;
}

local_inline struct visit_t *
vset_struct(struct visit_t *v, const Dict *dict, size_t UNUSED(n))
{
    assert(NULL != dict AND NULL != v);

    v->i    = 0;
    v->size = dict->group_capacity;
    v->grp  = dict->entries.cache;
    v->kh   = dict->entries.kh;
    v->val  = dict->entries.values;
    return v;
}

local_inline struct visit_t *
vset_struct_n(struct visit_t *v, const Dict *dict, const size_t n)
{
    // TODO: set properly
    assert(NULL != dict AND NULL != v);
    
    if (n > dict->capacity)
        *v=*(struct visit_t *)vget_empty();
    v->i    = ALIGN(n, NGROUP);
    v->size = dict->group_capacity;
    v->grp  = dict->entries.cache;
    v->kh   = dict->entries.kh;
    v->val  = dict->entries.values; 
    return v;
}

/** @vset_hint: set entries size hint */
local_inline void
vset_hint(struct visit_t *v, const size_t n)
{
    v->size = n; // TODO
}

#define vget_idx(v)     DICT_VAR(j)     //  var j
#define vget_mask(v)    DICT_VAR(mask)  // var mask

#define vset_next(v)    (vget_mask(v)=0) // next group
#define vget_grpidx(v)  (v).i   // current group index
#define vget_group(v)   (v).mm  // current group
#define vget_key(v)     vget_keyhash(v).key  // key
#define vget_hash(v)    vget_keyhash(v).hash // hash
#define vget_value(v)   (v).val[vget_idx(v)] // value
#define vget_keyhash(v) (v).kh[vget_idx(v)]  // kh array

#endif // QPY_VISIT_H

