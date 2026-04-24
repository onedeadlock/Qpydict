#include "include/methods.h"

int QPy_insert(QPyDictObject *self, QPy_PyObject key, QPy_PyObject value, void *exc)
{
    // NOT_IMPLEMENTED
    (void)self;
    Py_DECREF(key);
    Py_DECREF(value);
    return 0;
}

void QPy_ClearEntries(QPyDictObject *self)
{
    // NOTIMPLEMENTED: dictionary still lives in memory (leak)
    (void)self;

    if (NULL == self)
	return;
}
