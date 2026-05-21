#ifndef QPy_DICT_H
#define QPy_DICT_H
#ifndef QPy_MM_UNSUPPORTED
#define dict_ qpydict_
#define Dict  QPyDictObject

/**                     DICT IMPLEMENTATION
 * (1) Internal
 * (2) Rehashing and Resizing
 *  Resizing dict follows the conventional method of resizing containers except every resize is done with alignment to a basic size and a power of 2. Since every size op is a power of 2 and a multiple of group, a resize with a size between say 2^j and 2^k will end up as MUL(2^k, basicsize) (your load factor may also buff things up, be careful!).
 *
 * POV: resizing down is not advicable. In short words, it leaves the container in an unpredictable state (if you don't know what you're doing), so it is better to allocate the much/little size as needed beforehand.
 * In leu with the above, a call to `advice_size_requirement*` functions with your size, should tell you the exact size you get as would use internally after resize. for example:
 *
 * `advice_size_requirement(n)` // returns minimum size that would fit n entries without any future rehash (this is the default, and is so, unless the returned size would overflow)
 *
 * `advice_size_requirement_resize_down(dict)` // return safe size that guarantees no loss in entries
 *
 * ...and the rest.
 * Usually, unless FORCE_RESIZE is defined, resizing down may not actually rehash if it is possible not to. This is unlike rehashing, which rehashes regardless of any change
 */

#define PTR(ptr)   (void *)(ptr)
#define DPTR(dptr) (*(void **)(dptr))
#define SHPTR(ptr) (uint16_t *)PTR(ptr)
#define LONG(x)    (long)(x)

#define DCR(x) --(x)
#define ICR(x) ++(x)

#define PLUSNGROUP(x) ((x)   + NGROUP)
#define LASTGRP(x)    (ALIGNU(x/NGROUP, NGROUP) - NGROUP)
#define NEXT_GROUP(x) ((x) += NGROUP)

#define out_of_range_lf(lf) ((lf) < .3 OR(lf) > 1.)
#define inc_entry_size(d)   ++((d)->used_size)

#define dict_slot(d, p, i) (((p) + (i)) & (i - 1)) // TODO
#define probe_next_dict_slot(p, c) ((p) += 1 + (c)++)

#ifndef CACHE_TAG_NOZERO
local_inline int PURE(ctag)(const uint64_t v)
{
    return ((v & 0xff) - ((v & 0xff) * 0x2041u >> 20) * 127) + 1;
}
#else
#define ctag(v) ((v) & 0xff)
#endif

local_inline size_t
PURE(find_group_from_hash)(const hash_t hash,
                           const size_t size)
{
    return ALIGN(hash & (size - 1), NGROUP) / NGROUP;
}

warn_unused local void *
caligned_malloc(void *memdptr,
                const uint16_t align_size,
                const size_t   size)
{
    assert(align_size & (align_size - 1)); // ensure align_size is a power of 2 (just a rule; multiples still works)

        const uint16_t max_offset = sizeof(uint16_t) + (align_size - 1);

    if (size > (SIZE_MAX - max_offset_size))
    {
        // `size + alignment` would cause an overflow
        return NULL;
    }

    void *ptr = malloc(size + max_offset_size);
    if (NULL == ptr)
        return ptr;

    void *kptr = ALIGNU((uintptr_t)ptr + sizeof(uint16_t), align_size);

    ((uint16_t *)kptr)[-1] = (uintptr_t)kptr - (uintptr_t)ptr; // store offset size just before the aligned memory

    DDPTR(memdptr) = kptr;
    return kptr;
}

local void caligned_free(void *memptr)
{
    if (memptr == NULL)
        return;

    const uint16_t offset   = ((uint16_t *)memptr)[-1];
    const void *   memstart = (uintptr_t)memptr - offset;

    return free(memstart);
}

local_inline void *
caligned_calloc(const size_t size,
                const uint16_t align_size)
{
    void *ptr = NULL;

    if (caligned_malloc(&ptr, align_size, size))
        return memset(ptr, 0, size);
    return ptr;
}

warn_unused local_inline void *
caligned_malloc_set(const size_t size,
                    const uint16_t align_size,
                    const int fchar)
{
    void *ptr = NULL;

    if (caligned_malloc(&ptr, align_size, size))
        return memset(ptr, fchar, size);
    return ptr;
}

local void *caligned_realloc(void *memdptr, uint16_t align_size, size_t n)
{
    assert(NULL != memdptr);

    const uint16_t offset   = ((uint16_t *)memdptr)[-1];
    const void *   memstart = (uintptr_t)memdptr - offset;

    void *ptr = NULL;

    if (NULL == caligned_malloc(&ptr, align_size, n))
        return ptr;

    DDPTR(memdptr) = memcpy(ptr, DDPTR(memdptr), n);
    free(memstart);
    return ptr;
}

warn_unused local_inline void *
cache_malloc(void *memdptr, size_t n)
{
    void *ptr = NULL;

    assert(size != 0);
    assert(check_safe_if_safe_add(n, NGROUP));

    n = PLUSNGRP(n);
    ptr = caligned_malloc_set(n, NGROUP, EMPTY);
    if (ptr)
        DDPTR(memdptr) = ptr;
    return ptr;
}

warn_unused local_inline void *
cache_realloc(void *memdptr, size_t n)
{
    void *ptr = NULL;

    assert(check_if_safe_add(n, NGROUP));
    n = PLUSNGRP(n);
    if (caligned_realloc(&ptr, NGROUP, n))
    {
        DDPTR(memdptr) = ptr;
        return memset(ptr+LASTGRP(n), 0, n);
    }
    return ptr;
}

local_inline void cache_free(void *memptr)
{
    return caligned_free(memptr);
}

warn_unused local_inline void *
_malloc(void *memdptr, size_t size)
{
    void *ptr = NULL;

    assert(size != 0);

#ifndef NDEBUG
    ptr = calloc(1, size);
#else
    ptr = malloc(size);
#endif

    DDPTR(memdptr) = ptr;
    return ptr;
}

warn_unused local_inline void *
_realloc(void *memdptr, size_t size)
{
    return realloc(DDPTR(memdptr), NGROUP, size);
}

local_inline void _free(void *ptr)
{
    return free(ptr);
}

local_inline size_t next_power_of_two(size_t n)
{
    assert(n != 0);

    return (
#if (SIZE_MAX > 0xffffffffU)
            1ULL << (64 - BSF(n))
#else
            1U << (32 - BSF(n))
#endif
           );
}

local_inline size_t prev_power_of_two(size_t n)
{
    assert(n != 0);

    return (
#if (SIZE_MAX > 0xffffffffU)
            1ULL << (63 - BSF(n))
#else
            1U << (31 - BSF(n))
#endif
           );
}

local_inline size_t
get_size_no_resize_trigger(const size_t size,
                           const float  lf)
{
    return next_power_of_two(size / lf);
}

local_inline size_t
try_size_requirement(const size_t size,
                     const size_t max_object_size,
                     const float  lf)
{
    assert(size != 0);

    size_t try_size = get_size_no_resize_trigger(size, lf);

    if (check_if_safe_mul(try_size, max_object_size, (size_t)0))
    {
        // size would overflow
#if USE_EXACT_SIZE
        return 0;
#else
        return new_power_of_two(size); // object may trigger an early resize
#endif
    }
    return try_size;
}

local_inline size_t
advice_size_requirement(const size_t size, const float lf)
{
    if (0 == size)
        return 0;
    return get_size_noresize_trigger(size, lf);
}

local_inline int
key_generic_compare(const khpair_t it,
                    const Type    *key,
                    const hash_t   hash)
{
    int cmp;

    if (it->hash != hash)
        return 0;
    if (it->key == key)
        return 1;

    Py_INCREF(it->key);
    cmp = PyObject_RichCompareBool(it->key, key, Py_EQ);
    Py_DECREF(it->key);

    return cmp;
}

local_inline void
dict_set(Dict *dict,
         void *restrict che,
         void *restrict val,
         void *restrict kh,
         size_t cap,
         size_t size,
         float lf,
         uint8_t flag)
{
    assert(NULL != dict);

    dict->entries.kh     = kh;
    dict->entries.values = val;
    dict->entries.cache  = che;
    dict->flag           = flag;
    dict_set_new_capacity_(dict, cap, size, lf);
}

local_inline void
dict_set_new_capacity_(Dict *dict,
                      size_t cap,
                      size_t size,
                      float   lf)
{
    dict->capacity       = cap;
    dict->group_capacity = ALIGN(cap / NGROUP);
    dict->max_size       = (lf * cap) + 0.5;
    dict->used_size      = size;
    dict->lf             = lf;
}

local_inline void dict_unset(Dict **dict)
{
    assert(NULL != dict);
    *dict = empty_dict;
}

local_inline void
dict_set_alias(const Dict * restrict dict, Dict * restrict alias)
{
    assert(NULL != dict);

    dict_set(alias,
             dict->entries.kh, dict->entries.cache,
             dict->entries.values, dict->capacity,
             dict->used_size, dict->lf, dict->flags);
}

locale_inline int
dict_update_key_in_entry(Dict *dict,
                         Type *restrict key,
                         Type *restrict value,
                         size_t j)
{
    Type *tmp = dict->entries.values[j];

    dict->entries.values[j] = value;
    Py_XDECREF(tmp);
    Py_XDECREF(key); // key already exist

    return 0;
}

locale_inline size_t
dict_add_entry(Dict *dict,
               Type *restrict key,
               Type *restrict value,
               hash_t  hash,
               size_t tag,
               size_t j)
{
    entry_t entries = dict->entries;

    assert(j * NGROUP <= dict->capacity);
    assert(NULL != entries.values[j]);
    assert(NULL != entries.kh[j].key);

    entries.cache[j]   = tag;
    entries.values[j]  = value;
    entries.kh[j].key  = key;
    entries.kh[j].hash = hash;

    inc_entry_size(dict);
    return 0;
}

local_inline const size_t dict_size(const Dict *dict)
{
    assert(NULL != dict);
    return dict->used_size;
}

local_inline const size_t dict_capacity(const Dict *dict)
{
    assert(NULL != dict);
    return dict->capacity;
}

local_inline int dict_isempty(const Dict *dict)
{
    assert(NULL != dict);
    return dict->used_size == 0;
}

local_inline int dict_isunused(const Dict *dict)
{
    assert(NULL != dict);
    return dict->capacity == 0;
}

local_inline const float
dict_load_factor(const Dict *dict)
{
    return dict->lf;
}

local_inline int
dict_explicit_set_load_factor(Dict *dict, float lf)
{
    assert(NULL != dict);

    if (out_of_range_lf(lf))
        return -1;

    dict->lf = lf;

    /* if old_lf is greater than new_lf and size does not
        meet the new_lf criteria, a rehash must occur on
        the next call to a dict mutating function. */
    dict->max_size = dict->capacity * lf;
    return 0;
}

local_inline visit_t new_visit_struct(Dict *dict)
{
    assert(NULL != dict);

    visit_t v = {
        .i    = 0,
        .size = dict->group_capacity,
        .grp  = dict->entries.cache,
        .kh   = dict->entries.kh,
        .val  = dict->entries.values 
    };
    return v;
}

local_inline visit_t new_visit_struct_from(Dict *dict, ssize_t i)
{
    assert(NULL != dict);
    assert(i < dict->capacity);

    // TODO: set correct values
    visit_t v = {
        .i    = 0,
        .size = dict->group_capacity,
        .grp  = dict->entries.cache,
        .kh   = dict->entries.kh,
        .val  = dict->entries.values 
    };
    return v;
}

/**
 * @dict_for_each_*: visit every set entry in a dict and do things within the loop body just like a `for` block. Typically, they perform `for_each_set_entries_in_groups: do {...}`
 *
 * Warning: The @dict_for_each* set of macros are defined for convenience and are not intended for any use beyond this file. For iteration, use @dict_iter* functions.
 */

/**
 * @dict_for_each_call_method: The generic call macro  with default values/functions needed for the rest of the `dict_for_each*` macros
 * @parameters: @meth (the macro to call); @v (see @visit_t); non-default arguments are passed as vargs and handled by the called  macro.
 * The @___*___ names are variables to be defined in the loop; their names are what you get by removing the underscores and qpy. The default values it passes are described below:
 * @v
 * @j: a variable to store indices during visitation
 * @mask: a variable to store mask generated by @cmp
 * @mm_test* (or @cmp: generate mask for nonempty entries in a group
 * @next_group (does current_group_index += NGROUP)
 * @p: probe index
 * @k: probe counter
 * @g: save initial group index before visit (probe)
 * @dict_slot: returns the probed slot index
 * @probe_next_dict_slot
 */
#define dict_for_each_call_method(mth, v, ...)          \
    mth((v),                                            \
        ___qpyj___,   ___qpymask___, mm_test_has_entry, \
        mm_scan_mask, NEXT_GROUP,  ___qpyp___,          \
        ___qpyk___,   ___qpyg___,  dict_slot,           \
        probe_next_dict_slot,      __VA_ARGS__          \
       )

/**
 * @dict_for_each_set_ent: visit every set entry in the mask generated by @cmp.
 * Note: it is only used within @dict_for_each and @dict_for_each_mask block
 */
#define dict_for_each_set_ent_(v, j, m, cmp, scan, ...)         \
    for (mask_t m=(v.mm=mm_load(v.grp+v.i), cmp(v.mm)); (m) AND (j=scan(m)+v.i); m &= m - 1)

/**
 * @dict_for_each_cmp_set_ent: main block for @dict_for_wach_cmp: compares each group with an @mmx group and generates a mask; visit every set entry in each generated mask.
 *  Note: it is only used under its top block @dict_for_each_cmp.
 */
#define dict_cmp_for_each_set_ent_(v, j, m, _c, scan, _t, _p, _k, _g, _s, _n, cmp2, xmm, ...) \
    for (mask_t m=(v.mm=mm_load(v.grp+v.i), cmp2(v.mm, xmm)); (m) AND (j=scan(m)+v.i); m &= m - 1)

/** @dict_for_each: visit every set entry in dict */
#define dict_for_each_(v, j, m, cmp, scan, next, ...)   \
    for (size_t j=0; (v.i < v.size); next(v.i)) \
        dict_for_each_set_ent_(v, j, m, cmp, scan)

/**
 * @dict_for_each_mask: visit every set entry in dict
 * mask is handled externally and must be set to zero or call @dict_for_each_next_mask before it can visit the next group else it loops infinitely
 */
#define dict_for_each_mask_(v, j, m, cmp, _s, next, ...) \
    for (size_t j=0; (v.i < v.size); next(v.i))\
        for (mask_t m=(v.mm=mm_load(v.grp+v.i), cmp(v.mm)); m;)

/**
 * @dict_for_each_cmp: the top block for @dict_for_each_cmp_set_ent; visits entries by probing.
 * It can be called independent of its main block though (an infinte loop trap)
 */ 
#define dict_for_each_probe(v, j, m, _c, _s, _n, p, k, g, slot, next_slot, ...) \
    for (size_t p=0, k=0, j=0, g=v.i; (1); v.i=slot(p, g, v.size), next_slot(p, k))

/* The below macros are defined for convenient call to the above macros */
#define dict_for_each_probe(v) dict_for_each_call_meth(dict_for_each_, v)
#define dict_cmp_for_each_set_ent(v, x) dict_for_each_call_meth(dict_for_each_cmp_set_ent_, v, mm_test_equal, x)
#define dict_for_each_mask(v) dict_for_each_call_meth(dict_for_each_mask_, v)
#define dict_for_each(v) dict_for_each_call_meth(dict_for_each_, v)
#define dict_for_each_set_ent(v) dict_for_each_call_meth(dict_for_each_set_ent_, v)
#define dict_for_each_next_mask()  (___qpymask___=0)

/* The below macros are to access common properties within the dict_for_each* block through the visit struct */
#define dict_vst_get_group_index(v) ((v).i)
#define dict_vst_get_mm_group(v)    ((v).mm)
#define dict_vst_get_index(v) (___qpyj___)
#define dict_vst_get_mask(v)  (___qpymask___)
#define dict_vst_get_kh(v)    ((v).kh[__qpyj__])
#define dict_vst_get_key(v)   ((v).kh[__qpyj__].key)
#define dict_vst_get_hash(v)  ((v).kh[__qpyj__].hash)
#define dict_vst_get_value(v) ((v).val[__qpyj__])

local warn_unused int
dict_for_each_do(Dict *dict,
                  const void    *arg,
                  int  (do_func *)(const Type *key,
                                   const Type *val,
                                   void *arg))
{
    assert(NULL != dict);

    visit_t v = new_visit_struct(dict);

    dict_for_each(v)
    {
        if (do_func(dict_vst_get_key(v), dict_vst_get_value(v), arg) < 0)
            return -1;
    }
    return 0;
}

local_inline ssize_t
dict_count_only_from(Dict *dict, ssize_t i)
{
    visit_t v = new_visit_struct_from(dict, i);
    ssize_t j = 0;
    
    dict_for_each_mask(v)
    {
        j += POPCNT(dict_vst_get_mask(v));
        dict_for_each_next_mask(v);
    }
    return j;
}


local_inline int dict_malloc_ckhv(void restrict **c,
                                  void restrict **v,
                                  void restrict **kh)
{
    if (NOT cache_malloc(c, n))
        return -1;
    if (_malloc(&v, n * sizeof(Type *)) AND
        _malloc(&kh, n * sizeof(khpair_t)))
        return 0;
    dict_free_ckhv(*c, *v, *kh);
#   define dict_malloc_ckhv(c, v, kh) dict_malloc_ckhv(void*)c, (void*)v, (void*)kh)
    return -1;
}

local_inline void dict_free_ckhv(void * restrict c,
                                 void * restrict v,
                                 void * restrict kh)
{
    return calloc_free(c), _free(v), _free(kh);
}

local_inline void dict_free_ckhv_in_entry(Dict *dict)
{
    assert(NULL dict);
    entry_t ent = dict->entries;
    return dict_free_ckhv(ent.cache, ent.values, ent.kh);
}

local int
dict_malloc_int(Dict **dict,
                size_t n,
                float  lf)
{
    void *kh, *v, *c;
    
    assert(NULL != dict AND NULL != *dict);

    if (out_of_range_lf(lf))
        return -1;
    if (0 == n)
        return dict_unset(dict);

    n = try_size_requirement(n, sizeof(khpair_t), lf);
    if (0 == n)
        return -1;
    if (dict_malloc_ckhv(&c, &v, &kh))
        return -1;
    dict_set(*dict, c, v, kh, n, 0, lf);

    return n;
}

local void *dict_remove(Dict **dict)
{
    assert(NULL != dict && NULL != *dict);

    Dict alias = {0};
#ifdef NO_PyAPI
    clearfunc_t clear = (*dict)->clear;
#endif

    dict_alias_copy(*dict, &alias);
    if (dict_owned_memory(*dict)) // TODO
        _free(*dict);
    dict_unset(dict);

#ifdef NO_PyAPI
    if (NULL != clear) dict_for_each_do(alias, NULL, clear);
#else
    visit_t v = new_visit_struct(&alias);
    dict_for_each(v)
    {
        Py_DECREF(dict_vst_get_key(v));
        Py_DECREF(dict_vst_get_value(v));
    }
#endif
    dict_free_ckhv_in_entry(alias);
#   define dict_remove(d) dict_remove(&d)
    return NULL;
}

warn_unused local void *
dict_copy(Dict * restrict dest,
          Dict * restrict src)
{
    assert(NULL == src);
    if (dict_isempty(src))
        return NULL;
#   if NO_PyAPI
    if (NULL == dest)
    {
        if (NULL == (dest=_malloc(sizeof *src)))
            return dest;
        dict_set_owned_memory(dest); // TODO
    }
#   endif
    
    if (dict_malloc_int(dest, dict_size(src), dict_load_factor(src)) < 0)
        return NULL;

    visit_t v = new_visit_struct(src);
    dict_for_each(v)
    {
        khpair_t *kh  = dict_vst_get_kh(v);
        Type *    val = dict_vst_get_value(v);
        
        if (lookup_insert_nodeleted(d, kh.key, val, kh.hash))
            return dict_remove(dest);
    }
    dict_set_size(dest, dict_size(src));
    return dest;
}

warn_unused local void *
dict_ncopy(Dict * restrict dest,
           Dict * restrict src,
           size_t n)
{
    assert(NULL == src);
    if (dict_isempty(src))
        return NULL;
#   if NO_PyAPI
    if (NULL == dest)
    {
        if (NULL == (dest=_malloc(sizeof *src)))
            return dest;
        dict_set_owned_memory(dest);
    }
#   endif

    if (dict_malloc_int(dest, n, dict_load_factor(src)) < 0)
        return NULL;

    size_t  i = 0; // count copied entries
    visit_t v = new_visit_struct(dict);

    dict_for_each(v)
    {
        khpair_t kh  = dict_vst_get_kh(v);
        Type *   val = dict_vst_get_value(v);

        if (lookup_insert_nodeleted(d, kh.key, val, kh.hash))
            return dict_remove(dest);
        if (++i < n)
            goto exit;
    }
 exit:
    dict_set_size(dest, i);
    return dest;
}

local void *dict_clone(Dict *dict)
{
    assert(NULL != dict);
    
    if (dict_isempty(dict))
        return empty_dict;
    // TODO
    return NULL;
}

warn_unused local void *
dict_merge(Dict *d1,
           Dict *d2,
           Dict **dest)
{
    assert(NULL != d1 && NULL != d2);

    if (NOT dict_isempty(d1) AND dict_isempty(d2))
        return dict_copy(*dest, d1);
    if (dict_isempty(d1) AND NOT dict_isempty(d2))
        return dict_copy(*dest, d1);

    if (d1 == d2)
        return dict_copy(*dest, d1);
    // TODO:
    return NULL;
}

warn_unused local void *
dict_merge_nocopy(Dict *dict,
                  Dict *d1,
                  Dict **dest)
{
    assert(NULL != d1 && NULL != d2);

    if (NOT dict_isempty(d1) AND dict_isempty(d2))
        return dict_copy(*dest, d1);
    if (dict_isempty(d1) AND NOT dict_isempty(d2))
        return dict_copy(*dest, d1);
 
    if (d1 == d2)
        return d1;
    // TODO
    return NULL;
}

local int
dict_resize(Dict *dict, size_t n)
{
    assert(NULL != dict);

    // a size of zero is the same as remove
    if (NOT n)
        return dict_remove(dict);
    // we are rich! Do nothing
    if (n <= dict->max_size)
        return 0;
    return dict_rehash(dict, nsize);
}

local_inline int
dict_rehash(Dict *dict, size_t n)
{
    Dict d = {0};

    assert(0 != dict);
    if (0 == n)
        return dict_remove(dict);

    if (n < dict->used_size)
        LOG(stderr, "resize of `dict(%lu) at address %p` to `dict(%lu)` will result to loss of entries", LONG(dict->capacity), dict, LONG(n));

    if ((n < dict->max_size ? dict_ncopy(&d, dict, n) : dict_copy(&d, dict)) < 0)
        return ret;

    // remove dict
    dict_remove(dict);

    // copy to old dict
#ifndef NO_PyAPI
    dict_set_alias(&d, dict);
#else
    *dict = d;
#endif
    return 0;
}

//  const size_t group_idx = find_group_from_hash(hash, dict->capacity);

local ssize_t
lookup_insert_generic_nodeleted(Dict *dict,
                                Type *restrict key,
                                Type *restrict value,
                                const hash_t   hash)
{
    assert(NULL != dict);
    assert(NULL != key);
 
    const uint8_t tag = ctag(hash);
    const mm_t    dup = mm_duplicate(tag);
    visit_t v = {0}; // TODO

    dict_for_each_probe(v)
    {
        dict_cmp_for_each_set_ent(v, dup)
        {
            khpair_t kh = dict_vst_get_kh(v);
            int cmp = key_generic_compare(kh, key, hash);
            if (UNLIKELY(cmp < 0))
                return -1;
            if (cmp)
                return dict_update_key_in_entry(dict, key, value, dict_vst_get_index());
        }
        
        mask_t mask = mm_test_empty(dict_vst_get_mm_group(v));
        if (LIKELY(mask))
            return dict_add_entry(dict, key, value, hash, tag, mm_scan_mask(mask));
    }
    UNREACHABLE();
}

locale_inline ssize_t
lookup_insert_generic(Dict *dict,
                      Type *restrict key,
                      Type *restrict value,
                      const hash_t   hash)
{
    assert(NULL != dict);
    assert(NULL != key);

    bool    t = true; // true if k is unset
    size_t  k = 0;
    visit_t v = {0};
    const uint8_t tag = ctag(hash);
    const mm_t    dup = mm_duplicate(tag);

    dict_for_each_probe(v)
    {
        dict_cmp_for_each_set_ent(v, dup)
        {
            khpair_t kh = dict_vst_get_kh(v);
            int cmp = key_generic_compare(kh, key, hash);
            if (UNLIKELY(cmp < 0))
                return -1;
            if (cmp)
                return dict_update_key_in_entry(dict, key, value, dict_vst_get_index(v));
        }

        mask_t UNUSED(m) = 0;
        if ((t) AND (m=mm_find_empty_slot(dict_vst_get_mm_group(v))))
            k=dict_vst_get_group_index(v)+mm_scan_mask(m), t=false;

        if (mm_test_empty_fast(dict_vst_get_mm_group(v)))
        {
            if (NOT (t))
                return dict_add_entry(dict, key, value, hash, tag, k);
            return -1;
        }
    }
    UNREACHABLE();
}

locale_inline ssize_t
lookup_generic(Dict *dict, Type *key, hash_t hash)
{
    assert(NULL != dict);
    assert(NULL != key);

    const mm_t dup = mm_duplicate(ctag(hash));
    visit_t v = {0};

    dict_for_each_probe(v)
    {
        dict_cmp_for_each_set_ent(v, dup)
        {
            khpair_t kh = dict_vst_get_kh(v);
            int cmp = key_generic_compare(kh, key, hash);
            if (UNLIKELY(cmp < 0))
                return -1;
            if (cmp)
                return dict_vst_get_index(v);
        }
        if (mm_test_empty_fast(dict_vst_get_mm_group(v)))
            return -1;
    }
    UNREACHABLE();
}

#if NO_PyAPI
#    define dict_copy(d) dict_copy(NULL, d)
#    define dict_copy_to(dest, d) dict_copy(dest, d) 
#    define dict_ncopy(d) dict_ncopy(NULL, d)
#    define dict_ncopy_to(dest, d) dict_ncopy(dest, d) 
#endif

#undef PTR
#undef DPTR
#undef SHPTR
#undef LONG
#undef DCR
#undef ICR
#undef PLUSNGROUP
#undef LASTGRP
#undef NEXT_GROUP

#undef dict_
#undef Dict
#undef out_of_range_lf
#undef inc_entry_size
#undef dict_slot
#undef probe_next_dict_slot

#undef dict_for_each_call_method
#undef dict_for_each_set_ent_
#undef dict_cmp_for_each_set_ent_
#undef dict_for_each_
#undef dict_for_each_mask_
#undef dict_for_each_probe
#undef dict_cmp_for_each_set_ent
#undef dict_for_each_mask
#undef dict_for_each
#undef dict_for_each_set_ent
#undef dict_vst_get_group_index
#undef dict_vst_get_mm_group
#undef dict_vst_get_index
#undef dict_vst_get_mask
#undef dict_vst_next_mask
#undef dict_vst_get_kh
#undef dict_vst_get_key
#undef dict_vst_get_hash
#undef dict_vst_get_value
#else // QPy_MM_UNSUPPORTED
#error
#endif
