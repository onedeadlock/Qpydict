#ifndef QPy_DICT_H
#define QPy_DICT_H
#include <stdbool.h>
#include "internal/include/defs.h"
#include "internal/include/types.h"
#include "internal/include/arch_arm.h"
#include "internal/include/arch_i386.h"
#include "internal/include/arch_generic.h"

#define QPy_cache_tag_(v) ((v) - ((v) * 0x2041u >> 20) * 127) + 1

QPy_INLINE(int) QPy_generic_compare(const QPyDict_Array it, const QPy_PyObject key, const QPy_hash_t hash)
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

QPy_PTR_INLINE(int) QPy_lookup_generic_nodeleted(QPyDictObject *self, QPy_PyObject key, QPy_PyObject value)
{
    const size_t group_idx = QPy_align_size(hash & self->nentries - 1);
    const QPy_hash_t hash  = PyObject_hash(key);
    const uint8_t  tag     = QPy_cache_tag_(hash);
    const QPy_mm_t dup     = QPy_mm_duplicate(tag);
    
    QPy_ssize_t probe=0, cnt=0;

    if (hash < 0)
	return -1;

    while (true) {
	QPy_ssize_t i    = (probe + group_idx) & (self->group_size - 1);
	QPy_mm_t   group = QPy_mm_load(self->cache + i);
	QPy_mask_t mask  = QPy_mm_test_equal(group, dup);

	for (QPyDict_Array it = self->entries + i; mask;
	     mask &= mask - 1)
	    {
		int j   = QPy_scan_mask(mask);
		int cmp = QPy_generic_compare(it+j, key, hash);
		if (QPy_UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return QPy_update_dict(self, key, value, hash, tag, j);
	    }

	mask = QPy_mm_test_equal(group, _QPy_ZERO);
	if (mask)
	    {
		int k = QPy_scan_mask(mask);
		self->used_entries += 1;
		return QPy_update_dict(self, key, value, hash, tag, k);
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
    
    QPy_ssize_t probe=0, cnt=0, k=-1;

    if (hash < 0)
	return -1;

    while (true) {
	QPy_ssize_t i    = (probe + group_idx) & (self->group_size - 1);
	QPy_mm_t   group = QPy_mm_load(self->cache + i);
	QPy_mask_t mask  = QPy_mm_test_equal(group, dup);

	for (QPyDict_Array it = self->entries + i; mask;
	     mask &= mask - 1)
	    {
		int j   = QPy_scan_mask(mask);
		int cmp = QPy_generic_compare(it+j, key, hash);
		if (QPy_UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return QPy_update_dict(self, key, value, hash, tag, j);
	    }
	if (k < 0)
	    k = QPy_find_empty_slot(group);

	if (QPy_mm_test_empty(group))
	    break;

	probe += (cnt++) + 1;
    }
    if (k != -1)
	{
	    self->used_entries += 1;
	    return QPy_update_dict(self, key, value, hash, tag, k);
	}
    return -1;
}

#endif
