#ifndef QPy_DICT_H
#define QPy_DICT_H
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "include/types.h"
#include "include/visit.h"
#include "include/defs.h"
#include "include/arch/mm.h"

#if defined(MM_SUPPORT) AND (MM_SUPPORT != 0)
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

// GENERAL TODO: check_if_size_add_overflow, NGROUP_MAX
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

local_inline   const size_t
size_no_resize(const size_t size,
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

    size_t mx = size_no_resize(size, lf);

    if (check_if_safe_mul(mx, max_object_size, (size_t)0))
        return mx;
    return 0;
}

local_inline size_t
advice_size_requirement(const size_t size, const float lf)
{
    if (0 == size)
        return 0;
    return size_no_resize(size, lf);
}

local cache_t const dict_Local_null_group[NGROUP_MAX] = {
#   define y8(y) y, y, y, y, y, y, y, y
    y8(DICT_EMPTY), y8(DICT_EMPTY),
    y8(DICT_EMPTY), y8(DICT_EMPTY)
#   undef y8
};

local khpair_t const dict_Local_null_key[2] = {0};
local Type     const dict_Local_null_val[2] = {0};

static Dict const dict_Local_null_Dict = {
    .entries.cache   = &dict_Local_null_group,
    .entries.key     = dict_Local_null_key + 1, 
    .entries.values  = dict_Local_null_val + 1
};

local mm_t dict_Local_null_set;
local mm_t dict_Local_del_set;

local PyObject *dict_Local_Py_addnote = NULL;
local PyObject *dict_Local_Py_key     = NULL;

local_inline void
dict_set_Local(void)
{
    dict_Local_null_set = mm_set_null();
    dict_Local_del_set  = mm_set_del();

    // make read-only
#   define dict_Local_null_set (true, dict_Local_null_set)
#   define dict_Local_del_set  (true, dict_Local_del_set)

#ifndef NO_PyAPI
    dict_Local_Py_addnote = PyUnicode_FromString("add_note");
    dict_Local_Py_key     = PyUnicode_FromString("keys");

    // make read-only
#   define dict_Local_Py_addnote  (true, dict_Local_Py_addnote)
#   define dict_Local_Py_key      (true, dict_Local_Py_key)
#endif
}

#ifdef MM_ZERO
local_inline const uint8_t get_tag(const uint64_t v)
{
    const uint16_t x = v & 0xff;

    return (x - (x * 0x2041u >> 20) * 0x7f) + 1;
}
#else
#   define get_tag(v) ((v) & 0xff)
#endif

const local_inline mm_t dup_tag(const uint8_t tag)
{
    return mm_dup(tag);
}

local_inline mask_t load_group(const void *v)
{
    return mm_mask_null(v, dict_Local_null_set);
}

local_inline mask_t
cmp_group(const mm_t group, const mm_t mask)
{
    return mm_mask(group, mask);
}

local_inline mask_t cmp_null(const mm_t group)
{
    return mm_mask_null(group, dict_Local_null_set);
}

const local_inline bool has_null(const mm_t group)
{
    return mm_null_fast(group, dict_Local_null_set);
}

local_inline mask_t cmp_full(const mm_t group)
{
    return mm_mask_full(group, dict_Local_del_set, dict_Local_null_set);
}

const local_inline uint8_t bsr(const mask_t mask)
{
    return mm_scan(mask);
}

local_inline const size_t dict_size(const Dict *dict)
{
    assert(NULL != dict);
    return dict->used_size;
}

local_inline const size_t dict_maxsize(const Dict *dict)
{
    assert(NULL != dict);
    return dict->max_size;
}

local_inline const size_t dict_capacity(const Dict *dict)
{
    assert(NULL != dict);
    return dict->capacity;
}

local_inline const void * dict_khpairs(const Dict *dict)
{
    assert(NULL != dict);
    return dict->entries.kh;
}

local_inline const void * dict_cache(const Dict *dict)
{
    assert(NULL != dict);
    return dict->entries.cache;
}

local_inline const void * dict_values(const Dict *dict)
{
    assert(NULL != dict);
    return dict->entries.values;
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
dict_load_fact(const Dict *dict)
{
    assert(NULL != dict);
    return (1. * dict->max_size) / dict->capacity + 5e-4; // TODO: double overflow
}

local_inline int
dict_set_load_fact(Dict *dict, const float lf)
{
    assert(NULL != dict);

    if (out_of_range_lf(lf))
        return -1;

    dict->max_size = dict->capacity * lf;

    return 0;
}

local_inline void
dict_set_size(Dict *dict, const size_t n)
{
    assert(NULL != dict);

    dict->used_size = n | 0;
}

local_inline void dict_set_owned_memory(Dict *dict)
{
    assert(NULL != dict);

    dict->flags |= 0x80;
}

local_inline void
dict_setcap(Dict *dict, size_t cap, size_t size, float lf)
{
    dict->capacity       = cap;
    dict->group_capacity = ALIGN(cap / NGROUP, NGROUP);
    dict->max_size       = lf * cap;
    dict->used_size      = size;
}

local_inline const int dict_owned_memory(const Dict *dict)
{
    assert(NULL != dict);

    return dict->flags & 0x80;
}

#define DKTO_PTR(p, d) (((p) AND (d) AND (DPTR(d)=(p))), (p))

warn_unused local_inline void *
dict_malloc(void *dp, size_t n)
{
    void *ptr = NULL;

    assert(0 != n);

#if defined(NDEBUG) AND NOT defined(NO_PyAPI)
    ptr = PyMem_Malloc(n);

#  elif NOT defined(NO_PyAPI)
    ptr = PyMem_Calloc(n, sizeof(char));

#  elif defined(NDEBUG)
    ptr = malloc(n);
// NDEBUG AND NO_PyAPI
#else
    ptr = calloc(n, sizeof(char));
#endif
    return DKTO_PTR(ptr, dp);
}

warn_unused local_inline void *
dict_realloc(void *dp, size_t n)
{
    void *ptr = NULL;

    assert(NULL != dp AND 0 != n);

#  ifndef NO_PyAPI
    ptr = PyMem_Realloc(DPTR(dp), n);
#  else
    ptr = realloc(DPTR(dp), n);
#  endif
    return DKTO_PTR(ptr, dp);
}

local_inline void dict_free(void *ptr)
{
#  ifndef NO_PyAPI
    PyMem_Free(ptr);
#  else
    free(ptr);
#  endif
}

#define INTPTR(x)    (uintptr_t)(x)
#define ALIGNP(x, k) (void *)ALIGNU(x, k)

warn_unused local void *
dict_aligned_malloc(void *dp,
                    const size_t n, // size
                    const short  k) // align size
{
    assert(0 == (k & (k - 1))); // must be a power of two 

    void *ptr = NULL;
    short mx  = (k - 1) + sizeof mx; // max offset size

    if (check_if_size_add_overflow(n, mx))
        return NULL;
    if (NOT dict_malloc(&ptr, n + mx))
        return ptr;

    short *kp = ALIGNP(INTPTR(ptr) + sizeof mx, k);

    kp[-1] = INTPTR(kp) - INTPTR(ptr);

    return DKTO_PTR(kp, dp);
}

local void dict_aligned_free(void *ptr)
{
    if (ptr == NULL)
        return;
    const short mx = ((short *)ptr)[-1];

    return free((void *)(INTPTR(mx) - mx));
}
#undef INTPTR
#undef ALIGNP

local_inline void *
dict_aligned_calloc(const size_t n,
                    const short  k)
{
    void *ptr = dict_aligned_malloc(NULL, n, k);

    if (NULL != ptr)
        return memset(ptr, '\0', n);

    return ptr;
}

warn_unused local_inline void *
dict_memset_alloc(const size_t n,
                  const short  k,
                  const int    c)
{
    void *ptr = dict_aligned_malloc(NULL, n, k);

    if (NULL != ptr)
        return memset(ptr, c, n);

    return ptr;
}

warn_unused local_inline void *
dict_cmalloc(void *dp, const size_t n)
{
    assert(0 != n);

    if (check_if_size_add_overflow(n, NGROUP))
        return NULL;

    void *ptr = dict_aligned_mset(n+NGROUP, NGROUP, DICT_EMPTY);

    return DKTO_PTR(ptr, dp);
}

local_inline void dict_cfree(void *ptr)
{
    return dict_aligned_free(ptr);
}

warn_unused local_inline void *
dict_malloc_self_(void *UNUSED(type), size_t UNUSED(n))
{
    Dict *self = NULL;

#  ifndef NO_PyAPI
    assert(NULL != type);
    self = (Dict *)(Py_TYPE(type)->tp_alloc(type, 0));
#  else
    self = dict_malloc(NULL, sizeof(Dict));
#  endif

    if (NULL != self)
        dict_set_owned_memory(self);
    return self;
}

local_inline void *dict_free_self_(void *dp)
{
    if (NULL == dp)
        return dp;
    Dict *self = *(Dict **)dp;
    if (NULL == self OR NOT dict_owned_memory(self))
        return NULL;

#  ifndef NO_PyAPI
    PyTypeObject *tp = Py_TYPE(tp);
    if (tp->tp_flags & Py_TPFLAGS_HAVE_GC)
        PyObject_GC_UnTrack(tp);
    tp->tp_free(self);
#  else
    dict_free(self);
#  endif
    return NULL;
}

local_inline void dict_free_ckhv(void * restrict c,
                                 void * restrict v,
                                 void * restrict kh)
{
    dict_cfree(c);
    if (NULL != v)
        dict_free((Type *)(v) - 1);
    if (NULL != kh)
        dict_free((khpair_t *)(kh) - 1);
}

local_inline int dict_malloc_ckhv(cache_t  **c,
                                  Type     **v,
                                  khpair_t **kh,
                                  size_t     n)
{
    if (NOT dict_cmalloc(c, n))
        return -1;
    if (NOT dict_malloc(*v, n * sizeof(Type )) OR
        NOT dict_malloc(kh, n * sizeof(khpair_t)))
    {
        dict_cfree(c), dict_free(v), dict_free(kh);
        return -1;
    }
    // store dummy key and value before entries so that (key|value)[-1] returns it (in the case of error) 
    **v  = dict_Local_null_val; // TODO
    **kh = dict_Local_null_key;
    // move ahead of dummy key and value
    *v  += 1;
    *kh += 1;

    return 0;
}

#define DICT_ENTRYRY(d) (&((d)->entries))

local_inline void dict_free_ckhv_in_entry(Dict *dict)
{
    assert(NULL != dict);
    if (0 != dict_owned_memory(dict))
        return;
    entry_t *e = DICT_ENTRYRY(dict);

    return dict_free_ckhv(e->cache, e->values, e->kh);
}

local_inline int dict_unset(Dict **dict)
{
    assert(NULL != dict);
    *dict = (Dict *)dict_struct_empty();
    return 0;
}

local int
dict_set(Dict **dict,
                size_t n,
                float  lf)
{
    assert(NULL != dict AND NULL != *dict);
    assert(0 != dict_owned_memory(*dict));

    entry_t *e = DICT_ENTRYRY(*dict);
    void   *kh = &(e->kh), *v = &(e->values), *c = &(e->cache);

    if (out_of_range_lf(lf))
        return -1;
    if (0 == n)
        return dict_unset(dict);

    n = try_size_requirement(n, sizeof(khpair_t), lf);
    if (0 == n)
        return -1;
    if (NOT dict_malloc_ckhv(c, v, kh, n))
        return -1;
    dict_setcap(*dict, n, 0, lf);

    return 0;
}

local warn_unused int
dict_map(Dict *dict,
         void *arg,
         int  (* do_fn)(const Type key,
                        const Type val,
                        void *arg))
{
    assert(NULL != dict);

    const Type    khp = dict_khpairs(dict);
    const Type    vp  = dict_values(dict);
    const cache_t grp = dict_cache(dict);
    
    const size_t n = dict_capacity(dict);

    for (size_t j, i=0; i < n; i+=DICT_N_GROUP)
        for (mask_t m = mask_t m = cmp_full(load_group(grp + i)); m; m &= m - 1)
        {
            j = i + bsr(m);
            if (do_fn(khp[j].key, vp[j], arg))
                return -1;
        }
    return 0;
}

local_inline size_t
dict_count_used(Dict *dict, size_t n)
{
    assert(n > dict_capacity(dict));
    if (0 == n)
        return dict_size(dict);

    size_t k = 0, j = n; // TODO: align (n)

    for (size_t i=0; i < j; i+=DICT_N_GROUP)
    {
        mask_t m  = cmp_full(load_group(grp + i));
        if (m) k += popcnt(m); // TODO
    }
    return k;
}

warn_unused local void *
dict_new(void **type, ssize_t n, float lf)
{
    Dict *dict = NULL;
    static bool set = true;

    if (true == set)
        dict_set_Local();
    set = false; // set once

    if (NOT n)
        return &dict_Local_null_Dict;

    dict = dict_malloc_self_(type, 0);
    if (NULL == dict)
         return dict;
    if (dict_set(&dict, n, lf) != 0)
        return dict_free_self_(dict);
    return dict;
}


local_inline void dict_Py_release_kv_ref(Dict *dict)
{
    const cache_t grp = dict_cache(dict);
    const size_t  n   = dict_capacity(dict);
    
    for (size_t j, i=0; i < n; i+=DICT_N_GROUP)
        for (mask_t m = mask_t m = cmp_full(load_group(grp + i)); m; m &= m - 1)
        {
            j = i + bsr(m);
            khpair_t k = DICT_ENTRYRY(dict)->kh[j].key;
            Type     v = DICT_ENTRYRY(dict)->values[j];;

            Py_DECREF(k);
            Py_DECREF(v);
        }
}

local void *dict_remove(Dict **dict)
{
    assert(NULL != dict && NULL != *dict);
    assert(0 != dict_owned_memory(*dict));

    Dict *d = *dict;

# ifdef NO_PyAPI
    if (NULL != d->clear)
        dict_map(*dict, NULL, d->clear);
# else
    dict_Py_release_kv_ref(dict);
# endif
    dict_free_ckhv_in_entry(d);
    dict_free_self_(d);
    dict_unset(dict);

    return NULL;
#   define dict_remove(d) dict_remove(&d)
}

local_inline void *
dict_copy_insert_n_(Dict * restrict dest,
                    const Dict * restrict src,
                    const size_t n)
{
    const Type    khp = dict_khpairs(dict);
    const Type    vp  = dict_values(dict);
    const cache_t grp = dict_cache(dict);

    for (size_t j, k=n, i=0; i < n; i+=DICT_N_GROUP) // TODO: align(n)
        for (mask_t m = mask_t m = cmp_full(load_group(grp + i)); m; m &= m - 1)
        {
            j = i + bsr(m);
            khpair_t kh = khp[j];
            Type     v  = vp[j];

            if (dict_insert(dest, kh.key, v, kh.hash))
                return NULL;
            if (NOT --k)
                goto ret; 
        }
 ret:
    return dict_set_size(dest, n - k), dest;
}

local_inline void *
dict_copy_insert_all_(Dict * restrict dest,
                      const Dict * restrict src)
{
    const Type    khp = dict_khpairs(dict);
    const Type    vp  = dict_values(dict);
    const cache_t grp = dict_cache(dict);

    const size_t n = dict_capacity(dict);
    for (size_t j, k=n, i=0; i < n; i+=DICT_N_GROUP) // TODO: align(n)
        for (mask_t m = mask_t m = cmp_full(load_group(grp + i)); m; m &= m - 1)
        {
            j = i + bsr(m);
            khpair_t kh = khp[j];
            Type     v  = vp[j];

            if (dict_insert(dest, kh.key, v, kh.hash))
                return NULL;
        }
}

#define DICT_NEW NULL

warn_unused local_inline void *
dict_copy(Dict * restrict dest,
          Dict * restrict src)
{
    Dict *d = dest;
    bool  k = DICT_NEW == dest;

    assert(NULL != src);

    if (dict_isempty(src))
        return dict_empty_struct();
    if (true == k AND NOT(d=dict_new(dest, n, dict_load_fact(src))))
        return d;
    if (NULL == dict_copy_insert_all_(d, src))
        return (true == k AND dict_remove(d)) & 0;
    return d;
}

warn_unused local_inline void *
dict_ncopy(Dict * restrict dest,
           Dict * restrict src,
           const size_t n)
{
    Dict *d = dest;
    bool  k = DICT_NEW == dest;

    assert(NULL != src AND 0 != n);

    if (dict_isempty(src))
        return dict_empty_struct();
    if (true == k AND NOT(d=dict_new(dest, n, dict_load_fact(src))))
        return d;
    if (NULL == dict_copy_insert_n_(d, src, n))
        return (true == k AND dict_remove(d)) & 0;
    return d;
}

local void *dict_clone(Dict *dict)
{
    assert(NULL != dict);

    if (dict_isempty(dict))
        return dict_empty_struct();
    // TODO
    return NULL;
}

warn_unused local void *
dict_merge_(Dict *d1,
            Dict *d2,
            bool reuse)
{
    Dict *d = NULL;
    bool  e = dict_isempty(d1), e_ = dict_isempty(d2);

    if (NOT e AND e_)
        return dict_copy(DICT_NEW, d1);
    if (e AND NOT e_)
        return dict_copy(DICT_NEW, d2);

    const size_t n = dict_size(d1), m = dict_size(d2);
    // reuse first dict if rich
    if (true == reuse AND (dict_maxsize(d1) - n) > m)
        return dict_copy(d1, d2);

    if (check_if_size_add_overflow(n, m))
        return NULL;
    d = dict_new(dest, n+m, dict_load_fact(d1));
    if (NULL == d)
        return d;
    if (NOT dict_copy(d, d1) OR NOT dict_copy(d, d2))
        return dict_remove(d);

    return d;
}

warn_unused local_inline void *
dict_merge(Dict *d1,
           Dict *d2)
{
    assert(NULL != d1 && NULL != d2);

    if (d1 == d2)
        return dict_copy(DICT_NEW, d1);
    return dict_merge_(d1, d2, false);
}

warn_unused local_inline void *
dict_merge_nocopy(Dict *d1,
                  Dict *d2)
{
    assert(NULL != d1 && NULL != d2);

    if (d1 == d2)
        return d1;
    return dict_merge_(d1, d2, false);
}

warn_unused local_inline void *
dict_update(Dict *d1,
            Dict *d2)
{
    assert(NULL != d1 && NULL != d2);

    if (d1 == d2 OR dict_isempty(d2))
        return d1;
    return dict_merge_(d1, d2, true);
}
#undef DICT_NEW

#define DICT_VCOPY(d, n, b) (n < dict_size(d) ? dict_ncopy(b, d, n) : dict_copy(b, d)) 

local_inline int
dict_rehash(Dict **dict, size_t n)
{
    assert(NULL != dict AND NULL != *dict);

    if (0 == n)
        return dict_remove(*dict);

    Dict *d = DICT_VCOPY(*dict, n, NULL); 
    if (NULL == d)
        return -1;

    dict_remove(*dict);
    **dict = *d;
    return 0;
}
#undef DICT_VCOPY

local int
dict_resize(Dict *dict, size_t n)
{
    assert(NULL != dict);

    // a size of zero is the same as remove
    if (NOT n)
        return dict_remove(dict);
    // we are rich! Do nothing
    if (n <= dict_maxsize(dict))
        return 0;
    return dict_rehash(dict, n);
}

local_inline int
dict_swapval(Dict *dict, const Type key, Type value, size_t j)
{
    Type tmp = DICT_ENTRYRY(dict)->values[j];

    DICT_ENTRY(dict)->values[j] = value;
#   ifndef NO_PyAPI
    Py_XDECREF(tmp);
    Py_XDECREF(key); // key already exist
#   else
    if (NULL != dict->clear)
        dict->clear(key, tmp);
#   endif
    return 0;
}

local_inline size_t
dict_add(Dict *dict,
         Type restrict key,
         Type restrict value,
         hash_t  hash,
         size_t tag,
         size_t j)
{
    entry_t *e = DICT_ENTRY(dict);

    assert(NULL == e->values[j]);
    assert(NULL == e->kh[j].key);

    e->cache[j]   = tag;
    e->values[j]  = value;
    e->kh[j].key  = key;
    e->kh[j].hash = hash;

    inc_entry_size(dict);
    return 0;
}

local int dict_sentinel_cmp(Type UNUSED(v), Type UNUSED(u))
{
    LOG("%s", "cmp is not set");

    return -1;
}

#ifndef NO_PyAPI
local_inline int
dict_keycmp(const khpair_t it, const Type key, const hash_t hash)
{
    if (it.hash != hash)
        return 0;
    if (it.key == key)
        return 1;

    Py_INCREF(it.key); // keep a reference: bad `eq` may attempt to delete key
    int cmp = PyObject_RichCompareBool(it.key, key, Py_EQ);
    Py_DECREF(it.key);

    return cmp;
}
#   define dict_keycmp(d, kh, j, h) dict_keycmp(kh, j, h)
#else
#   define dict_keycmp(d, kh, j, h) dict->cmp((kh).key, k)
#endif

local_inline ssize_t
dict_insert(Dict *dict, Type key, Type val, hash_t h)
{
    assert(NULL != dict);
    assert(NULL != key);

    const Type    khp = dict_khpairs(dict); // key array
    const cache_t grp = dict_cache(dict); // cache array

    const size_t  g   = dict_capacity(dict) - 1;
    const uint8_t t   = get_tag(h); // tag
    const mm_t    txn = dup_tag(t); // tag x NGROUP

    for (size_t p=0, n=0, i=(h & g); true; i=(h + p & g))
    {
        const mm_t   v = load_group(grp + i);
        auto  mask_t m = cmp_group(v, txn); // matched tags
        for (int j; (m); m &= m - 1)
        {
            j = i + bsr(m);
            khpair_t kh = khp[j];
            int cmp = dict_keycmp(dict, kh, key, h);
            if (cmp)
            {
                if (cmp < 0)
                    return -1;
                return dict_swapval(dict, key, val, j);
            }
            m = cmp_null(v);
            if (m)
                return dict_add(dict, key, val, h, t, i + bsr(m));
            p += n; // probe next slot
            n += DICT_N_GROUP;
        }
    UNREACHABLE();
}

local_inline ssize_t
dict_insert_deleted(Dict *dict, Type key, Type val, hash_t h)
{
    assert(NULL != dict);
    assert(NULL != key);

    bool    f = true; // true if k is unset
    size_t  k = 0; // cache empty or deleted slot index

    const Type    khp = dict_khpairs(dict);
    const cache_t grp = dict_cache(dict);

    const size_t  g   = dict_capacity(dict) - 1;
    const uint8_t t   = get_tag(h);
    const mm_t    txn = dup_tag(t);

    for (size_t p=0, n=0, i=(h & g); true; i=(h + p & g))
    {
        const mm_t   v = load_group(grp + i);
        auto  mask_t m = cmp_group(v, txn);
        for (int j; (m); m &= m - 1)
        {
            j = i + bsr(m);
            khpair_t kh = khp[j];
            int cmp = dict_keycmp(dict, kh, key, h);
            if (cmp)
            {
                if (cmp < 0)
                    return -1;
                return dict_swapval(dict, key, val, j);
            }
            if (true == f AND (m=cmp_deleted(v)))
                k = i + bsr(m); f=false;
            if (has_null(v))
            {
                if (false == f)
                    return dict_add(dict, key, val, h, t, k);
                return -1;
            }
            p += n;
            n += DICT_N_GROUP;
        }
    UNREACHABLE();
}

local_inline ssize_t
dict_lookup(Dict *dict, Type key, hash_t h)
{
    assert(NULL != dict);
    assert(NULL != key);

    const Type    khp = dict_khpairs(dict);
    const cache_t grp = dict_cache(dict);

    const size_t  g   = dict_capacity(dict) - 1;
    const mm_t    txn = dup_tag(get_tag(h));
    

    for (size_t p=0, n=0, i=(h & g); true; i=(h + p & g))
    {
        const mm_t  v = load_group(grp + i); 
        for (mask_t m = cmp_group(v, txn); m; m &= m - 1)
        {
            khpair_t kh = khp[i + bsr(m)];
            int cmp = dict_keycmp(dict, kh, key, h);
            if (cmp) return cmp;
        }
        if (has_null(v))
            return -1;
        p += n; // probe next slot
        n += DICT_N_GROUP;
    }
    UNREACHABLE();
}

#if NO_PyAPI
#    define dict_copy(d) dict_copy(NULL, d)
#    define dict_copy_to(dest, d) dict_copy(dest, d)
#    define dict_ncopy(d) dict_ncopy(NULL, d)
#    define dict_ncopy_to(dest, d) dict_ncopy(dest, d)
#endif

// remove all macro definitions
#include "include/undef.h"
#undef PTR
#undef DPTR
#undef SHPTR
#undef LONG
#undef DCR
#undef ICR
#undef PLUSNGROUP
#undef LASTGRP
#undef NEXT_GROUP
#undef DICT_ENTRY

#undef dict_
#undef Dict
#undef out_of_range_lf
#undef inc_entry_size
#undef dict_slot
#undef probe_next_dict_slot
#else // QPy_MM_UNSUPPORTED
#error
#endif
#endif
