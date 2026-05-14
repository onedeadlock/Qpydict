#ifndef QPy_DICT_H
#define QPy_DICT_H
#include <stdbool.h>
#include <strings.h>
#include <assert.h>
#include "include/defs.h"
#include "include/types.h"
#include "include/mm.h"

#ifndef QPy_MM_UNSUPPORTED

#define DPTR(dptr) (*(void **)(dptr))
#define DCR(x)     --(x)
#define get_NEXT_GROUP(x) ((x) += NGROUP)

#define out_of_range_lf(lf) ((lf) < .3 OR (lf) > 1.)
#define inc_entry_size(d) ++((d)->used_size)

#define dict_slot(d, p, i) (((p) + (i)) & ((d)->group_capacity - 1))
#define probe_next_dict_slot(p, c) ((p) += ((c)++) + 1)

#ifndef CACHE_TAG_NOZERO
local_inline int PURE(ctag)(const uint64_t v)
{
    return ((v & 0xff) - ((v & 0xff) * 0x2041u >> 20) * 127) + 1;
}
#else
#    define ctag(v) ((v) & 0xff)
#endif

local_inline size_t PURE(find_group_from_hash)
    (
     const hash_t hash,
     const size_t size
     )
{
    return ALIGN(hash & (size - 1), NGROUP) / NGROUP;
}

warn_unused local void *caligned_malloc
(
    void          *memdptr,
    const uint16_t align_size,
    const size_t   size
    )
{
    const uint16_t align_offset_size = sizeof(uint16_t);
    const uint16_t align_fault       = align_size - 1;
    const uint16_t max_offset_size   = align_offset_size + align_fault;

    if (size > (SIZE_MAX - max_offset_size))
	{
	    // alignment would cause overflow
	    return NULL;
	}

    void *ptr = malloc(size + max_offset_size);
    if (NULL == ptr)
		return ptr;

	// align memory
    void *kptr = ALIGNU((uintptr_t)ptr + max_offset_size, align_size);

	// save align size (used for further memory op)
    ((uint16_t *)kptr)[-1] = (uintptr_t)kptr - (uintptr_t)ptr;

    DDPTR(memdptr) = kptr;
    return kptr;
}

local void caligned_free(void *memptr)
{
    if (memptr == NULL)
	return;

    const uint16_t align_size = ((uint16_t *)memptr)[-1];
    const void     *memstart  = (uintptr_t)memptr - align_size;

    return free(memstart);
}

warn_unused local_inline void * aligned_malloc_set
(
	const size_t   size,
	const uint16_t align_size,
	const int      fchar
)
{
    void *ptr = NULL;

    if (caligned_malloc(&ptr, align_size, size))
	return memset(ptr, fchar, size);
    return p;
}

local_inline void *aligned_calloc
(
	const size_t   size,
	const uint16_t align_size
)
{
    void *ptr = NULL;

    if (caligned_malloc(&ptr, align_size, size))
	return memset(ptr, 0, size);
    return ptr;
}

warn_unused local_inline
void *_cache_alloc(void *dptr, size_t size)
{
    void *ptr = NULL;

    assert(size != 0);

#ifndef CACHE_TAG_NOZERO
    ptr = aligned_malloc_set(size, NGROUP, QPy_EMPTY);
#else
    ptr = aligned_calloc(size, NGROUP);
#endif

    DDPTR(dptr) = ptr;
    return ptr;
}

local_inline void _cache_free(void *ptr)
{
    return caligned_free(ptr);
}

warn_unused local_inline
void * _malloc(void *dptr, size_t size)
{
    void *ptr = NULL;

    assert(size != 0);

#ifndef NDEBUG
    ptr = calloc(1, size);
#else
    ptr = malloc(size);
#endif

    DDPTR(dptr) = ptr;
    return ptr;
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
	    1U   << (32 - BSF(n))
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
	    1U   << (31 - BSF(n))
#endif
	    );
}

local_inline size_t get_size_no_resize_trigger
(
	const size_t size,
	const float   lf
)
{
    return next_power_of_two(size / lf);
}

local_inline size_t try_size_requirement
(
	const size_t size,
	const size_t max_object_size,
	const float  lf
)
{
    assert(size != 0);

    size_t try_size = get_size_no_resize_trigger(size, lf);

    if (check_if_safe_mul(try_size, max_object_size, (size_t)0))
	{
	    // try_size would overflow
#        if USE_EXACT_SIZE
	    return 0;
#	 else
	    return size; // object will trigger an early resize
#	 endif
	}

    return try_size;
}

local_inline int key_generic_compare
(
	const khpair_t it,
	const Type    *key,
	const hash_t   hash
)
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

locale_inline int dict_update_key_in_entry
(
	QPyDictObject  *dict,
	Type *restrict key,
	Type *restrict value,
	ssize_t j
)
{
    Type *tmp = dict->entries.values[j];

    dict->entries.values[j] = value;
    Py_XDECREF(tmp);
    Py_XDECREF(key); // key already exist

    return 0;
}

locale_inline size_t dict_add_entry
(
	QPyDictObject *dict,
	Type *restrict key,
	Type *restrict value,
	hash_t  hash,
	ssize_t tag,
	ssize_t j
)
{
    entry_t entries = dict->entries;

    assert(j * NGROUP <= dict->capacity);
    assert(NULL != entries.values[j]);
    assert(NULL != entries.kh[j]->key);

    entries.cache[j]    = tag;
    entries.values[j]   = value;
    entries.kh[j]->key  = key;
    entries.kh[j]->hash = hash;

    inc_entry_size(dict);
    return 0;
}

local_inline void dict_set
(
    QPyDictObject  *dict,
    void * restrict che,
    void * restrict val,
    void * restrict kh,
    ssize_t         cap,
    ssize_t         size,
    float           lf
    )
{
    assert(NULL != dict);
    assert(0 == dict->capacity);

    dict->entries.cache  = che;
    dict->entries.values = val;
    dict->entries.kh     = kh;

    dict->group_capacity = cap / NGROUP;
    dict->capacity       = cap;
    dict->max_size       = (lf * cap) + 0.5;
    dict->used_size      = size;
}

local_inline void dict_unset(QPyDictObject *dict)
{
    assert(NULL != dict);
    assert(0 == dict->capacity);

    dict->entries.cache  = empty_tag_full_group;
    dict->entries.values = NULL;
    dict->entries.kh     = NULL;

    dict->group_capacity = 0;
    dict->capacity       = 0;
    dict->max_size       = 0;
    dict->used_size      = 0;
}

local_inline const ssize_t dict_size(QPyDictObject *dict)
{
    assert(NULL != dict);
    return dict->used_size;
}

local_inline const ssize_t dict_capacity(QPyDictObject *dict)
{
    assert(NULL != dict);
    return dict->capacity;
}

local_inline int dict_isempty(QPyDictObject *dict)
{
    assert(NULL != dict);
    return dict->capacity == 0;
}

local_inline int dict_isunused(QPyDictObject *dict)
{
    assert(NULL != dict);
    return dict->capacity == 0;
}

local_inline int dict_explicit_set_load_factor(QPyDictObject *dict, float lf)
{
    assert(NULL != dict);

    if (out_of_range_lf(lf))
	return -1;

    /** if old_lf is greater than new_lf and size does not
	meet the new_lf criteria, a rehash must occur on
	the next call to a dict mutating function. */
    dict->max_size = dict->capacity * lf;
    return 0;
}

local int
dict_alloc_internal(
		    QPyDictObject *dict,
		    const ssize_t size,
		    const float   lf
		    )
{
    assert(NULL != dict);

    if (size < 0 OR out_of_range_lf(lf))
	return -1;
    if (0 == size)
	{
	    dict_unset(dict);
	    return 0;
	}

    const size_t n = try_size_requirement(size, sizeof(khpair_t), lf);
    if (0 == n)
	return -1;

    void *kh, *v, *c;

    if (NULL == _cache_alloc(&c, n) OR
	NULL == _malloc(&v,  n * sizeof(Type *)) OR
	NULL == _malloc(&kh, n * sizeof(khpair_t))
	)
	{
	    _cache_free(c);
	    _free(v);
	    return -1;
	}
    dict_set(dict, c, v, kh, n, 0, lf);
    return n;
}


local void *dict_resize(QPyDictObject * UNUSED(dict), ssize_t UNUSED(new_size))
{
}

local void *dict_copy(QPyDictObject *dict, ssize_t size)
{
    assert(NULL != dict);

    if (0 == size)
	size = dict->used_size;
    if (dict_isempty(dict))
	return NULL;
    // TODO
    return NULL;
}

local void *dict_clone(QPyDictObject *dict)
{
    assert(NULL != dict);

    if (0 == size)
	size = dict->capacity;
    if (dict_isempty(dict))
	return NULL;
    // TODO
    return NULL;
}

local void *dict_merge(QPyDictObject *dict, QPyDictObject *dict0)
{
    assert(NULL != dict && NULL != dict0);

    if (0 == size)
	size = dict->used_size;
    if (dict_isempty(dict) AND dict_isempty(dict0))
	return NULL;
    if (dict == dict0)
	return dict_copy(dict);
    // TODO
    return NULL;
}

local_inline visit_t new_visit_struct(QPyDictObject *dict)
{
    visit_t v = {
	.group = dict->entries.cache,
	.size  = dict->group_capacity,
	.mask  = 0
    };

    return v;
}
local_inline int visit_next_nonempty_group(visit_t *v)
{
    assert(NULL != v);
    group = v->group;
    if (NULL == group)
	return 0;
    assert(v->size < 1);

    while (true);
	{
	    v->mask = mm_test_hasentry(mm_load(v->group)); // TODO
	    if (LIKELY(v->mask))
		return 1;
	    if (DCR(v->size))
		return (v->group = NULL) != NULL;

	    get_NEXT_GROUP(v->group);
	}
    UNREACHABLE();
}

local warn_unused int dict_visit_all_do
(
    QPyDictObject *dict,
    const void *arg,
    int (do_func *)(const khpair_t kh, const Type *val, void *arg);
    )
{
    assert(NULL != dict);

    visit_t v   = new_visit_struct(dict);
    ssize_t ret = 1, i = 0, j = 0;

    // TODO:  mark CRITICAL SECTION
    while (visit_next_nonempty_group(&v))
	{
	    while (LIKELY(mask AND ret))
		{
		    j   = i * NGROUP + mm_scan_mask(mask);
		    ret = do_func(dict->entries.kh[j],
				  dict->entries.values[j],
				  arg);
		    mask &= mask - 1;
		}
	    i++;
	}
    return -!ret;
}

local ssize_t lookup_insert_generic_nodeleted
(
	QPyDictObject  *dict,
	Type *restrict key,
	Type *restrict value
)
{
	assert(NULL != dict);
	assert(NULL != key);

	const hash_t hash  = HASH(dict, key);

    if (hash < 0)
	return -1;

    const size_t group_idx = find_group_from_hash(hash, dict->capacity);
    const uint8_t  tag     = ctag(hash);
    const mm_t dup         = mm_duplicate(tag);

    size_t probe=0, cnt=0;

    do {
	size_t i     = dict_slot(dict, probe, group_idx);
	mm_t   group = mm_load(dict->entries.cache + i);
	mask_t mask  = mm_test_equal(group, dup);

	for (khpair_t it = dict->entries.kh + i; mask;
	     mask &= mask - 1)
	    {
		int j   = mm_scan_mask(mask);
		int cmp = key_generic_compare(it+j, key, hash);

		if (UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return dict_update_key_in_entry(dict,
						    key,
						    value,
						    j);
	    }
	mask = mm_test_empty(group); // TODO
	if (LIKELY(mask))
	    return dict_add_entry(dict,
				  key,
				  value,
				  hash,
				  tag,
				  mm_scan_mask(mask));

	probe_next_dict_slot(probe, cnt);
    } while (true);

    UNREACHABLE();
}

locale_inline ssize_t lookup_insert_generic
(
	QPyDictObject *dict,
	Type *restrict key,
	Type *restrict value
)
{
	assert(NULL != dict);
	assert(NULL != key);

	const hash_t hash  = HASH(dict, key);

    if (hash < 0)
	return -1;

    const size_t group_idx = find_group_from_hash(hash, dict->capacity);
    const uint8_t  tag     = ctag(hash);
    const mm_t     dup     = mm_duplicate(tag);

    size_t probe=0, cnt=0; ssize_t k=-1;

    do {
	size_t i     = dict_slot(dict, probe, group_idx);
	mm_t   group = mm_load(dict->entries.cache + i);
	mask_t mask  = mm_test_equal(group, dup);

	for (khpair_t it = dict->entries.kh + i; mask;
	     mask &= mask - 1)
	    {
		int j   = mm_scan_mask(mask);
		int cmp = key_generic_compare(it+j, key, hash);

		if (UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return dict_update_key_in_entry(dict,
						    key,
						    value,
						    j);
	    }
	if (k < 0)
	    k = mm_find_empty_slot(group); // TODO

	if (LIKELY(mm_test_empty_fast(group)))
	    break; // TODO
	probe_next_dict_slot(probe, cnt);
    } while (true);

    if (k != -1)
	return dict_add_entry(dict,
			      key,
			      value,
			      hash,
			      tag,
			      k);
    UNREACHABLE();
}

locale_inline ssize_t lookup_generic
(
	QPyDictObject *dict,
	PyObject      *key
)
{
	assert(NULL != dict);
	assert(NULL != key);

	const hash_t hash  = HASH(dict, key);

    if (hash < 0)
	return -1;

    const size_t group_idx = find_group_from_hash(hash, dict->capacity);
    const uint8_t  tag     = ctag(hash);
    const mm_t     dup     = mm_duplicate(tag);

    size_t probe=0, cnt=0;

    do {
	size_t i     = dict_slot(dict, probe, group_idx);
	mm_t   group = mm_load(dict->entries.cache + i);
	mask_t mask  = mm_test_equal(group, dup);

	for (khpair_t it = dict->entries.kh + i; mask;
	     mask &= mask - 1)
	    {
		int j   = mm_scan_mask(mask);
		int cmp = key_generic_compare(it+j, key, hash);

		if (UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return 0;
	    }
	if (LIKELY(mm_test_empty_fast(group)))
	    break;
	probe_next_dict_slot(probe, cnt);
    } while (true);

    UNREACHABLE();
}

#else // QPy_MM_UNSUPPORTED
#    error
#endif
