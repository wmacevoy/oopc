/*
  TEST ONLY: a SizedBottle in Rust over a Bottle it knows only by the C ABI.

  sized_bottle.rs extends the Rust Bottle beside it, and lets the compiler keep
  the prefix and the fini chain.  That cannot extend a Bottle written in C or
  C++.  This file can: like src/SizedBottle.c, it reaches its base only through
  the names include/Bottle.h publishes, so make test-mix can put it over a
  Bottle from any language (REQ10/INV-6).

  It is the C file transcribed, footguns included: the base's fields and slots
  are restated by hand, and fini chains by name.
*/

#![no_std]
#![allow(non_snake_case, non_upper_case_globals)]

use core::ffi::{c_char, c_void};

#[repr(C)]
pub struct Type {
    name: *const c_char,
    parent: *const Type,
    size: usize,
}
unsafe impl Sync for Type {}

/* include/Bottle.h, restated: the base as the C ABI shows it. */
#[repr(C)]
pub struct Bottle {
    vftbl: *const c_void,
    volume: f64,
}

extern "C" {
    static Bottle_type: Type;
    fn Bottle_init(me: *mut Bottle);
    fn Bottle_vf_fini(me: *mut Bottle);
    fn Bottle_vf_getVolume(me: *const Bottle) -> f64;
    fn Bottle_vf_setVolume(me: *mut Bottle, value: f64);
    fn Type_cast(object: *mut c_void, base: *const Type) -> *mut c_void;
    fn malloc(size: usize) -> *mut c_void;
    fn free(memory: *mut c_void);
}

/* include/SizedBottle.h, restated: the base's fields and slots first. */
#[repr(C)]
pub struct SizedBottle {
    vftbl: *const SizedBottleVftbl,
    volume: f64,
    max_volume: f64,
}

#[repr(C)]
pub struct SizedBottleVftbl {
    type_: *const Type,
    fini: unsafe extern "C" fn(*mut Bottle),
    get_volume: unsafe extern "C" fn(*const Bottle) -> f64,
    set_volume: unsafe extern "C" fn(*mut Bottle, f64),
    get_max_volume: unsafe extern "C" fn(*const Bottle) -> f64,
    set_max_volume: unsafe extern "C" fn(*mut Bottle, f64),
}
unsafe impl Sync for SizedBottleVftbl {}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_vf_setVolume(base: *mut Bottle, value: f64) {
    let me = base as *mut SizedBottle;
    let value = if value < 0.0 { 0.0 } else { value };
    let value = if value > (*me).max_volume { (*me).max_volume } else { value };
    Bottle_vf_setVolume(base, value) /* the base's own, whatever language wrote it */
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_vf_getMaxVolume(base: *const Bottle) -> f64 {
    (*(base as *const SizedBottle)).max_volume
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_vf_setMaxVolume(base: *mut Bottle, value: f64) {
    let me = base as *mut SizedBottle;
    let value = if value < 0.0 { 0.0 } else { value };
    (*me).max_volume = value;
    if Bottle_vf_getVolume(base) > value {
        Bottle_vf_setVolume(base, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_vf_fini(base: *mut Bottle) {
    Bottle_vf_fini(base) /* by name, never Bottle_fini: see src/SizedBottle.c */
}

#[no_mangle]
pub static SizedBottle_type: Type = Type {
    name: c"SizedBottle".as_ptr(),
    parent: unsafe { &Bottle_type },
    size: size_of::<SizedBottle>(),
};

#[no_mangle]
pub static SizedBottle_vftbl: SizedBottleVftbl = SizedBottleVftbl {
    type_: &SizedBottle_type,
    fini: SizedBottle_vf_fini,
    get_volume: Bottle_vf_getVolume, /* inherited: the foreign base's slot */
    set_volume: SizedBottle_vf_setVolume,
    get_max_volume: SizedBottle_vf_getMaxVolume,
    set_max_volume: SizedBottle_vf_setMaxVolume,
};

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_init(me: *mut SizedBottle) {
    Bottle_init(me as *mut Bottle);
    (*me).vftbl = &SizedBottle_vftbl;
    (*me).max_volume = 1.0;
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_fini(me: *mut SizedBottle) {
    ((*(*me).vftbl).fini)(me as *mut Bottle)
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
    ((*(*me).vftbl).get_volume)(me as *const Bottle)
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_setVolume(me: *mut SizedBottle, value: f64) {
    ((*(*me).vftbl).set_volume)(me as *mut Bottle, value)
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_getMaxVolume(me: *const SizedBottle) -> f64 {
    ((*(*me).vftbl).get_max_volume)(me as *const Bottle)
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_setMaxVolume(me: *mut SizedBottle, value: f64) {
    ((*(*me).vftbl).set_max_volume)(me as *mut Bottle, value)
}

#[no_mangle]
pub unsafe extern "C" fn SizedBottle_cast(object: *mut c_void) -> *mut SizedBottle {
    Type_cast(object, &SizedBottle_type) as *mut SizedBottle
}

#[no_mangle]
pub extern "C" fn SizedBottle_getType() -> *const Type {
    &SizedBottle_type
}
