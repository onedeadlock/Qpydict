#ifndef QPYDICT_MODULE_H
#define QPYDICT_MODULE_H
#include "include/types.h"
#include "include/methods.h"
#include "include/defs.h"

#define QPy_class_qualname  "Qpydict.qpydict"
#define QPy_class_name      "qpydict"
#define Qpydict_module_name "Qpydict"
#define Qpydict_module_doc  "A very fast Python dictionary"

#define python_set_exception(exc, msg) PyErr_SetString(exc, msg)
#define python_raise_badargument(msg)  PyErr_SetString(PyExc_TypeError, msg)
#define python_raise_overflow(msg)     PyErr_SetString(PyExc_OverflowError, msg)

#if Py_VERSION_HEX >= 0x030e00000
#    define python_iterator_next(it, arg) (PyIter_NextItem(it, arg) > 0)
#else
#    define python_iterator_next(it, arg) (*(arg)=PyIter_Next(it))
#endif

#define python_tuple_getitem(tup, item, idx) !((item)=PyTuple_GetItem(tup, i))

static int Qpydict_module_exec(PyObject *module);
static PyObject *version(PyObject *module, PyObject *arg);

// Initialization
static PyObject *QPyDict_new(PyTypeObject *cls, PyObject *args, PyObject *kwds);
static int QPyDict_init(PyObject *_self, PyObject *args, PyObject *kwds);
static void QPyDict_dealloc(PyObject *_self);
static int QPyDict_traverse(PyObject *_self, visitproc visit, void *arg);

// Class Methods
PyObject *QPyDict_Clear(PyObject *self);
PyObject *QPyDict_Contains(PyObject *self, PyObject *arg);
PyObject *QPyDict_SetItem(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *QPyDict_GetItem(PyObject *self, PyObject *args);

static PyMemberDef QPyDict_attr[] = {
    {
	.name   = "_capacity",
	.type   = QPy_T_SSIZE,
	.offset = offsetof(QPyDictObject, nentries),
	.flags  = Py_READONLY,
	.doc    = PyDoc_STR("")
    },
    {
	.name   = "_size",
	.type   = QPy_T_SSIZE,
	.offset = offsetof(QPyDictObject, used_entries),
	.flags  = Py_READONLY,
	.doc    = PyDoc_STR("")
    },
    {NULL}
};

static struct PyMethodDef QPyDict_clsmethods[] __attribute__((unused)) = {
    {
	.ml_name  = "",
	.ml_meth  = (PyCFunction)NULL,
	.ml_flags = 0,
	.ml_doc   = PyDoc_STR("")
    },
    {NULL}
};

static PyType_Slot QPyDict_slots[] = {
    {Py_tp_new,        QPyDict_new},
    {Py_tp_init,       QPyDict_init},
    {Py_tp_alloc,      PyType_GenericAlloc},
    {Py_tp_free,       PyObject_GC_Del},
    {Py_tp_dealloc,    QPyDict_dealloc},
    {Py_tp_traverse,   QPyDict_traverse},
    {Py_tp_members,    QPyDict_attr},
    {Py_tp_methods,    NULL},
    {Py_tp_hash,       PyObject_HashNotImplemented},
    {0, NULL}
};

static PyType_Spec QPyDict_clsspec = {
    .name      = QPy_class_qualname,
    .basicsize = sizeof(QPyDictObject),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots      = QPyDict_slots
};

static PyMethodDef Qpydict_module_methods[] = {
    {"version", (PyCFunction)version, METH_NOARGS, PyDoc_STR("Print Current Version of the Qpydict Module")},
    {NULL, (PyCFunction)NULL, 0, NULL}
};

static PyModuleDef_Slot Qpydict_module_slots[] = {
    {Py_mod_exec, Qpydict_module_exec},
    {0, NULL}
};

static PyModuleDef Qpydict_Module = {
    PyModuleDef_HEAD_INIT,
    .m_name       = Qpydict_module_name,
    .m_doc        = Qpydict_module_doc,
    .m_size       = 0,
    .m_methods    = Qpydict_module_methods,
    .m_slots      = Qpydict_module_slots,
};

static PyObject *version(PyObject *QPy_UNUSED(module), PyObject *QPy_UNUSED(arg))
{
    Py_INCREF(Py_None);
    return Py_None;
}

static int Qpydict_module_exec(PyObject *module)
{
    // Create, initialize and add class to the module's namespace
    PyObject *cls = PyType_FromModuleAndSpec(module, &QPyDict_clsspec, NULL);

    if (NULL == cls || PyModule_AddObject(module, QPy_class_name, cls) < 0)
        {
            Py_XDECREF(cls);
            return QPy_Err;
        }
    return 0;
}

PyMODINIT_FUNC PyInit_Qpydict(void)
{
    return PyModuleDef_Init(&Qpydict_Module);
}

#endif //QPydict_MODULE_H
