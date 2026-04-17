#define Py_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>

/*
[test - 2]
  A simple python module @HelloWorld, with a single callable method @helloworld, that prints "Hello World!" to stdout.

 [build]
      python -m build build_ext --inplace

[import]
      from HelloWorld import helloworld

helloworld() -> "Hello World!"
 */

// helloworld method C func
PyObject *hello_world(PyObject *self)
{
  puts("Hello World!");

  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef helloWorld_Methods[] = {
  {"helloworld", (PyCFunction)hello_world, METH_NOARGS, NULL},
  {NULL, NULL, 0, NULL}
};

static PyModuleDef HelloWorld_Module = {
  PyModuleDef_HEAD_INIT,
  .m_name = "helloworld",
  .m_methods = helloWorld_Methods,
  .m_size = 0
};

// intialize the HelloWorld module
PyMODINIT_FUNC
PyInit_HelloWorld(void)
{
  return PyModuleDef_Init(&HelloWorld_Module);
}
