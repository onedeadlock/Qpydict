#ifndef QPy_DICT_H
#define QPy_DICT_H
#include <stdbool.h>
#include "include/defs.h"
#include "include/types.h"
#include "include/mm.h"
#ifndef QPy_MM_UNSUPPORTED

#define QPy_INCR(d)       ++(d->used_entries)
#define place_in_group(v) (QPy_DIVGROUP(QPy_ALIGN(v, QPy_GROUP_PS)))

QPy_INLINE(int) cache_tag(const uint64_t v)
{
    return ((v & 0xff) - ((v & 0xff) * 0x2041u >> 20) * 127) + 1;
}

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
    const QPy_hash_t hash  = PyObject_Hash(key);
    const size_t group_idx = place_in_group(hash & self->nentries - 1);
    const uint8_t  tag     = cache_tag(hash);
    const mm_t dup         = mm_duplicate(tag);
    
    size_t probe=0, cnt=0;

    if (hash < 0)
	return -1;

    while (true) {
	size_t i         = (probe + group_idx) & (self->group_size - 1);
	mm_t   group = mm_load(self->cache + i);
	mask_t mask  = mm_test_equal(group, dup);

	for (QPyDict_Array it = self->entries + i; mask;
	     mask &= mask - 1)
	    {
		int j   = mm_scan_mask(mask);
		int cmp = generic_compare(it+j, key, hash);
		
		if (QPy_UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return dict_update(self, key, value, hash, tag, j);
	    }

	mask = mm_test_equal(group, _QPy_ZERO);
	if (mask)
	    {
		int k = mm_scan_mask(mask);
		QPy_INCR(self);
		return dict_update(self, key, value, hash, tag, k);
	    }
	probe += (cnt++) + 1;
    }
    QPy_UNREACHABLE();
}

QPy_PTR_INLINE(int) QPy_lookup_generic(QPyDictObject *self, QPy_PyObject key, QPy_PyObject value)
{
    const hash_t hash      = PyObject_Hash(key);
    const size_t group_idx = place_in_group(hash & self->nentries - 1);
    const uint8_t  tag     = cache_tag(hash);
    const mm_t dup         = mm_duplicate(tag);
    
    size_t probe=0, cnt=0; ssize_t k=-1;

    if (hash < 0)
	return -1;

    while (true) {
	size_t i         = (probe + group_idx) & (self->group_size - 1);
	mm_t   group = mm_load(self->cache + i);
	mask_t mask  = mm_test_equal(group, dup);

	for (QPyDict_Array it = self->entries + i; mask;
	     mask &= mask - 1)
	    {
		int j   = mm_scan_mask(mask);
		int cmp = generic_compare(it+j, key, hash);
		if (QPy_UNLIKELY(cmp < 0))
		    return -1;
		if (cmp)
		    return dict_update(self, key, value, hash, tag, j);
	    }
	if (k < 0)
	    k = mm_find_empty_slot(group);

	if (mm_test_empty(group))
	    break;

	probe += (cnt++) + 1;
    }
    if (k != -1)
	{
	    QPy_INCR(self);
	    return dict_update(self, key, value, hash, tag, k);
	}
    QPy_UNREACHABLE();
}

#else // QPy_MM_UNSUPPORTED
#    error
#endif
