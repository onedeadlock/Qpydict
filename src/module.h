#ifndef QPYDICT_MODULE_H
#define QPYDICT_MODULE_H
#include "types.h"

#define Qpydict_module_name "QPydict"
#define Qpydict_module_doc  "A very fast Python dictionary"

static int Qpydict_module_exec(QPyDict_PyObject module);
static QPyDict_PyObject version(QPyDict_PyObject module, QPyDict_PyObject arg);

// Initialization
static QPyDict_PyObject QPyDict_new(PyTypeObject *cls, QPyDict_PyObject args, QPyDict_PyObject kwds);
static int QPyDict_init(QPyDict_PyObject _self, QPyDict_PyObject args, QPyDict_PyObject kwds);
static void QPyDict_dealloc(QPyDict_PyObject _self);
static int QPyDict_traverse(QPyDict_PyObject _self, visitproc visit, void *arg);

// Class Methods
QPyDict_PyObject QPyDict_Clear(QPyDict_PyObject self);
QPyDict_PyObject QPyDict_Contains(QPyDict_PyObject self, QPyDict_PyObject arg);
QPyDict_PyObject QPyDict_SetItem(QPyDict_PyObject self, QPyDict_PyObject args, QPyDict_PyObject kwargs);
QPyDict_PyObject QPyDict_GetItem(QPyDict_PyObject self, QPyDict_PyObject args);

static PyMemberDef QPyDict_attr[] = {
    {
	.name   = "capacity",
	.type   = QPy_T_SSIZE,
	.offset = offsetof(QPyDictObject, capacity),
	.flags  = Py_READONLY,
	.doc    = PyDoc_STR("")
    },
    {
	.name   = "size",
	.type   = QPy_T_SSIZE,
	.offset = offsetof(QPyDictObject, size),
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
    {Py_tp_dealloc,    QPyDict_dealloc},
    {Py_tp_traverse,   QPyDict_traverse},
    {Py_tp_members,    QPyDict_attr},
    {Py_tp_methods,    NULL},
    {Py_tp_hash,       PyObject_HashNotImplemented},
    {0, NULL}
};

static PyType_Spec QPyDict_clsspec = {
    .name      = "QPydict.QPyDict",
    .basicsize = sizeof(QPyDictObject),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE | Py_TPFLAGS_HAVE_GC,
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

#endif //QPydict_MODULE_H
