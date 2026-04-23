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
#define QPy_ITERNEXT(iter, arg) !(arg = PyIter_Next(iter)) // TODO: use PyIter_NextItem for python>=3.14
#define QPy_TUPLE_GETITEM(tuple, item, i) !(item = PyTuple_GetItem(tuple, i))


#define QPyDict_CACHE(self)   ((self)->cache)
#define QPyDict_ENTRIES(self) ((self)->entries)
#define QPyDict_SIZE(self)    ((self)->nentries)
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

__attribute__((unused)) static void * QPyDict_aligned_malloc(QPy_ssize_t QPy_UNUSED(size), void *QPy_UNUSED(ptr))
{
    return NULL;
}

static void QPyDict_free(void *ptr)
{
    free(ptr);
}

__attribute__((unused)) static void QPyDict_aligned_free(void * QPy_UNUSED(ptr))
{
}

QPy_PTR_INLINE(int) QPyDict_GetCommonObjectSize(QPyDict_PyObject arg, QPy_ssize_t *size, int op)
{
    if ((op & QPy_LONG) && PyLong_Check(arg))
	{
	    QPy_ssize_t _size = PyLong_AsSsize_t(arg);
	    return _size < 0 ? QPy_Err : (QPy_SETVAL(*size, _size), QPy_LONG);
	}
    QPy_SETVAL(*size, PyObject_LengthHint(arg, QPy_DEFAULT_SIZE));
    return (QPy_ALL & ~QPy_LONG);
}

QPy_INLINE(void *) QPyDict_ClearObject(QPyDictObject *self)
{
    if (self)
	{
	    memset(self + sizeof(PyObject), 0, sizeof (QPyDictObject) - sizeof (PyObject));
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
	    QPyDict_Array ar_ = NULL; QPyDict_Cache ch_ = NULL;

	    if (!QPyDict_malloc(QPy_ARRAY_SIZE * size, &ar_) || !QPyDict_malloc(QPy_CACHE_SIZE * size, &ch_))
		{
		    QPyDict_free(ar_);
		    QPyDict_free(ch_);
		    return QPy_SETEXC(PyExc_MemoryError, "Out of Memory@call:internal:__init__");
		}
	    QPyDict_ENTRIES(self) = ar_;
	    QPyDict_CACHE(self)   = ch_;
	    QPyDict_SIZE(self)    = size;
	    QPyDict_GSIZE(self)   =  0;
	}
    return 0;
}

QPy_INLINE(int) QPyDict_MappingCheck(const QPyDict_PyObject arg)
{
    return PyObject_HasAttrString(arg, "keys");
}

QPy_INLINE(int) QPyDict_GetSizeFromArgKwargs(const QPyDict_PyObject restrict arg, const QPyDict_PyObject restrict kwargs)
{
    QPy_ssize_t as = 0, ks = 0, t = 0;

    if (NULL != arg
	&& (t = QPyDict_GetCommonObjectSize(arg, &as, QPy_ALL)) < 0)
	    return QPy_Err;
    if (NULL != kwargs)
	QPyDict_GetCommonObjectSize(arg, &ks, QPy_MAP);

    return (t & QPy_LONG) && (ks <= as) ? as : (as + ks);
}

QPy_INLINE(int) QPyDict_IterAsDict(QPyDictObject *self, QPyDict_PyObject arg)
{
    QPyDict_PyObject iter, items, key, value, exc;
    QPy_ssize_t err = 0;

    iter = PyObject_GetIter(arg);
    if (NULL == iter)
	return QPy_Err;

    exc = PyErr_GetRaisedException();
    key = value = NULL;
    while (!err && !QPy_ITERNEXT(iter, items))
	{
	    key  = value = NULL; // this is not redundant
	    err  = QPy_ITERNEXT(items, key) || QPy_ITERNEXT(items, value);
	    err  = err                      || QPyDict_insert(self, key, value, NULL);
	    Py_DECREF(items);
	}

    if (err || PyErr_Occurred())
	{
	    Py_XDECREF(key);
	    Py_XDECREF(value);
	    Py_DECREF(exc);
	    return QPy_Err;
	}
    PyErr_SetRaisedException(exc);
    return 0;
}

QPy_INLINE(int) QPyDict_PyDictAsDict(QPyDictObject *self, QPyDict_PyObject arg)
{
    QPyDict_PyObject key, value;
    QPy_ssize_t      pos = 0, err = 0;

    while (!err && PyDict_Next(arg, &pos, &key, &value))
	{
	    Py_INCREF(key);
	    Py_INCREF(value);
	    err = QPyDict_insert(self, key, value, NULL);
	}
    if (err)
	{
	    Py_DECREF(key);
	    Py_DECREF(value);
	    // the calling function should cleanup inserted entries
	    return QPy_Err;
	}
    return 0;
}

QPy_PTR_INLINE(int) QPyDict_FromPairsMapAsDict(QPyDictObject *self, QPyDict_PyObject arg)
{
    QPyDict_PyObject _items = PyMapping_Items(arg);

    if (NULL == _items)
	return QPy_Err;

    QPyDict_PyObject  key   = NULL, value = NULL;
    QPyDict_PyObject *items = PySequence_Fast_ITEMS(_items);
    QPy_ssize_t sz          = PySequence_Fast_GET_SIZE(_items);
    QPy_ssize_t err         = 0;

    for (QPy_ssize_t pos = 0; !err && (pos < sz); pos++)
	{
	    QPyDict_PyObject *pair = PySequence_Fast_ITEMS(items[pos]);

	    key   = pair[0];
	    value = pair[1];
	    Py_INCREF(key);
	    Py_INCREF(value);
	    err = QPyDict_insert(self, key, value, NULL);
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

QPy_PTR_INLINE(int) QPyDict_FromKeysMapAsDict(QPyDictObject *self, QPyDict_PyObject arg)
{
    QPyDict_PyObject _items = PyMapping_Items(arg);

    if (NULL == _items)
	return QPy_Err;

    QPyDict_PyObject  key   = NULL, value = NULL;
    QPyDict_PyObject *items = PySequence_Fast_ITEMS(_items);
    QPy_ssize_t sz          = PySequence_Fast_GET_SIZE(_items);
    QPy_ssize_t err         = 0;

    for (QPy_ssize_t pos = 0; !err && (pos < sz); pos++)
	{
	    key   = items[pos];
	    value = PyObject_GetItem(arg, key);
	    Py_INCREF(key);
	    err   = QPyDict_insert(self, key, value, NULL);
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

QPy_INLINE(int) QPyDict_MapAsDict(QPyDictObject *self, QPyDict_PyObject arg)
{
    QPy_ssize_t err = QPyDict_FromPairsMapAsDict(self, arg);

    if (err < 0 && PyErr_ExceptionMatches(PyExc_MemoryError))
	{
	    // fallback: try only keys
	    return QPyDict_FromKeysMapAsDict(self, arg);
	}
    return err;
}

QPy_INLINE(int) QPyDict_update_dict_fromArgKwargs(QPyDictObject *self, QPyDict_PyObject arg, QPyDict_PyObject kwargs)
{
    int err = 0;

    // fast path if argument is of dict type
    if (PyDict_CheckExact(arg))
	err = QPyDict_PyDictAsDict(self, arg);
    else if (QPyDict_MappingCheck(arg))
	err = QPyDict_MapAsDict(self, arg);
    else
	{
	    // fallback! slow iteration over arg (we treat arg as an iterator)
	    err = QPyDict_IterAsDict(self, arg);
	}
    // keyword entries
    if (err || QPyDict_PyDictAsDict(self, kwargs) < 0)
	return QPy_Err;
    return 0;
}

static QPyDict_PyObject QPyDict_new(PyTypeObject *cls, QPyDict_PyObject QPy_UNUSED(args), QPyDict_PyObject QPy_UNUSED(kwds))
{
    QPyDictObject *self = (void *)(cls->tp_alloc(cls, 0));

    return QPyDict_ClearObject(self);
}

static int QPyDict_init(QPyDict_PyObject _self, QPyDict_PyObject arg, QPyDict_PyObject kwargs)
{
    QPyDict_PyObject pos_arg;
    QPyDictObject   *self;
    QPy_ssize_t      size;

    if (QPy_TUPLE_GETITEM(arg, pos_arg, 0))
	PyErr_Clear();
    if (NULL == pos_arg && NULL == kwargs)
	return 0;

    // Allocate memory for entries
    size = QPyDict_GetSizeFromArgKwargs(pos_arg, kwargs);
    self = (QPyDictObject *)_self;

    if (QPyDict_CustomInit(self, size) < 0)
	return QPy_Err;

    // Insert entries into dict
    if (QPyDict_update_dict_fromArgKwargs(self, pos_arg, kwargs))
	{
	    // error! Deep clean dict
	    QPyDict_ClearEntries(self);
	    return QPy_Err;
	}
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
