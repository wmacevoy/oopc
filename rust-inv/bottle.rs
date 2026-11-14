/*
  Bottle, written in Rust, exporting the C ABI in include/Bottle.h.

  The same three layers as src/Bottle.c:

    1. the implementations: ordinary methods, `impl Bottle`
    2. the slots:           Bottle_vf_*, one line each over a method
    3. the dispatchers:     Bottle_*, which call through the vtable
*/

use core::ffi::c_void;
use core::ptr;

use crate::{free, malloc, Type, Type_cast};

#[repr(C)]
pub struct Bottle {
    /* A raw pointer, not a reference: it points at the start of the vtable of
       the object's own class, which for a derived class is a longer table. */
    pub(crate) vftbl: *const BottleVftbl,
    pub(crate) volume: f64,
}

/* Every slot takes the root type, *mut Bottle, at every level: see the note in
   include/Bottle.h.  A mistyped slot is a compile error here too. */
#[repr(C)]
pub struct BottleVftbl {
    pub type_: &'static Type,
    pub fini: unsafe extern "C" fn(*mut Bottle),
    pub get_volume: unsafe extern "C" fn(*const Bottle) -> f64,
    pub set_volume: unsafe extern "C" fn(*mut Bottle, f64),
}

/* ---- 1. implementations ---- */

impl Bottle {
    pub fn new() -> Bottle {
        Bottle { vftbl: &Bottle_vftbl, volume: 0.0 }
    }

    pub fn get_volume(&self) -> f64 {
        self.volume
    }

    pub fn set_volume(&mut self, value: f64) {
        self.volume = value;
    }
}

/* ---- class constants: immutable statics, so read-only data, as in C ---- */

#[no_mangle]
pub static Bottle_type: Type = Type {
    name: c"Bottle".as_ptr(),
    parent: None,
    size: size_of::<Bottle>(),
};

#[no_mangle]
pub static Bottle_vftbl: BottleVftbl = BottleVftbl {
    type_: &Bottle_type,
    fini: Bottle_vf_fini,
    get_volume: Bottle_vf_getVolume,
    set_volume: Bottle_vf_setVolume,
};

/* ---- 2. slots ---- */

/* Runs Bottle's destructor in place.  Rust has no fini to write: dropping a
   value drops its fields, so a derived class's base part follows by itself. */
#[no_mangle]
pub unsafe extern "C" fn Bottle_vf_fini(me: *mut Bottle) {
    ptr::drop_in_place(me)
}

#[no_mangle]
pub unsafe extern "C" fn Bottle_vf_getVolume(me: *const Bottle) -> f64 {
    (*me).get_volume()
}

#[no_mangle]
pub unsafe extern "C" fn Bottle_vf_setVolume(me: *mut Bottle, value: f64) {
    (*me).set_volume(value)
}

/* ---- 3. dispatchers and lifecycle ---- */

#[no_mangle]
pub unsafe extern "C" fn Bottle_init(me: *mut Bottle) {
    me.write(Bottle::new())
}

#[no_mangle]
pub unsafe extern "C" fn Bottle_fini(me: *mut Bottle) {
    ((*(*me).vftbl).fini)(me)
}

#[no_mangle]
pub unsafe extern "C" fn Bottle_new() -> *mut Bottle {
    let me = malloc(size_of::<Bottle>()) as *mut Bottle;
    if !me.is_null() {
        Bottle_init(me);
    }
    me
}

#[no_mangle]
pub unsafe extern "C" fn Bottle_delete(me: *mut Bottle) {
    if !me.is_null() {
        Bottle_fini(me);
        free(me as *mut c_void);
    }
}

#[no_mangle]
pub unsafe extern "C" fn Bottle_getVolume(me: *const Bottle) -> f64 {
    ((*(*me).vftbl).get_volume)(me)
}

#[no_mangle]
pub unsafe extern "C" fn Bottle_setVolume(me: *mut Bottle, value: f64) {
    ((*(*me).vftbl).set_volume)(me, value)
}

#[no_mangle]
pub unsafe extern "C" fn Bottle_cast(object: *mut c_void) -> *mut Bottle {
    Type_cast(object, &Bottle_type) as *mut Bottle
}

#[no_mangle]
pub extern "C" fn Bottle_getType() -> *const Type {
    &Bottle_type
}
