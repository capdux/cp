// Example 01: Hello CP + Deterministic Stack RAII (~init)
// Run with: cap examples/01_hello.cp

import std from "std";

struct ScopedGuard {
    label: string,

    static fn enter(label: string): ScopedGuard {
        std.debug.print("[RAII enter] '{s}'\n", .{label});
        return ScopedGuard{ .label = label };
    }

    ~init() {
        std.debug.print("[RAII ~init] '{s}' released deterministically\n", .{_.label});
    }
}

fn fib(n: u32): u64 {
    if (n <= 1) return @as(u64, n);
    let a: u64 = 0;
    let b: u64 = 1;
    let i: u32 = 2;
    while (i <= n) : (i += 1) {
        const next = a + b;
        a = b;
        b = next;
    }
    return b;
}

fn main() {
    const guard = ScopedGuard.enter("fib(50)");
    * = guard;
    const ans = fib(50);
    std.debug.print("Hello from CP v1.0! fib(50) = {d}\n", .{ans});
}
