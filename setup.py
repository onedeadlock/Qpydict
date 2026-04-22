from setuptools import setup, Extension

add_extension = Extension(
    name           = "Qpydict",
    sources        = ["../src/module.c", "../src/internal/methods.c"],
    include_dirs   = ["../src"],
    py_limited_api = True
)
setup(
    ext_modules = [add_extension]
)
