// Regression test for task-bun-20261125-discard-raii-guard (E0220)
// Verifies that:
// 1. Named RAII Guard bindings compile cleanly and run ~init() at scope exit.
// 2. `* = existing_guard;` (silencing unused warning on a named local) is allowed.
// 3. Discarding plain POD structs (without ~init()/~errinit()) via `* = Pod{ ... };` is allowed.

import std from "std";

pub struct Pod {
    pub val: i32,
}

pub struct CounterGuard {
    pub counter: *i32!,
    ~init() {
        unsafe { _.counter.* += 1; }
    }
}

pub fn makePod(v: i32): Pod {
    return Pod{ .val = v };
}

pub fn runGuardScope(counter: *i32!) {
    const g = CounterGuard{ .counter = counter };
    * = g; // silencing unused warning on already-bound local is valid
}

fn main() {
    * = makePod(42);
    * = Pod{ .val = 7 };
    let drops: i32 = 0;
    runGuardScope(&drops);
    if (drops != 1) {
        @panic("expected CounterGuard ~init() to run exactly once");
    }
    std.debug.print("[E0220_RAII_GUARD_PASS] drops={d}\n", .{drops});
}
