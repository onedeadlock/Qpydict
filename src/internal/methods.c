#include "include/methods.h"

int QPyDict_insert(QPyDictObject *self, QPyDict_PyObject key, QPyDict_PyObject value, void *exc)
{
    // NOTIMPLEMENTED: keys and values will leak
    (void)self;
    (void)key;
    (void)value;
    return 0;
}

void QPyDict_ClearEntries(QPyDictObject *self)
{
    // NOTIMPLEMENTED: dictionary still lives in memory (leak)
    (void)self;

    if (NULL == self)
	return;
}
