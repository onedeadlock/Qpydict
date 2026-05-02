#ifndef QPy_DICT_H
#define QPy_DICT_H
#include <stdbool.h>
#include "internal/include/def.h"
#include "internal/include/types.h"

QPy_PTR_INLINE(int) QPy_insert(QPyDictObject *self, QPy_PyObject key, QPy_PyObject value)
{
    const size_t group_idx = QPy_align_size(hash & self->nentries - 1);
    const QPy_hash_t hash  = 0;
    const uint8_t  tag     = QPy_cache_tag_(hash);
    const QPy_mm_t dup     = QPy_mm_duplicate(tag);
    
    size_t probe=0, cnt=0, i=0, j=0, k=-1;
    
    while (true) {
	i = (probe + group_idx) & (self->group_size - 1);

	QPy_mm_t   group = QPy_mm_load(self->cache + i);
	QPy_mask_t mask  = QPy_mm_test_equal(group, dup);

	for (QPyDict_Array it = self->entries + i; mask; )
	    {
		
		j = QPy_scan_mask(mask);

		if (QPy_generic_compare(it, key, hash, j))
		    return QPy_update_dict(self, key, value, hash, tag, k);
		mask &= mask - 1;
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
