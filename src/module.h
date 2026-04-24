#ifndef QPYDICT_MODULE_H
#define QPYDICT_MODULE_H
#include "internal/include/types.h"
#include "internal/include/methods.h"

#define QPy_class_qualname  "Qpydict.qpydict"
#define QPy_class_name      "qpydict"
#define Qpydict_module_name "Qpydict"
#define Qpydict_module_doc  "A very fast Python dictionary"

static int Qpydict_module_exec(QPy_PyObject module);
static QPy_PyObject version(QPy_PyObject module, QPy_PyObject arg);

// Initialization
static QPy_PyObject QPyDict_new(PyTypeObject *cls, QPy_PyObject args, QPy_PyObject kwds);
static int QPyDict_init(QPy_PyObject _self, QPy_PyObject args, QPy_PyObject kwds);
static void QPyDict_dealloc(QPy_PyObject _self);
static int QPyDict_traverse(QPy_PyObject _self, visitproc visit, void *arg);

// Class Methods
QPy_PyObject QPyDict_Clear(QPy_PyObject self);
QPy_PyObject QPyDict_Contains(QPy_PyObject self, QPy_PyObject arg);
QPy_PyObject QPyDict_SetItem(QPy_PyObject self, QPy_PyObject args, QPy_PyObject kwargs);
QPy_PyObject QPyDict_GetItem(QPy_PyObject self, QPy_PyObject args);

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

#endif //QPydict_MODULE_H
