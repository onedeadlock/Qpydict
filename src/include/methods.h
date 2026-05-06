#ifndef QPy_METHODS_H
#define QPy_METHODS_H
#include "types.h"

int QPy_insert(QPyDictObject *self, QPy_PyObject key, QPy_PyObject value, void *exc);

void QPy_ClearEntries(QPyDictObject *self);

#endif //QPy_METHODS_H
