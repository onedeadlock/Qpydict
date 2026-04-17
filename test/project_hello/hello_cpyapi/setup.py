from setuptools import setup, Extension

add_extension = Extension(
    name    = "HelloWorld",
    sources = ["hello_world.c"],
)
setup(
    ext_modules = [add_extension],
)
