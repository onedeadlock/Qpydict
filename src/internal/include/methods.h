#ifndef QPy_METHODS_H
#define QPy_METHODS_H
#include "types.h"

int QPyDict_insert(QPyDictObject *self, QPyDict_PyObject key, QPyDict_PyObject value, void *exc);

void QPyDict_ClearEntries(QPyDictObject *self);

#endif //QPy_METHODS_H
