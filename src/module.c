#include "module.h"

#define QPy_STINNULL(functype) __attribute__((nonnull, always_inline)) static inline functype
#define QPy_SETVAL(lv, rv)       ((lv) = (rv))
#define QPy_RAISE_Err(type, msg) (PyErr_SetString(type, msg), QPy_Err)
#define QPy_RAISE_BADARG(msg)    QPy_RAISE_Err(PyExc_TypeError, msg)
#define QPy_RAISE_OVERFLOW(msg)  QPy_RAISE_Err(PyExc_OverFlowError, msg)

enum {
    QPy_Err = -1,
    QPy_LONG = 321,
    QPy_LONG_OR_SEQ,
    QPy_SEQ,
    QPy_MAP
};

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
    return QPy_Err;
}

QPy_STINNULL(int) QPyDict_GET_COMMON_OBJECT_SIZE(QPyDict_PyObject arg, QPy_ssize_t *size, int op)
{
    if (QPy_LONG_OR_SEQUENCE == op)
	{
	    QPy_ssize_t QPy_UNUSED(_size);

	    if (PyLong_Check(arg))
		{
		    _size = PyLong_AsSsize_t(arg);
		    return _size < 0 ? QPy_Err : (QPy_SETVAL(*size, _size), QPy_LONG);
		}

	    // TODO: sequence/iterable type/instance here
	    if (0)
		{
		    return QPy_SEQ;
		}
	    return QPy_RAISE_BADARG("`arg` is not an integer or does not support the sequence/iterator protocol@call:internal:QPyDict_GET_COMMON_OBJECT_SIZE(arg, ...)");
	}

    // size from MappingType object/instance (dict-like) 
    if ((QPy_MAP == op) && !PyMapping_Check(arg))
	return QPy_RAISE_BADARG("`arg` does not suppprt the mapping protocol@call:internal:QPyDict_GET_COMMON_OBJECT_SIZE(arg, ...)");

    QPy_SETVAL(*size, PyMapping_Size(arg));
    return QPy_MAP;
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

static int QPyDict_init(QPyDict_PyObject _self, QPyDict_PyObject arg, QPyDict_PyObject kwargs)
{
    QPyDictObject * QPy_UNUSED(self) = (QPyDictObject *)_self;
    QPy_ssize_t QPy_UNUSED(as);
    QPy_ssize_t QPy_UNUSED(ks);
    QPy_ssize_t QPy_UNUSED(rs);
    int QPy_UNUSED(type);

    rs = ks = as = type = 0;
    if (NULL != arg)
	{
	    arg = PyTuple_GET_ITEM(arg, 0); 
	    if (NULL == arg)
		return QPy_Err;

	    type = QPyDict_GET_COMMON_OBJECT_SIZE(arg, &as, QPy_LONG_OR_SEQ);
	    if (type < 0)
		{
		    Py_DECREF(arg);
		    return QPy_Err;
		}
	}
		
    if (NULL != kwargs && (QPyDict_GET_COMMON_OBJECT_SIZE(arg, &ks, QPy_MAP) < 0))
	return -1;

    // create dict of size argsize + kwsize
    rs = argsize + kwsize;
    if (rs < 0)
	{
	    Py_XDECREF(arg);
	    return QPy_RAISE_OVERFLOW("Integer Overflow (ssize_t)");
	}

    // TODO: create dict here
    
    if (type && (type != QPy_LONG))
	{
	    // TODO: arg is an iterable/mapping type instance. for each pair of item in arg, add to our dict
	}

	    Py_XDECREF(arg);
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
