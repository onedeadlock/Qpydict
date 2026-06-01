#ifndef QPy_VISIT_H
#define QPy_VISIT_H
#include "mm.h"

#define DVVAR(m) ____var_QPy##m##____

#define DVCALL(__mth__, __v__, ...)                    \
    __mth__((__v__),                                   \
        DVVAR(j),     DVVAR(mask), mm_test_has_entry,  \
        mm_scan_mask, NEXT_GROUP,  DVVAR(p),           \
        DVVAR(k),     DVVAR(g),    dict_slot,          \
        probe_next_dict_slot,      __VA_ARGS__         \
       )

#define DVFOR_(v, j, m, cmp, scan, next, ...)   \
    for (size_t j=0; v.i < v.size; next(v.i)) \
        for (mask_t m=(v.mm=mm_load(v.grp+v.i), cmp(v.mm)); (m) AND (j=scan(m)+v.i); m &= m - 1)

#define DVFOR_MSK_(v, j, m, cmp, _s, next, ...) \
    for (size_t j=0; v.i < v.size; next(v.i))\
        for (mask_t m=(v.mm=mm_load(v.grp+v.i), cmp(v.mm)); m;)

#define DVPFOR_(v, j, m, _c, _s, _n, p, k, g, slot, next_slot, ...) \
    for (size_t p=0, k=0, j=0, g=v.i; (1); v.i=slot(p, g, v.size), next_slot(p, k))

#define DVPFOR_MSK_(v, j, m, _c, scan, _t, _p, _k, _g, _s, _n, cmp2, xmm, ...) \
    for (mask_t m=(v.mm=mm_load(v.grp+v.i), cmp2(v.mm, xmm)); (m) AND (j=scan(m)+v.i); m &= m - 1)

#define dict_for_each(v)           DVCALL(DVFOR_)
#define dict_for_each_mask(v)      DVCALL(DVFOR_MSK_, v)
#define dict_for_each_p(v)         DVCALL(DVPFOR_, v)
#define dict_for_each_mask_p(v, x) DVCALL(DVPFOR_MSK_, v, mm_test_equal, x)

struct visit_t
{
    ssize_t  i;    // current group index
    ssize_t  size; // aggr. group size
    mm_t     mm;   // save each mask temporarily
    cache_t  grp;  // current group
    Type **  kh;   // pointer to array of key-value pairs 
    Type **  val;  // pointer to array of values
};

local_inline const visit_t *
vget_empty(void)
{
    static struct visit_t v = {0, .grp=NULL}; // TODO
    return &v;
}

local_inline visit_t *
vset_struct(struct visit_t *v, const Dict *dict, size_t UNUSED(n))
{
    assert(NULL != dict AND NULL != v);

    v->i    = 0;
    v->size = dict->group_capacity;
    v->grp  = dict->entries.cache;
    v->kh   = dict->entries.kh;
    v->val  = dict->entries.values 
    return v;
}

local_inline visit_t *
vset_struct_n(struct visit_t *v, const Dict *dict, const size_t n)
{
    // TODO: set properly
    assert(NULL != dict AND NULL != v);
    
    if (n > dict->capacity)
        *v=*(visit_t *)vget_empty();
    v->i    = 0;
    v->size = dict->group_capacity;
    v->grp  = dict->entries.cache;
    v->kh   = dict->entries.kh;
    v->val  = dict->entries.values 
    return v;
}

/** @vset_hint: set entries size hint */
local_inline visit_t
vset_hint(struct visit_t *v, const size_t n)
{
    return v->size = n; // TODO
}

#define vget_idx(v)     DVVAR(j)     //  var j
#define vget_mask(v)    DVVAR(mask)  // var mask

#define vset_next(v)    (vget_mask(v)=0) // next group
#define vget_grpidx(v)  (v).i   // current group index
#define vget_group(v)   (v).mm)  // current group
#define vget_key(v)     vget_keyhash(v).key  // key
#define vget_hash(v)    vget_keyhash(v).hash // hash
#define vget_value(v)   (v).val[vget_idx(v)] // value
#define vget_keyhash(v) (v).kh[vget_idx(v)]  // kh array

#endif // QPY_VISIT_H

