#include "module.h"
#include "include/defs.h"

enum {
    QPy_Err      = -1,
    QPy_LONG     = 0x01,
    QPy_SEQUENCE = 0x02,
    QPy_MAP      = 0x04,
    QPy_ITER     = 0x08,
    QPy_ALL      = 0x0f
};

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

QPy_INLINE(int) QPy_MappingCheck(const PyObject *arg)
{
    return PyObject_HasAttrString(arg, "keys");
}

static int QPy_FormatErrorNote(void *fmt, ...)
{
    PyObject *note, methname;
    va_list vargs;
    // since python >=3.11: Exception classes inherit the new add_note method from the BaseException class
    methname = PyUnicode_FromString("add_note"); // TODO: make a global object
    if (NULL == methname)
	return QPy_Err;

    va_start(vargs, fmt);
    note = PyUnicode_FromFormatV(fmt, vargs);
    va_end(vargs);
    if (NULL == note)
	{
	    Py_DECREF(methname);
	    return QPy_Err;
	}

    PyObject *exc = PyErr_GetRaisedException();
    int err          = PyObject_CallMethodOneArg(exc, methname, note) < 0;

    if (0 != err)
	Py_DECREF(exc);

    PyErr_SetRaisedException(exc);
    Py_DECREF(methname);
    Py_DECREF(note);
    return -err;
}

QPy_INLINE(int) QPy_GetSizeFromArgKwargs(const PyObject *restrict arg, const PyObject *restrict kwargs)
{
    QPy_ssize_t as = 0, ks = 0;

    if (NULL != arg)
	as = PyObject_LengthHint(arg, 32);

    if (NULL != kwargs)
	ks = PyDict_Size(kwargs);

    return ks + as;
}

QPy_INLINE(int) QPy_PyDictAsDict(QPyDictObject *self, PyObject *arg)
{
    PyObject *key, value;
    QPy_ssize_t  pos = 0, err = 0;

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

QPy_PTR_INLINE(int) QPy_FromPairs_MapAsDict(QPyDictObject *self, PyObject *arg)
{
    PyObject *_items = PyMapping_Items(arg);

    if (NULL == _items)
        return QPy_Err;

    PyObject * key   = NULL, value = NULL;
    PyObject **items = PySequence_Fast_ITEMS(_items);
    QPy_ssize_t   sz    = PySequence_Fast_GET_SIZE(_items);
    QPy_ssize_t   err   = 0;

    for (QPy_ssize_t pos = 0; err == 0 && (pos < sz); pos++)
        {
            PyObject **pair = PySequence_Fast_ITEMS(items[pos]);

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
        }
    Py_DECREF(_items);
    return -err;
}

QPy_PTR_INLINE(int) QPy_FromKeys_MapAsDict(QPyDictObject *self, PyObject *arg)
{
    PyObject *_keys = PyMapping_Keys(arg);

    if (NULL == _keys)
        return QPy_Err;

    PyObject * key   = NULL, value = NULL;
    PyObject **items = PySequence_Fast_ITEMS(_keys);
    QPy_ssize_t   sz    = PySequence_Fast_GET_SIZE(_keys);
    QPy_ssize_t   err   = 0, pos = 0;

    for (; err == 0 && (pos < sz); pos++)
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
        }
    Py_DECREF(_keys);
    return -err;
}

QPy_INLINE(int) QPy_MapAsDict(QPyDictObject *self, PyObject *arg)
{
    return QPy_FromKeys_MapAsDict(self, arg);
}

int QPyDict_IterAsDict(QPyDictObject *self, PyObject *arg)
{
    PyObject *iter, item = NULL;
    
    iter = PyObject_GetIter(arg);
    if (iter == NULL)
        return QPy_Err;

    PyObject *key, value;
    Py_ssize_t   err  = 0, i = 0;

    for (; err == 0 && QPy_ITERNEXT(iter, &item); i++)
        {
            PyObject *pair = PySequence_Fast(item, "object is not an iterable");

            if (QPy_LIKELY(pair && PySequence_Fast_GET_SIZE(pair) == 2))
                {
                    key   = PySequence_Fast_GET_ITEM(pair, 0);
                    value = PySequence_Fast_GET_ITEM(pair, 1);
                    Py_INCREF(key);
                    Py_INCREF(value);
                    Py_DECREF(pair);
                    Py_DECREF(item);

                    item = NULL;
                    err  = QPy_insert(self, key, value, NULL);
                    continue;
                }

            if (NULL != pair)
                {
                    PyErr_Format(PyExc_ValueError, "dictionary update sequence element #%zd has length %zd; 2 is required", i, PySequence_Fast_GET_SIZE(pair));
                    Py_DECREF(pair);
                    Py_DECREF(iter);
                    return QPy_Err;
                }
            if (PyErr_ExceptionMatches(PyExc_TypeError))
		QPy_FormatErrorNote("Cannot convert dictionary update sequence element #%zd to a sequence", i);
            Py_DECREF(iter);
            return QPy_Err;
        }

    if (0 != err || NULL != item)
        if (err)
            {
                Py_DECREF(key);
                Py_DECREF(value);
            }
    Py_DECREF(iter);
    return -err;
}

QPy_INLINE(int) QPy_UpdateDict_FromArgKwargs(QPyDictObject *self, PyObject *arg, PyObject *kwargs)
{
    int err = 0;

    if (NULL != arg)
    	{
	    if (PyDict_CheckExact(arg))
	    	err = QPy_PyDictAsDict(self, arg);
	    else if (QPy_MappingCheck(arg))
		err = QPy_MapAsDict(self, arg);
	    else
		err = QPyDict_IterAsDict(self, arg);
	}
    if (NULL != kwargs && 0 == err)
    	err = QPy_PyDictAsDict(self, kwargs);

    return err;
}

static PyObject *QPyDict_new(PyTypeObject *cls, PyObject *QPy_UNUSED(args), PyObject *QPy_UNUSED(kwds))
{
    QPyDictObject *self = (QPyDictObject *)(cls->tp_alloc(cls, 0));
    return QPy_ClearObject(self);
}

static int QPyDict_init(PyObject *_self, PyObject *arg, PyObject *kwargs)
{
    PyObject *  pos_arg;
    QPyDictObject *self;
    QPy_ssize_t    size=0;

    if (QPy_TUPLE_GETITEM(arg, pos_arg, 0))
        PyErr_Clear();
    if (NULL == pos_arg && NULL == kwargs)
        return 0;

    // Allocate memory for entries
    self = (QPyDictObject *)_self;
    size = QPy_GetSizeFromArgKwargs(pos_arg, kwargs);

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

static int QPyDict_traverse(PyObject *_self, visitproc visit, void *arg)
{
    // allow class to be tracked by GC (preventing cyclic references)
    Py_VISIT(Py_TYPE(_self));

    return 0;
}

static void QPyDict_dealloc(PyObject *_self)
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
