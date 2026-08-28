"""
Python binding for the oopc single-inheritance C pattern.

There is no extension module and no C glue -- this file drives the shared
library through ctypes.  The C ABI *is* the binding.

The point of the exercise: Python's own single inheritance lines up 1:1 with the
C hierarchy.  SizedBottle below declares only the methods SizedBottle *adds*.
It inherits get_volume/set_volume from Bottle, and those call the C functions
Bottle_getVolume/Bottle_setVolume -- which dispatch through the C vtable and so
land in SizedBottle's clamping implementation.  Python contributes the class
hierarchy, C contributes the virtual dispatch, and neither has to know how the
other does it.
"""

import ctypes
import os


def _load():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    for name in ("liboopc.dylib", "liboopc.so", "oopc.dll"):
        path = os.path.join(root, "bin", name)
        if os.path.exists(path):
            return ctypes.CDLL(path)
    raise OSError("liboopc not found in %s/bin -- run `make` first" % root)


_lib = _load()


class Type(ctypes.Structure):
    """Mirrors struct TypeStruct: one per class, three words, shared."""


Type._fields_ = [
    ("name", ctypes.c_char_p),
    ("parent", ctypes.POINTER(Type)),
    ("size", ctypes.c_size_t),
]

_TYPE_P = ctypes.POINTER(Type)
_PTR = ctypes.c_void_p
_D = ctypes.c_double


def _sig(name, restype, *argtypes):
    fn = getattr(_lib, name)
    fn.restype = restype
    fn.argtypes = list(argtypes)
    return fn


# ctypes defaults every return type to int, which silently truncates pointers
# and returns garbage for doubles -- so every signature is declared.
_sig("Type_of", _TYPE_P, _PTR)
_sig("Type_isa", ctypes.c_int, _TYPE_P, _TYPE_P)

_sig("Bottle_new", _PTR)
_sig("Bottle_delete", None, _PTR)
_sig("Bottle_init", None, _PTR)
_sig("Bottle_fini", None, _PTR)
_sig("Bottle_getVolume", _D, _PTR)
_sig("Bottle_setVolume", None, _PTR, _D)
_sig("Bottle_cast", _PTR, _PTR)
_sig("Bottle_getType", _TYPE_P)

_sig("SizedBottle_new", _PTR)
_sig("SizedBottle_delete", None, _PTR)
_sig("SizedBottle_init", None, _PTR)
_sig("SizedBottle_fini", None, _PTR)
_sig("SizedBottle_getMaxVolume", _D, _PTR)
_sig("SizedBottle_setMaxVolume", None, _PTR, _D)
_sig("SizedBottle_cast", _PTR, _PTR)
_sig("SizedBottle_getType", _TYPE_P)


class Bottle:
    # A subclass names its C class here; everything else is inherited.
    _C = "Bottle"

    def __init__(self, ptr=None, owned=None):
        if ptr is None:
            ptr = getattr(_lib, type(self)._C + "_new")()
            owned = True
        self._ptr = ptr
        self._owned = bool(owned)

    # ---- lifetime -------------------------------------------------------
    # Bottle_delete dispatches fini through the C vtable, so one deleter is
    # correct for every class in the hierarchy.

    def close(self):
        if getattr(self, "_ptr", None) is not None and self._owned:
            _lib.Bottle_delete(self._ptr)
        self._ptr = None

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass  # interpreter teardown can null out module globals

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
        return False

    # ---- runtime type ---------------------------------------------------

    @classmethod
    def c_type(cls):
        """The static Type of this class."""
        return getattr(_lib, cls._C + "_getType")().contents

    @property
    def type_name(self):
        """The DYNAMIC type -- what the object actually is."""
        return _lib.Type_of(self._ptr).contents.name.decode()

    @classmethod
    def cast(cls, obj):
        """Checked downcast. Returns a cls view of obj, or None on mismatch."""
        ptr = getattr(_lib, cls._C + "_cast")(obj._ptr)
        return cls(ptr, owned=False) if ptr else None

    # ---- methods --------------------------------------------------------

    def get_volume(self):
        return _lib.Bottle_getVolume(self._ptr)

    def set_volume(self, value):
        _lib.Bottle_setVolume(self._ptr, value)

    volume = property(get_volume, set_volume)

    def __repr__(self):
        return "<%s volume=%g>" % (self.type_name, self.get_volume())


class SizedBottle(Bottle):
    _C = "SizedBottle"

    # Only the NEW methods. get_volume/set_volume are inherited above and still
    # reach SizedBottle's override, because the dispatch happens in C.

    def get_max_volume(self):
        return _lib.SizedBottle_getMaxVolume(self._ptr)

    def set_max_volume(self, value):
        _lib.SizedBottle_setMaxVolume(self._ptr, value)

    max_volume = property(get_max_volume, set_max_volume)
