// Regression test for task-bun-20261122-generic-arg-opt:
// Postfix optional types inside generic call args like `Box(i32?)` must lower to `Box(?i32)`.

import { expect_eq } from "cap:test";

fn Box(comptime T: type): type {
    return struct {
        v: T,
    };
}

fn Pair(comptime A: type, comptime B: type): type {
    return struct {
        a: A,
        b: B,
    };
}

pub type OptBox = Box(i32?);
pub type NestedOptBox = Box(Box(i32?));
pub type OptPair = Pair(i32?, u64?);

fn check_expr_pos(): i32 {
    const ExprBox = Box(i32?);
    let eb: ExprBox = .{ v: 55 };
    return eb.v.?;
}

fn main() {
    let b1: OptBox = .{ v: 123 };
    let b2: OptBox = .{ v: null };
    expect_eq(b1.v.?, 123);
    expect_eq(b2.v == null, true);

    let nb: NestedOptBox = .{ v: .{ v: 99 } };
    expect_eq(nb.v.v.?, 99);

    let p: OptPair = .{ a: 10, b: null };
    expect_eq(p.a.?, 10);
    expect_eq(p.b == null, true);

    expect_eq(check_expr_pos(), 55);
}
