/* Tests for the Rust binding.  Run: make test-rust
   Labels name the requirement checked, from docs/REQUIREMENTS_v7.md. */

mod oopc;

use oopc::{Bottle, Owned, SizedBottle};

/* REQ7/RS-1: a view owns no bytes and an owner is one pointer.  Compile-time,
   like the static_asserts in cpp/Bottle.hpp: if this file builds, they hold. */
const _: () = assert!(std::mem::size_of::<Bottle>() == 0);
const _: () = assert!(std::mem::size_of::<SizedBottle>() == 0);
const _: () = assert!(std::mem::size_of::<Owned<SizedBottle>>() == std::mem::size_of::<*mut u8>());

fn check<T: PartialEq + std::fmt::Debug>(failures: &mut u32, label: &str, got: T, want: T) {
    if got == want {
        println!("  ok  {:<56} {:?}", label, got);
    } else {
        println!("  FAIL {}: got {:?}, want {:?}", label, got, want);
        *failures += 1;
    }
}

/* Takes the base by reference; the C vtable picks the override. */
fn fill(b: &mut Bottle, value: f64) -> f64 {
    b.set_volume(value);
    b.get_volume()
}

fn main() {
    let mut failures = 0;
    let f = &mut failures;

    let mut b = Bottle::new();
    let mut sb = SizedBottle::new();
    sb.set_max_volume(10.0);

    println!("dispatch: Rust views over a C hierarchy");
    check(f, "REQ7/RS-2 &mut Bottle on a Bottle keeps 100", fill(&mut b, 100.0), 100.0);
    check(f, "REQ7/RS-2 &mut Bottle on a SizedBottle clamps to 10", fill(&mut sb, 100.0), 10.0);
    sb.set_volume(8.0); /* inherited: SizedBottle declares no set_volume */
    sb.set_max_volume(3.0);
    check(f, "REQ2/OM-6 lowering the max lowers the volume", sb.get_volume(), 3.0);
    check(f, "REQ2/OM-6 and the max is 3", sb.get_max_volume(), 3.0);

    println!("\nruntime types and checked downcasts");
    check(f, "REQ2/TY-1 dynamic type through &Bottle", (&*sb as &Bottle).type_name(), "SizedBottle");
    check(f, "REQ2/TY-2 SizedBottle::cast(plain Bottle) is None", SizedBottle::cast(&mut b).is_none(), true);
    check(f, "REQ2/TY-2 SizedBottle::cast(SizedBottle) is Some", SizedBottle::cast(&mut sb).is_some(), true);

    println!("\nsafe code cannot slice");
    b.set_volume(1.0);
    sb.set_volume(2.0);
    std::mem::swap::<Bottle>(&mut b, &mut sb);
    check(f, "REQ7/RS-3 after swap, the Bottle is still a Bottle", b.type_name(), "Bottle");
    check(f, "REQ7/RS-3 after swap, the SizedBottle is still one", sb.type_name(), "SizedBottle");
    check(f, "REQ7/RS-3 after swap, the Bottle keeps its volume", b.get_volume(), 1.0);
    check(f, "REQ7/RS-3 after swap, the SizedBottle keeps its volume", sb.get_volume(), 2.0);

    println!("\nan owner of the base deletes the derived object");
    let mut base: Owned<Bottle> = sb.into();
    check(f, "REQ7/RS-2 Owned<Bottle> holding a SizedBottle clamps", fill(&mut base, 100.0), 3.0);
    drop(base); /* Bottle_delete dispatches: SizedBottle's fini, then Bottle's */

    if failures != 0 {
        println!("\nRust: {} FAILED", failures);
        std::process::exit(1);
    }
    println!("\nRust: all checks passed");
}
