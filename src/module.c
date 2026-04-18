#include "module.h"

static QPyDict_PyObject version(QPyDict_PyObject QPy_UNUSED(module))
{
    Py_INCREF(Py_None);
    return Py_None;
}

static int QPydict_module_exec(QPyDict_PyObject module)
{
    if (PyType_FromModuleAndSpec(module, spec, NULL))
	return 0;

    return -1;
}

Py_INITFUNC PyInit_QPydict(void)
{
    return PyModuleDef_Init(&QPydict_Module);
}

static QPyDict_PyObject QPyDict_new(PyTypeObject *clstype, QPyDict_PyObject QPy_UNUSED(args), QPyDict_PyObject QPy_UNUSED(kwds))
{
    // Allocate memory for our object
    return (QPyDict_PyObject)(cls->tp_alloc(clstype, 0));
}

static int QPyDict_init(QPyDict_PyObject cls, QPyDict_PyObject args, QPyDict_PyObject kwds)
{
    QPyDictObject *self = (QPyDictObject *)cls;

    return 0;
}

static int QPyDict_tranverse(QPyDict_PyObject cls, visitproc QPy_UNUSED(visit), void * QPy_UNUSED(arg))
{
    // allow qpydict type to be tracked by GC (preventing cyclic references)
    Py_VISIT(Py_TYPE(cls));

    return 0;
}

static void QPyDict_dealloc(QPyDict_PyObject cls)
{
    QPyDictObject *self = (QPyDictObject *)cls;

    // untrack from GC
    PyObject_GC_UNTRACK(Py_TYPE(self));
    Py_CLEAR(Py_TYPE(self));

    /**
       TDDO
       (1) clean all dict items. Basically:
       while ((self->sz)--)
       {
           Py_XDECREF(self->Array->key);
           Py_XDECREF(self->Array->val);
       }
       (2)
       Deallocate Array, Policy, Cache
    */

    // free class object
    Py_TYPE(self)->tp_free(self);
}
