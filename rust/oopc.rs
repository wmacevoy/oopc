/*
  Rust over the C ABI.  Like python/oopc.py and js/oopc.mjs, this file is the
  whole binding: there is no per-class C code, no build script and no crate.

  Rust has no inheritance, so the hierarchy is spelled with two ordinary Rust
  idioms:

    - A class is a VIEW type that owns no bytes.  `&Bottle` is the C `Bottle *`,
      nothing more.  `SizedBottle` derefs to `Bottle`, which is the upcast: a
      plain pointer conversion, exactly as in C.  A derived class declares only
      what it adds; the inherited methods call the BASE C functions, and the C
      vtable picks the override.

    - `Owned<T>` owns an object C allocated.  It is one pointer, and dropping it
      calls Bottle_delete, which dispatches through the C vtable.  So, unlike
      the C++ wrapper, an `Owned<Bottle>` that holds a SizedBottle is finalized
      correctly.

  WHY A VIEW OWNS NO BYTES.  If `Bottle` were a 16-byte Rust struct, safe code
  holding two `&mut Bottle` could `std::mem::swap` them.  With one of them
  really a SizedBottle, that swaps two vtable pointers between objects of
  different sizes: slicing, in safe Rust.  A zero-sized view makes that swap
  move nothing.  rust/test_oopc.rs checks it (REQ7/RS-3).
*/

#![allow(non_snake_case)] /* the extern names below are the C ABI's */

use std::ffi::{c_char, c_void, CStr};
use std::marker::{PhantomData, PhantomPinned};
use std::ops::{Deref, DerefMut};
use std::ptr::NonNull;

/* The C ABI, restated by hand as python/oopc.py restates it for ctypes.
   Nothing checks these against the headers in include/: a wrong signature
   here is the Rust form of "the C ABI trusts its callers". */
extern "C" {
    fn Type_of(object: *const c_void) -> *const c_void;
    fn Type_name(type_: *const c_void) -> *const c_char;

    fn Bottle_new() -> *mut Bottle;
    fn Bottle_delete(me: *mut Bottle);
    fn Bottle_getVolume(me: *const Bottle) -> f64;
    fn Bottle_setVolume(me: *mut Bottle, value: f64);

    fn SizedBottle_new() -> *mut SizedBottle;
    fn SizedBottle_getMaxVolume(me: *const SizedBottle) -> f64;
    fn SizedBottle_setMaxVolume(me: *mut SizedBottle, value: f64);
    fn SizedBottle_cast(object: *mut c_void) -> *mut SizedBottle;
}

/* ---- Bottle: the root class ---- */

/* No bytes of its own.  The marker makes it not Send, Sync or Unpin: the object
   behind the pointer belongs to C, and nothing here promises it is thread-safe
   or movable. */
#[repr(C)]
pub struct Bottle {
    _bytes: [u8; 0],
    _c_owns_it: PhantomData<(*mut u8, PhantomPinned)>,
}

impl Bottle {
    pub fn new() -> Owned<Bottle> {
        unsafe { Owned::from_new(Bottle_new()) }
    }

    pub fn get_volume(&self) -> f64 {
        unsafe { Bottle_getVolume(self) }
    }

    pub fn set_volume(&mut self, value: f64) {
        unsafe { Bottle_setVolume(self, value) }
    }

    /* The dynamic class name, read through the vtable (REQ2/TY-1). */
    pub fn type_name(&self) -> &'static str {
        let name = unsafe { CStr::from_ptr(Type_name(Type_of(self as *const Bottle as *const c_void))) };
        name.to_str().unwrap_or("?")
    }
}

/* ---- SizedBottle: declares only what it adds ---- */

#[repr(C)]
pub struct SizedBottle {
    _bytes: [u8; 0],
    _c_owns_it: PhantomData<(*mut u8, PhantomPinned)>,
}

impl SizedBottle {
    pub fn new() -> Owned<SizedBottle> {
        unsafe { Owned::from_new(SizedBottle_new()) }
    }

    pub fn get_max_volume(&self) -> f64 {
        unsafe { SizedBottle_getMaxVolume(self) }
    }

    pub fn set_max_volume(&mut self, value: f64) {
        unsafe { SizedBottle_setMaxVolume(self, value) }
    }

    /* The checked downcast (REQ2/TY-2): None if `bottle` is not a SizedBottle. */
    pub fn cast(bottle: &mut Bottle) -> Option<&mut SizedBottle> {
        unsafe { SizedBottle_cast(bottle as *mut Bottle as *mut c_void).as_mut() }
    }
}

/* The upcast.  A SizedBottle starts with a Bottle, so its address is a
   Bottle's address. */
impl Deref for SizedBottle {
    type Target = Bottle;
    fn deref(&self) -> &Bottle {
        unsafe { &*(self as *const SizedBottle as *const Bottle) }
    }
}

impl DerefMut for SizedBottle {
    fn deref_mut(&mut self) -> &mut Bottle {
        unsafe { &mut *(self as *mut SizedBottle as *mut Bottle) }
    }
}

impl From<Owned<SizedBottle>> for Owned<Bottle> {
    fn from(sized: Owned<SizedBottle>) -> Owned<Bottle> {
        let me = sized.me.cast();
        std::mem::forget(sized); /* the new owner deletes it, once */
        Owned { me }
    }
}

/* ---- Owned<T>: one pointer to an object C allocated ---- */

pub struct Owned<T> {
    me: NonNull<T>,
}

impl<T> Owned<T> {
    /* `me` is what Class_new returned.  Private, so every T here is a class in
       the Bottle hierarchy.  Null means malloc failed; there is no object to
       give back, so this panics rather than hand out a dangling owner. */
    unsafe fn from_new(me: *mut T) -> Owned<T> {
        Owned { me: NonNull::new(me).expect("Class_new returned null: out of memory") }
    }
}

impl<T> Deref for Owned<T> {
    type Target = T;
    fn deref(&self) -> &T {
        unsafe { self.me.as_ref() }
    }
}

impl<T> DerefMut for Owned<T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { self.me.as_mut() }
    }
}

impl<T> Drop for Owned<T> {
    fn drop(&mut self) {
        /* Virtual: runs the most-derived fini, then frees (REQ1/OM-5). */
        unsafe { Bottle_delete(self.me.as_ptr().cast()) }
    }
}
