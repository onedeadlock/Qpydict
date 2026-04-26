#include "module.h"

#define QPy_PTR_INLINE(type) __attribute__((nonnull, always_inline)) static inline type
#define QPy_INLINE(type)     __attribute__((always_inline))         static inline type
#define QPy_LIKELY(expr)     __builtin_expect(!!(expr), 1)
#define QPy_UNLIKELY(expr)   __builtin_expect(!!(expr), 0)
#define QPy_SETVAL(lv, rv)      ((lv) = (rv))
#define QPy_SETEXC(type, msg)   (PyErr_SetString(type, msg), QPy_Err)
#define QPy_RAISE_BADARG(msg)   QPy_SETEXC(PyExc_TypeError, msg)
#define QPy_RAISE_OVERFLOW(msg) QPy_SETEXC(PyExc_OverflowError, msg)
// qpy-iter-next: return 0 for success else 1
#define QPy_ITERNEXT(iter, arg) QPy_SETVAL(*arg, PyIter_Next(iter)) // TODO: use PyIter_NextItem for python>=3.14
#define QPy_TUPLE_GETITEM(tuple, item, i) !(item = PyTuple_GetItem(tuple, i))

#define QPy_CACHE(self)   ((self)->cache)
#define QPy_ENTRIES(self) ((self)->entries)
#define QPy_SIZE(self)    ((self)->nentries)
#define QPy_LEN(self)     ((self)->used_entries)
#define QPy_GSIZE(self)   ((self)->group_size)
#define QPy_TMPCACHE(self)    NULL

enum {
    QPy_Err      = -1,
    QPy_LONG     = 0x01,
    QPy_SEQUENCE = 0x02,
    QPy_MAP      = 0x04,
    QPy_ITER     = 0x08,
    QPy_ALL      = 0x0f
};

static QPy_PyObject version(QPy_PyObject QPy_UNUSED(module), QPy_PyObject QPy_UNUSED(arg))
{
    Py_INCREF(Py_None);
    return Py_None;
}

static int Qpydict_module_exec(QPy_PyObject module)
{
    // Create, initialize and add class to the module's namespace
    QPy_PyObject cls = PyType_FromModuleAndSpec(module, &QPyDict_clsspec, NULL);

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


static void * QPy_malloc(QPy_ssize_t size, void *ptr)
{
    void *mem = calloc(1, size);

    if (mem)
	*(void **)ptr = mem;
    return mem;
}

__attribute__((unused)) static void * QPy_aligned_malloc(QPy_ssize_t QPy_UNUSED(size), void *QPy_UNUSED(ptr))
{
    return NULL;
}

static void QPy_free(void *ptr)
{
    free(ptr);
}

__attribute__((unused)) static void QPy_aligned_free(void * QPy_UNUSED(ptr))
{
}

QPy_PTR_INLINE(int) QPy_GetCommonObjectSize(QPy_PyObject arg, QPy_ssize_t *size, int op)
{
    if ((op & QPy_LONG) && PyLong_Check(arg))
	{
	    QPy_ssize_t _size = PyLong_AsSsize_t(arg);
	    return _size < 0 ? QPy_Err : (QPy_SETVAL(*size, _size), QPy_LONG);
	}
    QPy_SETVAL(*size, PyObject_LengthHint(arg, QPy_DEFAULT_SIZE));
    return (QPy_ALL & ~QPy_LONG);
}

QPy_INLINE(void *) QPy_ClearObject(QPyDictObject *self)
{
    if (self)
	{
	    QPy_ENTRIES(self) = NULL;
	    QPy_CACHE(self)   = QPy_TMPCACHE();
	    QPy_LEN(self)     = 0;
	    QPy_SIZE(self)    = 0;
	    QPy_GSIZE(self)   = 0;
	}
    return self;
}

QPy_PTR_INLINE(int) QPy_CustomInit(QPyDictObject *self, QPy_ssize_t size)
{
    if (size < 0)
	return QPy_RAISE_OVERFLOW("Integer Overflow:@call:internal:__init__");

    if (size != 0)
	{
	    QPyDict_Array ar_ = NULL; QPyDict_Cache ch_ = NULL;

	    if (!QPy_malloc(QPy_ARRAY_SIZE * size, &ar_) || !QPy_malloc(QPy_CACHE_SIZE * size, &ch_))
		{
		    QPy_free(ar_);
		    QPy_free(ch_);
		    return QPy_SETEXC(PyExc_MemoryError, "Out of Memory@call:internal:__init__");
		}
	    QPy_ENTRIES(self) = ar_;
	    QPy_CACHE(self)   = ch_;
	    QPy_SIZE(self)    = size;
	    QPy_GSIZE(self)   =  0;
	}
    return 0;
}

QPy_INLINE(int) QPy_MappingCheck(const QPy_PyObject arg)
{
    return PyObject_HasAttrString(arg, "keys");
}

QPy_INLINE(int) QPy_GetSizeFromArgKwargs(const QPy_PyObject restrict arg, const QPy_PyObject restrict kwargs)
{
    QPy_ssize_t as = 0, ks = 0, t = 0;

    if (NULL != arg
	&& (t = QPy_GetCommonObjectSize(arg, &as, QPy_ALL)) < 0)
	    return QPy_Err;
    if (NULL != kwargs)
	QPy_GetCommonObjectSize(arg, &ks, QPy_MAP);

    return (t & QPy_LONG) && (ks <= as) ? as : (as + ks);
}

QPy_INLINE(int) QPy_PyDictAsDict(QPyDictObject *self, QPy_PyObject arg)
{
    QPy_PyObject key, value;
    QPy_ssize_t  pos = 0, err = 0;

    if (NULL == arg)
	return 0;

    while (err == 0 && PyDict_Next(arg, &pos, &key, &value))
	{
	    Py_INCREF(key);
	    Py_INCREF(value);
	    err = QPy_insert(self, key, value, NULL);
	}
    if (err)
	{
	    Py_DECREF(key);
	    Py_DECREF(value);
	    return QPy_Err;
	}
    return 0;
}

QPy_PTR_INLINE(int) QPy_FromPairs_MapAsDict(QPyDictObject *self, QPy_PyObject arg)
{
    QPy_PyObject _items = PyMapping_Items(arg);

    if (NULL == _items)
	return QPy_Err;

    QPy_PyObject  key   = NULL, value = NULL;
    QPy_PyObject *items = PySequence_Fast_ITEMS(_items);
    QPy_ssize_t   sz    = PySequence_Fast_GET_SIZE(_items);
    QPy_ssize_t   err   = 0;

    for (QPy_ssize_t pos = 0; err == 0 && (pos < sz); pos++)
	{
	    QPy_PyObject *pair = PySequence_Fast_ITEMS(items[pos]);

	    key   = pair[0];
	    value = pair[1];
	    Py_INCREF(key);
	    Py_INCREF(value);
	    err = QPy_insert(self, key, value, NULL);
	}
     if (err)
	{
	    Py_DECREF(key);
	    Py_DECREF(value);
	    Py_DECREF(_items);
	    return QPy_Err;
	}
     Py_DECREF(_items);
     return 0;
}

QPy_PTR_INLINE(int) QPy_FromKeys_MapAsDict(QPyDictObject *self, QPy_PyObject arg)
{
    QPy_PyObject _keys = PyMapping_Keys(arg);

    if (NULL == _items)
	return QPy_Err;

    QPy_PyObject  key   = NULL, value = NULL;
    QPy_PyObject *items = PySequence_Fast_ITEMS(_keys);
    QPy_ssize_t   sz    = PySequence_Fast_GET_SIZE(_keys);
    QPy_ssize_t   err   = 0;

    for (QPy_ssize_t pos = 0; err == 0 && (pos < sz); pos++)
	{
	    key   = items[pos];
	    value = PyObject_GetItem(arg, key);
	    Py_INCREF(key);
	    err = QPy_insert(self, key, value, NULL);
	}
    if (err)
	{
	    Py_DECREF(key);
	    Py_DECREF(value);
	    Py_DECREF(_keys);
	    return QPy_Err;
	}
    Py_DECREF(_keys);
    return 0;
}

QPy_INLINE(int) QPy_MapAsDict(QPyDictObject *self, QPy_PyObject arg)
{
    QPy_ssize_t err = QPy_FromPairs_MapAsDict(self, arg);

    if (err < 0 && PyErr_ExceptionMatches(PyExc_MemoryError))
	{
	    // fallback: try only keys
	    return QPy_FromKeys_MapAsDict(self, arg);
	}
    return err;
}

int QPyDict_IterAsDict(QPyDictObject *self, QPy_PyObject arg)
{
    QPy_PyObject iter, item = NULL;

    iter = PyObject_GetIter(arg);
    if (iter == NULL)
	return QPy_Err;

    QPy_PyObject key, value;
    Py_ssize_t   err  = 0;

    while (err == 0 && QPy_ITERNEXT(iter, &item))
	{
	    QPy_PyObject pair = PySequence_Fast(item, "");

	    if (QPy_LIKELY(pair && PySequence_Fast_GET_SIZE(pair) == 2))
		{
		    key   = PySequence_Fast_GET_ITEM(pair, 0);
		    value = PySequence_Fast_GET_ITEM(pair, 1);
		    Py_INCREF(key);
		    Py_INCREF(value);
		    err   = QPy_insert(self, key, value, NULL);

		    Py_DECREF(pair);
		    Py_DECREF(item);
		    item = NULL;
		    continue;
		}
	    // TODO: Handle error
	    goto error;
	}
    Py_DECREF(iter);
    return 0;

error:
    Py_DECREF(iter);
    return 0;
}
QPy_INLINE(int) QPy_UpdateDict_FromArgKwargs(QPyDictObject *self, QPy_PyObject arg, QPy_PyObject kwargs)
{
    int err = 0;

    // fast path if argument is of dict type
    if (PyDict_CheckExact(arg))
	err = QPy_PyDictAsDict(self, arg);
    else if (QPy_MappingCheck(arg))
	err = QPy_MapAsDict(self, arg);
    else
	err = QPyDict_IterAsDict(self, arg); // fallback! slow iteration over arg (we treat arg as an iterator)

    if (err || QPy_PyDictAsDict(self, kwargs) < 0)
	return QPy_Err;
    return err;
}

static QPy_PyObject QPyDict_new(PyTypeObject *cls, QPy_PyObject QPy_UNUSED(args), QPy_PyObject QPy_UNUSED(kwds))
{
    QPyDictObject *self = (QPyDictObject *)(cls->tp_alloc(cls, 0));
    return QPy_ClearObject(self);
}

static int QPyDict_init(QPy_PyObject _self, QPy_PyObject arg, QPy_PyObject kwargs)
{
    QPy_PyObject   pos_arg;
    QPyDictObject *self;
    QPy_ssize_t    size;

    if (QPy_TUPLE_GETITEM(arg, pos_arg, 0))
	PyErr_Clear();
    if (NULL == pos_arg && NULL == kwargs)
	return 0;

    // Allocate memory for entries
    size = QPy_GetSizeFromArgKwargs(pos_arg, kwargs);
    self = (QPyDictObject *)_self;
    if (QPy_CustomInit(self, size) < 0)
	    return QPy_Err;

    // Insert entries into dict
    if (QPy_UpdateDict_FromArgKwargs(self, pos_arg, kwargs))
	{
	    // error! Deep clean dict
	    QPy_ClearEntries(self);
	    return QPy_Err;
	}
    return 0;
}

static int QPyDict_traverse(QPy_PyObject _self, visitproc visit, void *arg)
{
    // allow class to be tracked by GC (preventing cyclic references)
    Py_VISIT(Py_TYPE(_self));

    return 0;
}

static void QPyDict_dealloc(QPy_PyObject _self)
{
    QPyDictObject *self = (QPyDictObject *)_self;
    PyTypeObject  *cls  = Py_TYPE(self);

    // untrack from the Garbage Collector
    PyObject_GC_UnTrack(cls);

    // Deep clean class object
    QPy_ClearEntries(self);
    QPy_free(QPy_ENTRIES(self));
    QPy_free(QPy_CACHE(self));
    QPy_ClearObject(self);
    cls->tp_free(self);
}
