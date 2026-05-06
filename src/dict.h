#ifndef QPy_DICT_H
#define QPy_DICT_H
#include <stdbool.h>
#include "include/defs.h"
#include "include/types.h"
#include "include/arch_arm.h"
#include "include/arch_i386.h"
#include "include/arch_generic.h"

#define QPy_INCR(d)       ++(d->used_entries)
#define QPy_cache_tag_(v) ((v) - ((v) * 0x2041u >> 20) * 127) + 1
#define QPy_place_in_group(v) (QPy_DIVGROUP(QPy_ALIGN(v)))

QPy_INLINE(int) generic_compare(const QPyDict_Array it, const QPy_PyObject key, const QPy_hash_t hash)
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

QPy_PTR_INLINE(int) insert_generic_nodeleted(QPyDictObject *self, QPy_PyObject key, QPy_PyObject value)
{
    const size_t group_idx = QPy_place_in_group(hash & self->nentries - 1);
    const QPy_hash_t hash  = PyObject_hash(key);
    const uint8_t  tag     = QPy_cache_tag_(hash);
    const QPy_mm_t dup     = QPy_mm_duplicate(tag);
    
    size_t probe=0, cnt=0;

    if (hash < 0)
	return -1;

    while (true) {
	size_t i         = (probe + group_idx) & (self->group_size - 1);
	QPy_mm_t   group = QPy_mm_load(self->cache + i);
	QPy_mask_t mask  = QPy_mm_test_equal(group, dup);

	for (QPyDict_Array it = self->entries + i; mask;
	     mask &= mask - 1)
	    {
		int j   = QPy_scan_mask(mask);
		int cmp = generic_compare(it+j, key, hash);
		if (QPy_UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return dict_update(self, key, value, hash, tag, j);
	    }

	mask = QPy_mm_test_equal(group, _QPy_ZERO);
	if (mask)
	    {
		int k = QPy_scan_mask(mask);
		QPy_INCR(self);
		return dict_update(self, key, value, hash, tag, k);
	    }
	probe += (cnt++) + 1;
    }
}

QPy_PTR_INLINE(int) QPy_lookup_generic(QPyDictObject *self, QPy_PyObject key, QPy_PyObject value)
{
    const size_t group_idx = QPy_align_size(hash & self->nentries - 1);
    const QPy_hash_t hash  = PyObject_hash(key);
    const uint8_t  tag     = QPy_cache_tag_(hash);
    const QPy_mm_t dup     = QPy_mm_duplicate(tag);
    
    size_t probe=0, cnt=0; ssize_t k=-1;

    if (hash < 0)
	return -1;

    while (true) {
	size_t i         = (probe + group_idx) & (self->group_size - 1);
	QPy_mm_t   group = QPy_mm_load(self->cache + i);
	QPy_mask_t mask  = QPy_mm_test_equal(group, dup);

	for (QPyDict_Array it = self->entries + i; mask;
	     mask &= mask - 1)
	    {
		int j   = QPy_scan_mask(mask);
		int cmp = generic_compare(it+j, key, hash);
		if (QPy_UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return dict_update(self, key, value, hash, tag, j);
	    }
	if (k < 0)
	    k = QPy_find_empty_slot(group);

	if (QPy_mm_test_empty(group))
	    break;

	probe += (cnt++) + 1;
    }
    if (k != -1)
	{
	    QPy_INCR(self);
	    return dict_update(self, key, value, hash, tag, k);
	}
    // unreachable
    return -1;
}

#endif
