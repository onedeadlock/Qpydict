#include "module.h"

#define QPy_PTR_INLINE(type) __attribute__((nonnull, always_inline)) static inline type
#define QPy_INLINE(type)     __attribute__((always_inline))         static inline type
#define QPy_SETVAL(lv, rv)       ((lv) = (rv))
#define QPy_RAISE_Err(type, msg) (PyErr_SetString(type, msg), QPy_Err)
#define QPy_RAISE_BADARG(msg)    QPy_RAISE_Err(PyExc_TypeError, msg)
#define QPy_RAISE_OVERFLOW(msg)  QPy_RAISE_Err(PyExc_OverflowError, msg)

#define QPyDict_CACHE(self)   ((self)->cache)
#define QPyDict_ENTRIES(self) ((self)->entries)
#define QPyDict_SIZE(self)    (self)->nentries)
#define QPyDict_LEN(self)     ((self)->used_entries)
#define QPyDict_GSIZE(self)   ((self)->group_size)
#define QPy_TMPCACHE(self)    NULL

enum {
    QPy_Err = -1,
    QPy_LONG = 321,
    QPy_LONG_OR_SEQUENCE,
    QPy_SEQUENCE,
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

PyMODINIT_FUNC PyInit_Qpydict(void)
{
    return PyModuleDef_Init(&Qpydict_Module);
}

static void * QPyDict_malloc(QPy_ssize_t size, void *ptr)
{
    void *mem = calloc(1, size);

    if (mem)
	*(void **)ptr = mem;

    return mem;
}

static void * QPyDict_aligned_malloc(QPy_ssize_t QPy_UNUSED(size), void *QPy_UNUSED(ptr))
{
    return NULL;
}

static void QPyDict_free(void *ptr)
{
    free(ptr);
}

static void QPyDict_aligned_free(void * QPy_UNUSED(ptr))
{
}

QPy_PTR_INLINE(int) QPyDict_GetCommonObjectSize(QPyDict_PyObject arg, QPy_ssize_t *size, int op)
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
	    return QPy_RAISE_BADARG("`arg` is not an integer or does not support the sequence/iterator protocol@call:internal:QPyDict_GetCommonObjectSize");
	}

    // size from MappingType object/instance (dict-like)
    if ((QPy_MAP == op) && !PyMapping_Check(arg))
	return QPy_RAISE_BADARG("`arg` does not suppprt the mapping protocol@call:internal:QPyDict_GetCommonObjectSize");

    QPy_SETVAL(*size, PyMapping_Size(arg));
    return QPy_MAP;
}

QPy_INLINE(void *) QPyDict_ClearObject(QPyDictObject *self)
{
    if (self)
	{
	    memset(self, 0, sizeof (QPyDictObject));
	    QPyDict_CACHE(self)  = QPy_TMPCACHE();
	}
    return self;
}

QPy_PTR_INLINE(int) QPyDict_CustomInit(QPyDictObject *self, QPy_ssize_t size)
{
    if (size < 0)
	return QPy_RAISE_OVERFLOW("Integer Overflow:@call:internal:__init__");

    if (size != 0)
	{
	    QPyDict_Array ar_; QPyDict_Cache ch_;

	    if (!QPyDict_malloc(QPy_ARRAY_SIZE * size, &ar_) || !QPyDict_malloc(QPy_CACHE_SIZE * size, &ch_))
		{
		    QPyDict_free(ar_);
		    QPyDict_free(ch_);
		    return QPy_RAISE_Err(PyExc_MemoryError, "Out of Memory@call:internal:__init__");
		}
	    QPyDict_ENTRIES(self) = ar_;
	    QPyDict_CACHE(self)   = ch_;
	    QPyDict_SIZE(self)    = size;
	    QPyDict_GSIZE(self)   =  0;
	}
    return 0;
}

QPy_INLINE(int) QPyDict_IterAsDict(QPyDictObject *self, QPyDict_PyObject iter)
{
    QPyDict_PyObject QPy_UNUSED(item);

    if (PyIter_Check(iter))
	{
	    while (PyIter_Next(iter, &item) > 0)
		{
		    QPyDict_PyObject key, value;

		    if (PyIter_Next(item, &key) < 1)
			{
			    Py_DECREF(item);
			    return QPy_Err;
			}
		    if (PyIter_Next(item, &value) < 1)
			{
			    Py_DECREF(item);
			    return QPy_Err;
			}

		    if (QPyDict_insert(self, key, value))
			{
			    Py_DECREF(item);
			    return QPy_Err;
			}
		    Py_DECREF(item);
		}
	}
    return 0;
}

QPy_INLINE(int) QPyDict_SeqAsDict(QPyDictObject *self, QPyDict_PyObject iter, Py_ssize_t size)
{
    return 0;
}

QPy_INLINE(int) QPyDict_MappingAsDict(QPyDictObject *self, QPyDict_PyObject iter, Py_ssize_t size)
{
    return 0;
}

static QPyDict_PyObject QPyDict_new(PyTypeObject *cls, QPyDict_PyObject QPy_UNUSED(args), QPyDict_PyObject QPy_UNUSED(kwds))
{
    QPyDictObject *self = cls->tp_alloc(cls, 0);

    return QPy_ClearObject(self);
}

static int QPyDict_init(QPyDict_PyObject _self, QPyDict_PyObject arg, QPyDict_PyObject kwargs)
{
    QPyDictObject * QPy_UNUSED(self) = (QPyDictObject *)_self;
    QPy_ssize_t QPy_UNUSED(as);   // size of object in arg
    QPy_ssize_t QPy_UNUSED(ks);   // size of keyword args
    int         QPy_UNUSED(type); // type returned by QPyDict_GetCommonObjectSize

    ks = as = type = 0;
    if (NULL != arg)
	{
	    arg = PyTuple_GET_ITEM(arg, 0);
	    if (NULL == arg)
		return QPy_Err;

	    type = QPyDict_GetCommonObjectSize(arg, &as, QPy_LONG_OR_SEQUENCE);
	    if (type < 0)
		{
		    Py_DECREF(arg);
		    return QPy_Err;
		}
	}
    if (NULL != kwargs && (QPyDict_GetCommonObjectSize(arg, &ks, QPy_MAP) < 0))
	{
	    Py_XDECREF(arg);
	    return QPy_Err;
	}

    if (QPy_Err == QPyDict_CustomInit(self, as + ks))
	{
	    Py_XDECREF(arg);
	    return QPy_Err;
	}

    if (type && (type != QPy_SEQUENCE))
	{
	    while ()
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
