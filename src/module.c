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
    QPy_Err      = -1,
    QPy_LONG     = 0x01,
    QPy_SEQUENCE = 0x02,
    QPy_MAP      = 0x04,
    QPy_ITER     = 0x08,
    QPy_ALL      = 0x0f
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
    if ((op & QPy_MAP) && PyMapping_Check(arg))
	return (QPy_SETVAL(*size, PyMapping_Size(arg)), QPy_MAP);
    if ((op & QPy_SEQUENCE) && PySequence_Check(arg))
	return (QPy_SETVAL(*size, PySequence_Size(arg)), QPy_SEQUENCE);
    if ((op & QPy_ITER) && PyIter_Check(arg))
	return (QPy_SETVAL(*size, QPy_DEFAULT_SIZE), QPy_ITER);
    if ((op & QPy_LONG) && PyLong_Check(arg))
	{
	    QPy_ssize_t _size = PyLong_AsSsize_t(arg);
	    return _size < 0 ? QPy_Err : (QPy_SETVAL(*size, _size), QPy_LONG);
	}
    return QPy_Err;
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
    QPyDict_PyObject item, key, value;
    uintptr_t err;

    if (! PyIter_Check(iter))
	return QPy_Err;
    item = key = value = err = 0;
    while (PyIter_NextItem(iter, &item) > 0 && PyIter_Check(item))
	{
	    // get key and value from item
	    err  = PyIter_NextItem(item, &key) < 1 || PyIter_Next(item, &value) < 1;
	    // insert key and value into dict
	    err || QPyDict_insert(self, key, value, &err);

	    Py_DECREF(item); item = NULL;
	    if (err)
		break;
	    key = value = NULL;
	}
    if (NULL != item)
	{
	    Py_DECREF(item);
	    QPy_RAISE_Err(PyExc_TypeError, "");
	    return QPy_Err;
	}
    if (err) {
	Py_XDECREF(key);
	Py_XDECREF(value);
	if (err == 1)
	    QPy_RAISE_Err(PyExc_ValueError, "");
	else
	    QPy_RAISE_Err((QPyDict_PyObject)(void *)err, "");
	return QPy_Err;
    }

    return 0;
}

QPy_INLINE(int) QPyDict_SeqAsDict(QPyDictObject *self, QPyDict_PyObject seq, Py_ssize_t size)
{
    return 0;
}

QPy_INLINE(int) QPyDict_MapAsDict(QPyDictObject *self, QPyDict_PyObject map, Py_ssize_t size)
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
    QPyDictObject * QPy_UNUSED(self);
    QPy_ssize_t QPy_UNUSED(as);   // size of positional arg
    QPy_ssize_t QPy_UNUSED(ks);   // size of keyword args
    int         QPy_UNUSED(type), QPy_UNUSED(err);

    ks = as = type = 0;
    // Get size and type of positional & keyword arguments
    if (NULL != arg)
	{
	    arg = PyTuple_GET_ITEM(arg, 0);
	    if (NULL == arg)
		return QPy_Err;
	    type = QPyDict_GetCommonObjectSize(arg, &as, QPy_ALL);
	}
    if (NULL != kwargs)
	QPyDict_GetCommonObjectSize(arg, &ks, QPy_MAP);

    // Allocate memory for entries
    self = (QPyDictObject *)_self;
    if (QPy_Err == QPyDict_CustomInit(self, as + ks))
	{
	    Py_XDECREF(arg);
	    return QPy_Err;
	}

    // Insert entries into dict
    if (! (QPy_LONG & type))
	{
	    // entries from positional argument
	    err = (type & (QPy_SEQUENCE|QPy_ITER)) && QPyDict_IterAsDict(self, args, as);
	    err = (type & QPy_MAP)      && QPyDict_MapAsDict(self, args, as);
	}
    // entries from keyword arguments
    if (err || (QPy_Err == QPyDict_MapAsDict(self, kwargs, ks)))
	{
	    // error! Deep clean dict
	    QPyDict_ClearEntries(self);
	    Py_XDECREF(arg);
	    return QPy_Err;
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
