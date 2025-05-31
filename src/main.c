#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  PyObject_HEAD void *address;
  PyObject *value;
  int size;
  int owns_memory;
} PointerObject;

static PyObject *Pointer_new(PyTypeObject *type, PyObject *args,
                             PyObject *kwds);
static int Pointer_init(PointerObject *self, PyObject *args, PyObject *kwds);
static void Pointer_dealloc(PointerObject *self);
static PyObject *Pointer_dereference(PointerObject *self, PyObject *args);
static PyObject *Pointer_assign(PointerObject *self, PyObject *args);
static PyObject *Pointer_address(PointerObject *self, PyObject *args);
static PyObject *Pointer_arithmetic(PointerObject *self, PyObject *args);
static PyObject *Pointer_get_value_address(PointerObject *self, PyObject *args);
static PyObject *Pointer_from_address(PyObject *cls, PyObject *args);
static PyObject *Pointer_malloc(PyObject *cls, PyObject *args);
static PyObject *Pointer_free(PointerObject *self, PyObject *args);
static PyObject *Pointer_read_bytes(PointerObject *self, PyObject *args);
static PyObject *Pointer_write_bytes(PointerObject *self, PyObject *args);
static PyObject *Pointer_read_int(PointerObject *self, PyObject *args);
static PyObject *Pointer_write_int(PointerObject *self, PyObject *args);
static PyObject *Pointer_cast(PointerObject *self, PyObject *args);
static PyObject *Pointer_get_size(PointerObject *self, PyObject *args);
static PyObject *Pointer_get_size_attr(PointerObject *self, void *closure);

static PyMethodDef Pointer_methods[] = {
    {"dereference", (PyCFunction)Pointer_dereference, METH_NOARGS,
     "dereference the pointer (returns Python object)"},
    {"assign", (PyCFunction)Pointer_assign, METH_VARARGS,
     "assign a Python object to the pointer"},
    {"address", (PyCFunction)Pointer_address, METH_NOARGS,
     "get the pointer's own address"},
    {"value_address", (PyCFunction)Pointer_get_value_address, METH_NOARGS,
     "get the address this pointer points to"},
    {"add", (PyCFunction)Pointer_arithmetic, METH_VARARGS,
     "pointer arithmetic (returns new pointer)"},
    {"free", (PyCFunction)Pointer_free, METH_NOARGS,
     "free malloc'd memory (if owned by this pointer)"},
    {"read_bytes", (PyCFunction)Pointer_read_bytes, METH_VARARGS,
     "read raw bytes from memory address"},
    {"write_bytes", (PyCFunction)Pointer_write_bytes, METH_VARARGS,
     "write raw bytes to memory address"},
    {"read_int", (PyCFunction)Pointer_read_int, METH_VARARGS,
     "read integer from memory address"},
    {"write_int", (PyCFunction)Pointer_write_int, METH_VARARGS,
     "write integer to memory address"},
    {"cast", (PyCFunction)Pointer_cast, METH_VARARGS,
     "cast pointer to different type/size"},
    {"from_address", (PyCFunction)Pointer_from_address,
     METH_VARARGS | METH_CLASS, "create pointer from raw memory address"},
    {"malloc", (PyCFunction)Pointer_malloc, METH_VARARGS | METH_CLASS,
     "allocate memory and return pointer"},
    {"get_size", (PyCFunction)Pointer_get_size, METH_NOARGS,
     "Get the size of the pointer's data type"},

    {NULL}};

static PyMethodDef module_methods[] = {{NULL}};

static PyGetSetDef Pointer_getsetters[] = {
    {"size", (getter)Pointer_get_size_attr, NULL,
     "size of the pointer's data type", NULL},
    {NULL}};

static PyTypeObject PointerType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pointers.Pointer",
    .tp_doc = "low-level pointer objects with real memory addresses",
    .tp_basicsize = sizeof(PointerObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Pointer_new,
    .tp_init = (initproc)Pointer_init,
    .tp_dealloc = (destructor)Pointer_dealloc,
    .tp_methods = Pointer_methods,
    .tp_getset = Pointer_getsetters,
};

static PyModuleDef pointersmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "pointers",
    .m_doc = "provides real pointer operations with memory management",
    .m_size = -1,
    .m_methods = module_methods,
};

PyMODINIT_FUNC PyInit_pointers(void) {
  PyObject *m;

  if (PyType_Ready(&PointerType) < 0) return NULL;

  m = PyModule_Create(&pointersmodule);
  if (m == NULL) return NULL;

  Py_INCREF(&PointerType);
  if (PyModule_AddObject(m, "Pointer", (PyObject *)&PointerType) < 0) {
    Py_DECREF(&PointerType);
    Py_DECREF(m);
    return NULL;
  }

  return m;
}

static PyObject *Pointer_new(PyTypeObject *type, PyObject *args,
                             PyObject *kwds) {
  PointerObject *self;
  self = (PointerObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->address = NULL;
    self->value = NULL;
    self->size = sizeof(void *);
    self->owns_memory = 0;
  }
  return (PyObject *)self;
}

static int Pointer_init(PointerObject *self, PyObject *args, PyObject *kwds) {
  PyObject *value = NULL;
  int size = sizeof(void *);

  static char *kwlist[] = {"value", "size", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &value, &size)) {
    return -1;
  }

  self->size = size;

  if (value != NULL) {
    Py_XDECREF(self->value);
    Py_INCREF(value);
    self->value = value;

    self->address = (void *)value;
  }

  return 0;
}

static void Pointer_dealloc(PointerObject *self) {
  if (self->owns_memory && self->address) {
    free(self->address);
  }

  Py_XDECREF(self->value);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *Pointer_get_size_attr(PointerObject *self, void *closure) {
  return PyLong_FromLong(self->size);
}

static PyObject *Pointer_dereference(PointerObject *self, PyObject *args) {
  if (self->value == NULL) {
    PyErr_SetString(PyExc_ValueError,
                    "pointer has no associated Python object");
    return NULL;
  }
  Py_INCREF(self->value);
  return self->value;
}

static PyObject *Pointer_assign(PointerObject *self, PyObject *args) {
  PyObject *value;

  if (!PyArg_ParseTuple(args, "O", &value)) return NULL;

  Py_XDECREF(self->value);
  Py_INCREF(value);
  self->value = value;
  self->address = (void *)value;

  Py_RETURN_NONE;
}

static PyObject *Pointer_address(PointerObject *self, PyObject *args) {
  return PyLong_FromVoidPtr((void *)self);
}

static PyObject *Pointer_get_value_address(PointerObject *self,
                                           PyObject *args) {
  if (self->address == NULL) {
    Py_RETURN_NONE;
  }
  return PyLong_FromVoidPtr(self->address);
}

static PyObject *Pointer_arithmetic(PointerObject *self, PyObject *args) {
  int offset;

  if (!PyArg_ParseTuple(args, "i", &offset)) return NULL;

  if (self->address == NULL) {
    PyErr_SetString(PyExc_ValueError, "pointer address is NULL");
    return NULL;
  }

  PointerObject *new_pointer =
      (PointerObject *)Pointer_new(&PointerType, NULL, NULL);
  if (new_pointer == NULL) return NULL;

  new_pointer->address = (char *)self->address + (offset * self->size);
  new_pointer->size = self->size;
  new_pointer->owns_memory = 0;
  new_pointer->value = NULL;

  return (PyObject *)new_pointer;
}

static PyObject *Pointer_from_address(PyObject *cls, PyObject *args) {
  unsigned long long addr;
  int size = sizeof(void *);

  if (!PyArg_ParseTuple(args, "K|i", &addr, &size)) return NULL;

  PyTypeObject *type = (PyTypeObject *)cls;
  PointerObject *self = (PointerObject *)Pointer_new(type, NULL, NULL);
  if (self == NULL) return NULL;

  self->address = (void *)addr;
  self->size = size;
  self->owns_memory = 0;
  self->value = NULL;

  return (PyObject *)self;
}

static PyObject *Pointer_malloc(PyObject *cls, PyObject *args) {
  int size;

  if (!PyArg_ParseTuple(args, "i", &size)) return NULL;

  if (size <= 0) {
    PyErr_SetString(PyExc_ValueError, "size must be positive");
    return NULL;
  }

  void *mem = malloc(size);
  if (mem == NULL) {
    PyErr_SetString(PyExc_MemoryError, "failed to allocate memory");
    return NULL;
  }

  PyTypeObject *type = (PyTypeObject *)cls;
  PointerObject *self = (PointerObject *)Pointer_new(type, NULL, NULL);
  if (self == NULL) {
    free(mem);
    return NULL;
  }

  self->address = mem;
  self->size = size;
  self->owns_memory = 1;
  self->value = NULL;

  memset(mem, 0, size);

  return (PyObject *)self;
}

static PyObject *Pointer_free(PointerObject *self, PyObject *args) {
  if (self->owns_memory && self->address) {
    free(self->address);
    self->address = NULL;
    self->owns_memory = 0;
  } else {
    PyErr_SetString(PyExc_ValueError,
                    "cannot free memory not owned by this pointer");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *Pointer_read_bytes(PointerObject *self, PyObject *args) {
  int count = 1;

  if (!PyArg_ParseTuple(args, "|i", &count)) return NULL;

  if (self->address == NULL) {
    PyErr_SetString(PyExc_ValueError, "pointer address is NULL");
    return NULL;
  }

  if (count <= 0) {
    PyErr_SetString(PyExc_ValueError, "count must be positive");
    return NULL;
  }

  return PyBytes_FromStringAndSize((char *)self->address, count);
}

static PyObject *Pointer_write_bytes(PointerObject *self, PyObject *args) {
  Py_buffer buffer;

  if (!PyArg_ParseTuple(args, "y*", &buffer)) return NULL;

  if (self->address == NULL) {
    PyBuffer_Release(&buffer);
    PyErr_SetString(PyExc_ValueError, "pointer address is NULL");
    return NULL;
  }

  memcpy(self->address, buffer.buf, buffer.len);
  PyBuffer_Release(&buffer);

  Py_RETURN_NONE;
}

static PyObject *Pointer_read_int(PointerObject *self, PyObject *args) {
  int byte_size = 4;

  if (!PyArg_ParseTuple(args, "|i", &byte_size)) return NULL;

  if (self->address == NULL) {
    PyErr_SetString(PyExc_ValueError, "pointer address is NULL");
    return NULL;
  }

  switch (byte_size) {
    case 1:
      return PyLong_FromLong(*(char *)self->address);
    case 2:
      return PyLong_FromLong(*(short *)self->address);
    case 4:
      return PyLong_FromLong(*(int *)self->address);
    case 8:
      return PyLong_FromLongLong(*(long long *)self->address);
    default:
      PyErr_SetString(PyExc_ValueError,
                      "invalid byte size (must be 1, 2, 4, or 8)");
      return NULL;
  }
}

static PyObject *Pointer_get_size(PointerObject *self, PyObject *args) {
  return PyLong_FromLong(self->size);
}

static PyObject *Pointer_write_int(PointerObject *self, PyObject *args) {
  long long value;
  int byte_size = 4;

  if (!PyArg_ParseTuple(args, "L|i", &value, &byte_size)) return NULL;

  if (self->address == NULL) {
    PyErr_SetString(PyExc_ValueError, "pointer address is NULL");
    return NULL;
  }

  switch (byte_size) {
    case 1:
      *(char *)self->address = (char)value;
      break;
    case 2:
      *(short *)self->address = (short)value;
      break;
    case 4:
      *(int *)self->address = (int)value;
      break;
    case 8:
      *(long long *)self->address = value;
      break;
    default:
      PyErr_SetString(PyExc_ValueError,
                      "invalid byte size (must be 1, 2, 4, or 8)");
      return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *Pointer_cast(PointerObject *self, PyObject *args) {
  int new_size;

  if (!PyArg_ParseTuple(args, "i", &new_size)) return NULL;

  if (new_size <= 0) {
    PyErr_SetString(PyExc_ValueError, "size must be positive");
    return NULL;
  }

  PointerObject *new_pointer =
      (PointerObject *)Pointer_new(&PointerType, NULL, NULL);
  if (new_pointer == NULL) return NULL;

  new_pointer->address = self->address;
  new_pointer->size = new_size;
  new_pointer->owns_memory = 0;
  new_pointer->value = NULL;

  return (PyObject *)new_pointer;
}
