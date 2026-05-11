#ifndef QPy_DICT_H
#define QPy_DICT_H
#include <stdbool.h>
#include <strings.h>
#include "include/defs.h"
#include "include/types.h"
#include "include/mm.h"
#ifndef QPy_MM_UNSUPPORTED

#ifndef UINTPTR_MAX
typedef uintptr_t unsigned long int
#endif
#define DDPTR(dptr) (*(void **)(dptr))

#define dict_slot(d, p, i) (((p) + (i)) & ((d)->group_capacity - 1))
#define probe_next_dict_slot(p, c) ((p) += ((c)++) + 1)

#define inc_entry_size(d) ++((d)->size)

#ifndef QPy_IMPRAND
QPy_INLINE(int) __attribute__((pure)) ctag(const uint64_t v)
{
    return ((v & 0xff) - ((v & 0xff) * 0x2041u >> 20) * 127) + 1;
}
#else
#define ctag(v) ((v) & 0xff)
#endif

QPy_INLINE(size_t) __attribute__((pure)) find_group_from_hash(const hash_t hash, const size_t size)
{
    const size_t group = QPy_ALIGN(hash & (size - 1));

    return group / QPy_GROUP;
}

__attribute__((warn_unused_result, nonnull))
static void *caligned_malloc(void *mempptr, uint16_t align_size, size_t size)
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
    // align memory to align size
    void *kptr = QPy_ALIGNU((uintptr_t)ptr + max_offset_size, align_size);
    // save align size (used for further memory op)
    ((uint16_t *)kptr)[-1] = (uintptr_t)kptr - (uintptr_t)ptr;
    
    DDPTR(mempptr) = kptr;
    return kptr;
}

static void caligned_free(void *memptr)
{
    if (memptr == NULL)
	return;

    const uint16_t align_size = ((uint16_t *)memptr)[-1];
    const void     *memstart  = (uintptr_t)memptr - align_size; // move behind offset (initial block start)

    return free(memstart);
}

__attribute__((warn_unused_result))
static inline void *aligned_malloc_set(const size_t size, const uint16_t align_size, const int fchar)
{
    void *ptr = NULL;

    if (caligned_malloc(&ptr, align_size, size))
	return memset(ptr, fchar, size);
    return p;
}

__attribute__((warn_unused_result))
static inline void *aligned_calloc(const size_t size, const uint16_t align_size)
{
    void *ptr = NULL;

    if (caligned_malloc(&ptr, align_size, size))
	return memset(ptr, 0, size);
    return ptr;
}

__attribute__((warn_unused_result, nonnull))
static inline void *_cache_alloc(void *pptr, size_t size)
{
    void *ptr = NULL;

    assert(size != 0);
#ifdef QPy_IMPRAND
    // allocate memory with aligned to number of slots per group (always a power of two) and set with empty char
    ptr = aligned_malloc_set(size, QPy_GROUP, QPy_EMPTY);
#else
    // the same as above but zero out memory instead
    ptr = aligned_calloc(size, QPy_GROUP);
#endif
    DDPTR(pptr) = ptr;
    return ptr;
}

__attribute__((warn_unused_result, nonnull))
static inline void *_malloc(void *pptr, size_t size)
{
    void *ptr = NULL;

    assert(size != 0);
#ifndef NDEBUG
    ptr = calloc(1, size);
#else
    ptr = malloc(size);
#endif
    DDPTR(pptr) = ptr;
    return ptr;
}

static inline void _free(void *ptr)
{
    return free(ptr);
}

QPy_INLINE(size_t) next_power_of_two(size_t n)
{
    assert(n != 0);

    return (
#if (SIZE_MAX > 0xffffffffU)
	    1ULL << (64 - __builtin_clz(n))
#else
	    1U   << (32 - __builtin_clz(n))
#endif
	    );
}

QPy_INLINE(size_t) prev_power_of_two(size_t n)
{
    assert(n != 0);

    // TODO: if n equals 1, should we return a correct 1, or 2 to make n remain a power of 2?
    return (
#if (SIZE_MAX > 0xffffffffU)
	    1ULL << (63 - __builtin_clz(n))
#else
	    1U   << (31 - __builtin_clz(n))
#endif
	    );
}

QPy_INLINE(size_t) get_size_no_resize_trigger(const size_t size, const double lf)
{
    // returned value maybe alot larger than size
    return next_power_of_two(size + (1 - lf) * size);
}

QPy_INLINE(int) key_generic_compare(const khpair_t it, const PyObject *key, const hash_t hash)
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

QPy_INLINE(int) dict_update_key_in_entry(QPyDictObject *self, PyObject *restrict key, PyObject *restrict value, ssize_t j)
{
    PyObject *tmp = self->entries.values[j];

    self->entries.values[j] = value;
    Py_XDECREF(tmp);

    // key already exist in dict so no use here
    Py_XDECREF(key);
    return 0;
}

QPy_INLINE(ssize_t) dict_add_entry(QPyDictObject *self, PyObject *restrict key, PyObject *restrict value, hash_t hash, ssize_t tag, ssize_t j)
{
    entry_t entries = self->entries;

    assert(j * QPy_GROUP <= self->capacity);
    assert(NULL == entries.values[j]);
    assert(NULL == entries.kh[j].key);

    entries.cache[j]  = tag;
    entries.values[j] = value;

    entries.kh[j].key  = key;
    entries.kh[j].hash = hash;

    inc_entry_size(self);
    return 0;
}

static int dict_alloc_internal(QPyDictObject *self, ssize_t size)
{
    if (size < 0)
	return -1;
    if (0 == size)
	return 0;
 
    void *kh, *v, *c;

    if (NULL == _cache_alloc(&c, size) ||
	NULL == _malloc(&v,  size * sizeof(PyObject *)) ||
	NULL == _malloc(&kh, size * sizeof(khpair_t)))
	{
	    _cache_free(c);
	    _free(v);
	    return -1;
	}

    self->entries.cache  = c;
    self->entries.values = v;
    self->entries.kh     = kh;

    self.capacity = size;
    self.group_capacity = size / QPy_GROUP;
    return 0;
}


QPy_PTR_INLINE(ssize_t) lookup_insert_generic_nodeleted(QPyDictObject *self, PyObject *restrict key, PyObject *restrict value)
{
    const hash_t hash  = PyObject_Hash(key);

    if (hash < 0)
	return -1;

    const size_t group_idx = find_group_from_hash(hash, self->capacity);
    const uint8_t  tag     = ctag(hash);
    const mm_t dup         = mm_duplicate(tag);
    
    size_t probe=0, cnt=0;

    do {
	size_t i     = dict_slot(self, probe, group_idx);
	mm_t   group = mm_load(self->entries.cache + i);
	mask_t mask  = mm_test_equal(group, dup);

	for (khpair_t it = self->entries.kh + i; mask;
	     mask &= mask - 1)
	    {
		int j   = mm_scan_mask(mask);
		int cmp = key_generic_compare(it+j, key, hash);
		
		if (QPy_UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return dict_update_key_in_entry(self, key, value, j);
	    }
	mask = mm_test_empty(group); // TODO
	if (QPy_LIKELY(mask))
	    return dict_add_entry(self, key, value, hash, tag, mm_scan_mask(mask));

	probe_next_dict_slot(probe, cnt);
    } while (true);

    QPy_UNREACHABLE();
}

QPy_PTR_INLINE(ssize_t) lookup_insert_generic(QPyDictObject *self, PyObject *restrict key, PyObject *restrict value)
{
    const hash_t hash      = PyObject_Hash(key);

    if (hash < 0)
	return -1;

    const size_t group_idx = find_group_from_hash(hash, self->capacity);
    const uint8_t  tag     = ctag(hash);
    const mm_t     dup     = mm_duplicate(tag);
    
    size_t probe=0, cnt=0; ssize_t k=-1;

    do {
	size_t i     = dict_slot(self, probe, group_idx);
	mm_t   group = mm_load(self->entries.cache + i);
	mask_t mask  = mm_test_equal(group, dup);

	for (khpair_t it = self->entries.kh + i; mask;
	     mask &= mask - 1)
	    {
		int j   = mm_scan_mask(mask);
		int cmp = key_generic_compare(it+j, key, hash);
		
		if (QPy_UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return dict_update_key_in_entry(self, key, value, j);
	    }
	if (k < 0)
	    k = mm_find_empty_slot(group); // TODO

	if (QPy_LIKELY(mm_test_empty_fast(group)))
	    break; // TODO
	probe_next_dict_slot(probe, cnt);
    } while (true);

    if (k != -1)
	return dict_add_entry(self, key, value, hash, tag, k);
    QPy_UNREACHABLE();
}

QPy_PTR_INLINE(ssize_t) lookup_generic(QPyDictObject *self, PyObject *key)
{
    const hash_t hash      = PyObject_Hash(key);

    if (hash < 0)
	return -1;

    const size_t group_idx = find_group_from_hash(hash, self->capacity);
    const uint8_t  tag     = ctag(hash);
    const mm_t     dup     = mm_duplicate(tag);
    
    size_t probe=0, cnt=0;

    do {
	size_t i     = dict_slot(self, probe, group_idx);
	mm_t   group = mm_load(self->entries.cache + i);
	mask_t mask  = mm_test_equal(group, dup);

	for (khpair_t it = self->entries.kh + i; mask;
	     mask &= mask - 1)
	    {
		int j   = mm_scan_mask(mask);
		int cmp = key_generic_compare(it+j, key, hash);
		
		if (QPy_UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return 0;
	    }
	if (QPy_LIKELY(mm_test_empty_fast(group)))
	    break;
	probe_next_dict_slot(probe, cnt);
    } while (true);

    QPy_UNREACHABLE();
}


#else // QPy_MM_UNSUPPORTED
#    error
#endif
