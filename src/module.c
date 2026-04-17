#include "module.h"

static QPyDict_PyObject version(QPyDict_PyObject QPy_UNUSED(self), QPyDict_PyObject QPy_UNUSED(arg))
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

Py_INITFUNC PyInit_QPyDict(void)
{
  return PyModuleDef_Init(&QPyDict_module);
}

static void QPyDict_dealloc(QPyDict_PyObject cls)
{
    QPyDictObject *self = (QPyDictObject *)cls;

    /* TDDO
       (1) clean all dict items
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

static QPyDict_PyObject QPyDict_new(PyTypeObject *cls, QPyDict_PyObject args, QPyDict_PyObject kwds)
{
  // Allocate QPyDict Class
  return (QPyDict_PyObject)cls->tp_alloc(cls, 0);
}


static int QPyDict_init(QPyDict_PyObject selfobj, QPyDict_PyObject args, QPyDict_PyObject kwds)
{
  QPyDictObject *self = (QPyDictObject *)selfobj;

  return 0;
}
