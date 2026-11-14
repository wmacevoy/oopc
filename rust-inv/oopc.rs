/*
  rust-inv: Rust is the source, the C ABI is the artifact.

  bottle.rs and sized_bottle.rs define the same symbols, with the same layout,
  as src/Bottle.c and src/SizedBottle.c.  Read each next to its C file.  The
  unmodified C demo, C suite and Python binding run against this build
  (REQ7/INV-5).

  No std, no crates, no runtime: the build emits one object file, linked like
  any C object.  Type.c and Trace.c are not rewritten; the Rust classes use
  src/Type.c through the three declarations below.
*/

#![no_std]
#![allow(non_snake_case, non_upper_case_globals)] /* exported names are the C ABI's */

use core::ffi::{c_char, c_void};

pub mod bottle;
pub mod sized_bottle;

/* include/Type.h's struct TypeStruct. */
#[repr(C)]
pub struct Type {
    pub name: *const c_char,
    pub parent: Option<&'static Type>, /* None at the root: a null pointer to C */
    pub size: usize,
}

/* A Type is never written after it is built, so sharing one between threads is
   safe.  Rust cannot see that through the raw `name` pointer; this says so. */
unsafe impl Sync for Type {}

extern "C" {
    pub fn Type_cast(object: *mut c_void, base: *const Type) -> *mut c_void;
    pub fn malloc(size: usize) -> *mut c_void;
    pub fn free(memory: *mut c_void);
}
