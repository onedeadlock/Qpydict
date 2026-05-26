#ifndef QPy_DICT_H
#define QPy_DICT_H
#include <stdbool.h>
#include <stdlib.h>
#include "include/types.h"
#include "include/mm.h"
#unclude "include/visit.h"
#include "include/defs.h"

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
        return NULL;

    void *ptr = malloc(size + max_offset_size);
    if (NULL == ptr)
        return ptr;
    void *kptr = ALIGNU((uintptr_t)ptr + sizeof(uint16_t), align_size);
    ((uint16_t *)kptr)[-1] = (uintptr_t)kptr - (uintptr_t)ptr; // store offset size just before the aligned memory

    DPTR(memdptr) = kptr;
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

    DPTR(memdptr) = memcpy(ptr, DPTR(memdptr), n);
    free(memstart);
    return ptr;
}

warn_unused local_inline void *
cache_malloc(void *memdptr, size_t n)
{
    void *ptr = NULL;

    assert(size != 0 AND check_if_safe_add(n, NGROUP));

    n += NGROUP;
    ptr = caligned_malloc_set(n, NGROUP, EMPTY);
    if (ptr)
        DPTR(memdptr) = ptr;
    return ptr;
}

warn_unused local_inline void *
cache_realloc(void *memdptr, size_t n)
{
    void *ptr = NULL;

    assert(size != 0 AND check_if_safe_add(n, NGROUP));

    n += NGROUP;
    if (caligned_realloc(&ptr, NGROUP, n))
    {
        DPTR(memdptr) = ptr;
        return memset(ptr+LASTGRP(n), 0, n);
    }
    return ptr;
}

local_inline void cache_free(void *memptr)
{
    return caligned_free(memptr);
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

    size_t ts = get_size_no_resize_trigger(size, lf);

    if (check_if_safe_mul(ts, max_object_size, (size_t)0))
        return ts;
    return 0;
}

local_inline size_t
advice_size_requirement(const size_t size, const float lf)
{
    if (0 == size)
        return 0;
    return get_size_noresize_trigger(size, lf);
}


local_inline const void *dict_key_empty(void)
{
    static const khpair_t kh = {NULL, 0};
    return &kh + sizeof kh; // [-1] = kh
}

local_inline const void *dict_value_empty(void)
{
    static const Type *v = NULL;
    return &v + sizeof v; // [-1] = v
}

local_inline const Dict *dict_struct_empty(void)
{
    static const Dict d = {
        .entries.cache  = LOCAL_empty_tag_full_group,
        .entries.kh     = dict_key_empty(),
        .enteies.values = dict_values_empty();
    };
    return &d;
}

local const int dict_struct_offset(void)
{
#  ifndef NO_PyAPI
    static const int i = offsetof(Dict, entries);
#  else
    static const int i = 0;
#  endif
    return i;
}

local_inline const int dict_struct_size(void)
{
#  ifndef NO_PyAPI
    static const int i = sizeof(Dict) - offsetof(Dict, entries);
#  else
    static const int i = sizeof(Dict);
#  endif
    return i;
}

local_inline void
dict_setcap(Dict *dict, size_t cap, size_t size, float lf)
{
    dict->capacity       = cap;
    dict->group_capacity = ALIGN(cap / NGROUP);
    dict->max_size       = (lf * cap);
    dict->used_size      = size;
    dict->lf             = lf;
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
dict_load_fact(const Dict *dict)
{
    return dict->lf;
}

local_inline int
dict_set_load_fact(Dict *dict, float lf)
{
    assert(NULL != dict);

    if (out_of_range_lf(lf))
        return -1;
    dict->lf = lf;
    dict->max_size = dict->capacity * lf;

    return 0;
}

local_inline void dict_set_owned_memory(const Dict *dict)
{
    assert(NULL != dict);
    dict->flags |= 0x80;
}

local_inline int dict_owned_memory(const Dict *dict)
{
    assert(NULL != dict);
    return dict->flags & 0x80;
}

local warn_unused int
dict_map(Dict *dict,
         const void    *arg,
         int  (do_func *)(const Type *key,
                          const Type *val,
                          void *arg))
{
    assert(NULL != dict);

    struct visit_t v = dict_set_visit_struct(&v, dict);

    dict_for_each(v)
    {
        if (do_func(dict_vst_get_key(v), dict_vst_get_value(v), arg) < 0)
            return -1;
    }
    return 0;
}

local_inline size_t
dict_count_only_from(Dict *dict, size_t n)
{
    size_t  j = 0;
    struct visit_t v = dict_set_visit_struct_n(&v, dict, n);

    dict_for_each_mask(v)
    {
        j += POPCNT(dict_vst_get_mask(v));
        dict_for_each_next_mask(v);
    }
    return j;
}

warn_unused local_inline void *
dict_malloc(void **dp, size_t n)
{
    void *ptr = NULL;

    assert(0 != n);

#if defined(NDEBUG) AND NOT defined(NO_PyAPI)
    ptr = PyMem_Malloc(n);

#  elif NOT defined(NO_PyAPI)
    ptr = PyMem_Calloc(n, 1);

#  elif defined(NDEBUG)
    ptr = malloc(n);
// NDEBUG AND NO_PyAPI
#else
    ptr = calloc(n, sizeof(uint8_t));
#endif

    if (NULL != ptr AND NULL != dp)
        *dp = ptr;
    return ptr;
    //
#   define dict_malloc(d, s) dict_malloc((void *)(d), (s))
}

warn_unused local_inline void *
dict_realloc(void **dp, size_t n)
{
    void *ptr = NULL;

    assert(NULL != dp);

#  ifndef NO_PyAPI
    ptr = PyMem_Realloc(*dp, n);
#  else
    ptr = realloc(*dp, n);
#  endif

    NULL != ptr AND (*dp=ptr);
    return ptr;
    //
#   define dict_realloc(d, n) dict_realloc((void *)(d), (n))
}

local_inline void dict_free(void *ptr)
{
#  ifndef NO_PyAPI
    ptr = PyMem_Free(ptr);
#  else
    ptr = free(ptr);
#  endif
}

local_inline void *dict_malloc_self_(void **self, size_t UNUSED(n))
{
    Dict *dict = NULL;

#  ifndef NO_PyAPI
    assert(NULL != self AND NULL != *self);
    dict = (Dict *)(Py_TYPE(*self)->tp_alloc(*self, 0));
#  else
    dict = dict_malloc(self, sizeof(Dict));
#  endif

    dict_set_owned_memory(dict);
    return dict;
    //
#   define dict_malloc_self_(s, n) dict_malloc_self_((void *)(s), (n))
}

local_inline void *dict_free_self_(void **self)
{
    assert(NULL != self);

    Dict *dict = *self;

    if (NULL == dict)
        return dict;
    if (NOT dict_owned_memory(dict))
        return NULL;

#  ifndef NO_PyAPI
    Py_TYPE(dict)->tp_free(dict);
#  else
    dict_free(dict);
#  endif

    dict_unset(*self);
    return NULL;
    //
#   define dict_free_self_(s) dict_free_self_((void *)(s))
}

local_inline int dict_malloc_ckhv(void restrict **c,
                                  void restrict **v,
                                  void restrict **kh)
{
    if (NOT cache_malloc(c, n))
        return -1;
    if (dict_malloc(v, n * sizeof(Type *)) AND
        dict_malloc(kh, n * sizeof(khpair_t)))
    {
        // store dummy key and value before entries so that (key|value)[-1] returns it (in the case of error) 
        *(Type **)(*v)    = *(Type *)dict_value_empty();
        *(khpair_t *)(*v) = *(khpair_t *)(dict_key_empty());
        // move ahead of dummy key and value
        *v = (Type *)(*v) + 1;
        *kh = (khpair_t *)(*kh) + 1;
        return 0;
    }
    dict_free_ckhv(*c, *v, *kh);
    return -1;
    //
#   define dict_malloc_ckhv(c, v, kh) dict_malloc_ckhv(void*)c, (void*)v, (void*)kh)
}

local_inline void dict_free_ckhv(void * restrict c,
                                 void * restrict v,
                                 void * restrict kh)
{
    // TODO:  ensure not to attempt a free on static object
    calloc_free(c);
    if (NULL != v)
        dict_free((Type **)(v) - 1);
    if (NULL != kh)
        dict_free((khpair_t *)(kh) - 1);
}

local_inline void dict_free_ckhv_in_entry(Dict *dict)
{
    assert(NULL != dict);
    entry_t e = dict->entries;
    return dict_free_ckhv(e.cache, e.values, e.kh);
}

local int
dict_set(Dict **dict,
                size_t n,
                float  lf)
{
    assert(NULL != dict AND NULL != *dict);
    entry_t e = (*dict)->entries;
    void *kh  = e.kh, *v = e.values, *c = e.cache;

    if (out_of_range_lf(lf))
        return -1;
    if (0 == n)
        return dict_unset(dict);

    n = try_size_requirement(n, sizeof(khpair_t), lf);
    if (0 == n)
        return -1;
    if (dict_malloc_ckhv(&c, &v, &kh))
        return -1;
    dict_setcap(*dict, n, 0, lf);

    return 0;
}

local_inline void dict_unset(Dict **dict)
{
    assert(NULL != dict);
# ifndef NO_PyAPI
    const int i = dict_struct_offset();
    memcpy(*dict+i, dict_empty_struct()+i, dict_struct_size());
# else
    *dict = (Dict *)dict_empty_struct();
# endif
}

local_inline void
dict_alias_copy(const Dict * restrict dict, Dict * restrict alias)
{
    assert(NULL != dict AND NULL != alias);
    *alias = *dict;
}

warn_unused local void *
dict_new(void **type, ssize_t n, float lf)
{
    Dict *dict = NULL;

    dict = dict_malloc_self_(type, 0);
    if (NULL == dict)
         return dict;
    if (dict_set(&dict, n, lf) != 0)
        return dict_free(dict);
# ifdef NO_PyAPI
    dict AND (*type=dict);
# endif
    return d;
}

local void *dict_remove(Dict **dict)
{
    assert(NULL != dict && NULL != *dict);

    Dict alias = {0};
# ifdef NO_PyAPI
    clearfunc_t clear = (*dict)->clear;
# endif
    dict_alias_copy(*dict, &alias);
    dict_free_self_(dict); // TODO
# ifdef NO_PyAPI
    if (NULL != clear) dict_map(&alias, NULL, clear);
# else
    struct visit_t v = dict_set_visit_struct(&v, &alias);
    dict_for_each(v)
    {
        Py_DECREF(dict_vst_get_key(v));
        Py_DECREF(dict_vst_get_value(v));
    }
# endif
    dict_free_ckhv_in_entry(&alias);
#   define dict_remove(d) dict_remove(&d)
    return NULL;
}

local_inline void *
dict_copy_insert_n_(Dict * restrict dest,
                    const Dict * restrict src,
                    const size_t n)
{
    size_t  i = 0; // count copied entries
    struct visit_t v = dict_set_visit_struct(&v, src);

    dict_for_each(v)
    {
        khpair_t kh  = dict_vst_get_kh(v);
        Type *   val = dict_vst_get_value(v);

        if (dict_lookup_insert(dest, kh.key, val, kh.hash))
            return -1;
        if (++i < n)
            goto copied;
    }
 copied:
    dict_set_size(dest, i);
    return dest;
}

local_inline void *
dict_copy_insert_all_(Dict * restrict dest,
                      const Dict * restrict src,
                      const size_t n)
{
    struct visit_t v = dict_set_visit_struct(&v, src);

    dict_for_each(v)
    {
        khpair_t kh  = dict_vst_get_kh(v);
        Type *   val = dict_vst_get_value(v);

        if (dict_lookup_insert_(dest, kh.key, val, kh.hash))
            return NULL;
    }
    dict_set_size(dest, dict_size(src));
    return dest;
}

warn_unused local_inline void *
dict_copy(Dict * restrict dest,
          Dict * restrict src)
{
    Dict *d = dest;

    assert(NULL != src);

    if (dict_isempty(src))
        return dict_empty_struct();
    if (NULL == d AND NOT(d=dict_new(&dest, n, dict_load_fact(src))))
        return d;
    if (NULL == dict_copy_insert_all_(d, src))
        return (0 AND dict_remove(d)), NULL; // TODO
    return d;
}

warn_unused local_inline void *
dict_ncopy(Dict * restrict dest,
           Dict * restrict src,
           size_t n)
{
    Dict *d = dest;

    assert(NULL != src);

    if (dict_isempty(src))
        return dict_empty_struct();
    if (NULL == d AND NOT(d=dict_new(&dest, n, dict_load_fact(src))))
        return d;
    if (NULL == dict_copy_insert_n_(d, src, n))
        return (0 AND dict_remove(d)), NULL;
    return d;
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
dict_merge_(Dict *d1,1
            Dict *d2,
            Dict **dest)
{
    Dict *d = NULL;
    bool e1 = dict_isempty(d1), e2 = dict_isempty(d2);

    if (NOT e1 AND e2)
        return dict_copy(dest?*dest:NULL, d1);
    if (e1 AND NOT e2)
        return dict_copy(dest?*dest:NULL, d2);

    const size_t n = dict_size(d1) + dict_size(d2);
    Dict *d  = dict_new(dest, n, dict_load_fact(d1));

    if (NULL == d)
        return d;
    if (NOT dict_copy(d, d1) OR NOT dict_copy(d, d2))
        return dict_remove(d);
    dest AND (*dest=d);
    return d;
}

warn_unused local void *
dict_merge(Dict *d1,
           Dict *d2,
           Dict **dest)
{
    assert(NULL != d1 && NULL != d2);

    if (d1 == d2)
        return dict_copy(*dest, d1);
    return dict_merge_(d1, d2, dest);
}

warn_unused local void *
dict_merge_nocopy(Dict *d1,
                  Dict *d2,
                  Dict **dest)
{
    assert(NULL != d1 && NULL != d2);

    if (d1 == d2)
        return d1;
    return dict_merge_(d1, d2, dest);
}

local_inline int
dict_rehash(Dict **dict, size_t n)
{
    assert(NULL != dict AND NULL != *dict);

    if (0 == n)
        return dict_remove(*dict);

    Dict a = *dict, *d;

    dict_unset(*dict);
    // copy old dict
    d = n < dict_maxsize(&a) ? dict_ncopy(NULL, &a, n) : dict_copy(NULL, &a);
    if (NULL == d)
    {
        **dict = alias;
        return -1;
    }
    dict_remove(&a); // remove dict
    *dict = *d; // TODO: alias
    return 0;
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

locale_inline int
dict_update_key_in_entry(Dict *dict,
                         Type *restrict key,
                         Type *restrict value,
                         size_t j)
{
    Type *tmp = dict->entries.values[j];

    dict->entries.values[j] = value;
#   ifndef NO_PyAPI
    Py_XDECREF(tmp);
    Py_XDECREF(key); // key already exist
#   else
    if (NULL != dict->clear)
        dict->clear(key, tmp);
#   endif
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

    assert(NULL == entries.values[j]);
    assert(NULL == entries.kh[j].key);

    entries.cache[j]   = tag;
    entries.values[j]  = value;
    entries.kh[j].key  = key;
    entries.kh[j].hash = hash;

    inc_entry_size(dict);
    return 0;
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
    struct visit_t v  = {0}; // TODO

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
    struct visit_t v  = {0};
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

    const mm_t dup   = mm_duplicate(ctag(hash));
    struct visit_t v = {0};

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

// remove all macro definitions
#include "include/cleanup.h"
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
#else // QPy_MM_UNSUPPORTED
#error
#endif
