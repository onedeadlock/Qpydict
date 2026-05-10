#ifndef QPy_DICT_H
#define QPy_DICT_H
#include <stdbool.h>
#include "include/defs.h"
#include "include/types.h"
#include "include/mm.h"
#ifndef QPy_MM_UNSUPPORTED

#define QPy_UNREACHABLE() (void)0 // TODO

#define dict_slot(d, p, i) (((p) + (i)) & ((d)->group_capacity - 1))
#define probe_next_dict_slot(p, c) ((p) += ((c)++) + 1)

#define inc_entry_size(d) ++((d)->size)

QPy_INLINE(int) __attribute__((pure)) ctag(const uint64_t v)
{
    return ((v & 0xff) - ((v & 0xff) * 0x2041u >> 20) * 127) + 1;
}

QPy_INLINE(size_t) __attribute__((pure)) find_group_from_hash(const hash_t hash, const size_t size)
{
    const size_t group = QPy_ALIGN(hash & (size - 1));

    return group / QPy_GROUP;
}

QPy_INLINE(int) key_generic_compare(const khpair_t it, const PyObject *key, const QPy_hash_t hash)
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

dict_add_entry(QPyDictObject *self, PyObject *restrict key, PyObject *restrict value, hash_t hash, ssize_t tag, ssize_t j)
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
