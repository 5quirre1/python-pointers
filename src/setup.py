from setuptools import setup, Extension

module = Extension(
    'pointers',
    sources=['pointers.c'],
    include_dirs=[r"C:\Users\squirrel\AppData\Local\Programs\Python\Python313\include"],
    library_dirs=[r"C:\Users\squirrel\AppData\Local\Programs\Python\Python313\libs"],
    libraries=['python313'],
)

setup(
    name='pointers',
    version='1.0',
    ext_modules=[module],
)
