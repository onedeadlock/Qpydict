#ifndef QPyDict_MODULE_H
#define QPyDict_MODULE_H

#define Py_SSIZE_T_CLEAN
#include <Python.h>

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#define QPydict_module_name "QPydict"
#define QPydict_module_doc  "A very fast Python dictionary"
#define QPy_UNUSED(x) Py_UNUSED(x)
#define QPy_T_SSIZE  Py_T_PYSSIZET

typedef PyObject * QPyDict_PyObject;
typedef Py_ssize_t QPy_ssize_t;
static int QPydict_module_exec(QPyDict_PyObject module);
static QPyDict_PyObject version(QPyDict_PyObject module);

// Initialization
static QPyDict_PyObject QPyDict_new(PyTypeObject *clstype, QPyDict_PyObject args, QPyDict_PyObject kwds);
static int QPyDict_init(QPyDict_PyObject cls, QPyDict_PyObject args, QPyDict_PyObject kwds);
static void QPyDict_dealloc(QPyDict_PyObject cls);
static int QPyDict_tranverse(QPyDict_PyObject cls, visitproc visit, void *arg);

// Class Methods
QPyDict_PyObject QPyDict_Clear(QPyDict_PyObject self);
QPyDict_PyObject QPyDict_Contains(QPyDict_PyObject self, QPyDict_PyObject arg);
QPyDict_PyObject QPyDict_SetItem(QPyDict_PyObject self, QPyDict_PyObject args, QPyDict_PyObject kwargs);
QPyDict_PyObject QPyDict_GetItem(QPyDict_PyObject self, QPyDict_PyObject args);

static const char *QPyDict_docs[] = {
    ""
};

// QPyDict (Instance) Object
typedef struct {
    PyObject_HEAD
    QPyDict_Cache  cache;
    QPyDict_Array  data;
    QPyDict_Policy policy;
    QPy_ssize_t    capacity;
    QPy_ssize_t    grpsize;
    QPy_ssize_t    size;
    QPy_ssize_t    lf;
} __attribute__((aligned(64))) QPyDictObject;

static PyMemberDef QPyDict_attr[] = {
    {
	.name   = "capacity",
	.type   = QPy_T_SSIZE,
	.offset = offsetof(QPyDictObject, capacity),
	.flags  = Py_READONLY,
	.doc    = PyDoc_STR(QPyDict_docs[0])
    },
    {
	.name   = "size",
	.type   = QPy_T_SSIZE,
	.offset = offsetof(QPyDictObject, size),
	.flags  = Py_READONLY,
	.doc    = PyDoc_STR(QPyDict_docs[0])
    }
    {NULL}
};

static struct PyMethodDef QPyDict_clsmethods[] = {
    {
	.ml_name  = "";
	.ml_meth  = (PyCFunction)NULL;
	.ml_flags = 0;
	.ml_doc   = PyDoc_STR(QPyDict_docs[0]);
    },
    {NULL}
};

static PyType_Slot QPyDict_slots[] = {
    {Py_tp_new,        QPyDict_new},
    {Py_tp_init,       QPyDict_init},
    {Py_tp_alloc,      PyType_GenericAlloc},
    {Py_tp_dealloc,    QPyDict_dealloc},
    {Py_tp_transverse, QPyDict_transverse},
    {Py_tp_members,    QPyDict_attr},
    {Py_tp_methods,    QPyDict_clsmethods},
    {Py_tp_hash,       PyObject_HashNotImplemented},
    {NULL}
};

static PyType_Spec QPyDict_clsspec = {
    .name      = "QPydict.QPyDict",
    .doc       = PyDoc_STR(QPyDict_docs[0]),
    .basicsize = sizeof(QPyDictObject),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE | Py_TPFLAGS_HAVE_GC,
    .slot      = QPyDict_slots
};

static PyMethodDef QPydict_module_methods[] = {
    {"version", (PyCFunction)version, METH_NOARGS, PyDoc_STR("Print Current Version of the Qpydict Module")},
    {NULL, (PyCFunction)NULL, 0, NULL}
};

static PyModuleDef_Slot QPydict_module_slots[] = {
    {Py_mod_exec, QPydict_module_exec},
    {NULL, 0}
};

static PyModuleDef QPydict_Module = {
    PyModuleDef_HEAD_INIT,
    .m_name       = QPyDict_module_name,
    .m_doc        = QPydict_module_doc;
    .m_size       = 0;
    .m_methods    = QPydict_module_methods;
    .m_slots      = QPydict_module_slots;
};

#endif //QPydict_MODULE_H
