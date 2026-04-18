#include "module.h"

static QPyDict_PyObject version(QPyDict_PyObject QPy_UNUSED(module), QPyDict_PyObject QPy_UNUSED(arg))
{
    Py_INCREF(Py_None);
    return Py_None;
}

static int Qpydict_module_exec(QPyDict_PyObject module)
{
    PyTypeObject *cls = (PyTypeObject *)PyType_FromModuleAndSpec(module, &QPyDict_clsspec, NULL);

    // Add class to the modules namespace (dict)
    if (cls && !(PyModule_AddType(module, cls) < 0))
	{
	    Py_DECREF(cls);
	    return 0;
	}
    Py_DECREF(cls);
    return -1;
}

PyMODINIT_FUNC PyInit_Qpydict(void)
{
    return PyModuleDef_Init(&Qpydict_Module);
}

static QPyDict_PyObject QPyDict_new(PyTypeObject *cls, QPyDict_PyObject QPy_UNUSED(args), QPyDict_PyObject QPy_UNUSED(kwds))
{
    // Allocate memory for our object
    return (QPyDict_PyObject)(cls->tp_alloc(cls, 0));
}

static int QPyDict_init(QPyDict_PyObject _self, QPyDict_PyObject args, QPyDict_PyObject kwds)
{
    QPyDictObject * QPy_UNUSED(self) = (QPyDictObject *)_self;
    return 0;
}

static int QPyDict_traverse(QPyDict_PyObject _self, visitproc visit, void *arg)
{
    // allow qpydict type to be tracked by GC (preventing cyclic references)
    Py_VISIT(Py_TYPE(_self));

    return 0;
}

static void QPyDict_dealloc(QPyDict_PyObject _self)
{
    QPyDictObject *self = (QPyDictObject *)_self;
    PyTypeObject  *cls  = Py_TYPE(self);

    // untrack from GC
    PyObject_GC_UnTrack(cls);
    Py_CLEAR(cls);

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
    cls->tp_free(self);
}
