/*
  SizedBottle, written in Rust, exporting the C ABI in include/SizedBottle.h.

  What src/SizedBottle.c keeps by hand, and who keeps it here:

    - the base's fields first    -- `base: Bottle` is the first field, #[repr(C)]
    - the base's slots first     -- `base: BottleVftbl`, the same way
    - an inherited slot          -- `..Bottle_vftbl` copies what is not overridden
    - the fini chain             -- dropping a SizedBottle drops its `base`
    - Bottle_vf_ vs Bottle_      -- `self.base.set_volume(v)` is a plain call;
                                    the dispatcher is not something a method
                                    reaches by accident, so nothing recurses

  What is still written by hand: the downcast, once, in `me` and `me_mut`.
*/

use core::ffi::c_void;
use core::ptr;

use crate::bottle::{Bottle, BottleVftbl, Bottle_init, Bottle_type, Bottle_vftbl};
use crate::{free, malloc, Type, Type_cast};

#[repr(C)]
pub struct SizedBottle {
    base: Bottle,
    max_volume: f64,
}

#[repr(C)]
pub struct SizedBottleVftbl {
    pub base: BottleVftbl,
    /* new slots, also typed on the root */
    pub get_max_volume: unsafe extern "C" fn(*const Bottle) -> f64,
    pub set_max_volume: unsafe extern "C" fn(*mut Bottle, f64),
}

/* ---- 1. implementations ---- */

impl SizedBottle {
    pub fn get_max_volume(&self) -> f64 {
        self.max_volume
    }

    pub fn set_max_volume(&mut self, value: f64) {
        let value = if value < 0.0 { 0.0 } else { value };
        self.max_volume = value;
        if self.base.volume > value {
            self.base.volume = value; /* keep volume <= max_volume */
        }
    }

    pub fn set_volume(&mut self, value: f64) {
        let value = if value < 0.0 { 0.0 } else { value };
        let value = if value > self.max_volume { self.max_volume } else { value };
        self.base.set_volume(value) /* Bottle's own, not virtual */
    }

    /* The downcast every slot needs, written once.  Sound because a
       SizedBottle is #[repr(C)] and its first field is its Bottle. */
    unsafe fn me<'a>(base: *const Bottle) -> &'a SizedBottle {
        &*(base as *const SizedBottle)
    }

    unsafe fn me_mut<'a>(base: *mut Bottle) -> &'a mut SizedBottle {
        &mut *(base as *mut SizedBottle)
    }

    unsafe fn vftbl(me: *const SizedBottle) -> &'static SizedBottleVftbl {
        &*((*me).base.vftbl as *const SizedBottleVftbl)
    }
}

/* ---- class constants ---- */

#[no_mangle]
pub static SizedBottle_type: Type = Type {
    name: c"SizedBottle".as_ptr(),
    parent: Some(&Bottle_type),
    size: size_of::<SizedBottle>(),
};

#[no_mangle]
pub static SizedBottle_vftbl: SizedBottleVftbl = SizedBottleVftbl {
    base: BottleVftbl {
        type_: &SizedBottle_type,
        fini: SizedBottle_vf_fini,
        set_volume: SizedBottle_vf_setVolume,
        ..Bottle_vftbl /* getVolume is inherited */
    },
    get_max_volume: SizedBottle_vf_getMaxVolume,
    set_max_volume: SizedBottle_vf_setMaxVolume,
};

/* ---- 2. slots ---- */

/* Drops the SizedBottle, and with it the Bottle inside: the whole chain. */
#[no_mangle]
pub unsafe extern "C" fn SizedBottle_vf_fini(base: *mut Bottle) {
    ptr::drop_in_place(base as *mut SizedBottle)
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_vf_setVolume(base: *mut Bottle, value: f64) {
    SizedBottle::me_mut(base).set_volume(value)
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_vf_getMaxVolume(base: *const Bottle) -> f64 {
    SizedBottle::me(base).get_max_volume()
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_vf_setMaxVolume(base: *mut Bottle, value: f64) {
    SizedBottle::me_mut(base).set_max_volume(value)
}

/* ---- 3. dispatchers and lifecycle ---- */

/* As in C: the base's init first, then the derived table over it. */
#[no_mangle]
pub unsafe extern "C" fn SizedBottle_init(me: *mut SizedBottle) {
    Bottle_init(me as *mut Bottle);
    (*me).base.vftbl = ptr::from_ref(&SizedBottle_vftbl) as *const BottleVftbl;
    (*me).max_volume = 1.0;
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_fini(me: *mut SizedBottle) {
    (SizedBottle::vftbl(me).base.fini)(me as *mut Bottle)
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_new() -> *mut SizedBottle {
    let me = malloc(size_of::<SizedBottle>()) as *mut SizedBottle;
    if !me.is_null() {
        SizedBottle_init(me);
    }
    me
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_delete(me: *mut SizedBottle) {
    if !me.is_null() {
        SizedBottle_fini(me);
        free(me as *mut c_void);
    }
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_getVolume(me: *const SizedBottle) -> f64 {
    (SizedBottle::vftbl(me).base.get_volume)(me as *const Bottle)
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_setVolume(me: *mut SizedBottle, value: f64) {
    (SizedBottle::vftbl(me).base.set_volume)(me as *mut Bottle, value)
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_getMaxVolume(me: *const SizedBottle) -> f64 {
    (SizedBottle::vftbl(me).get_max_volume)(me as *const Bottle)
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_setMaxVolume(me: *mut SizedBottle, value: f64) {
    (SizedBottle::vftbl(me).set_max_volume)(me as *mut Bottle, value)
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_cast(object: *mut c_void) -> *mut SizedBottle {
    Type_cast(object, &SizedBottle_type) as *mut SizedBottle
}

#[no_mangle]
pub extern "C" fn SizedBottle_getType() -> *const Type {
    &SizedBottle_type
}
