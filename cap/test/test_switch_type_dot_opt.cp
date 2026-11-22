import { test, expect_eq } from "cap:test";

pub struct A {
    x: i32,
}

pub struct D {
    y: i32,
}

const B = struct {
    pub const C = A;
    pub const Nested = struct {
        pub const Leaf = D;
    };
};

fn classify(comptime T: type): i32 {
    return switch (T) {
        B.C => 1,
        A? => 2,
        B.Nested.Leaf => 3,
        B.Nested.Leaf? => 4,
        else => 0,
    };
}

fn main() {
    test("comptime switch(T) with multi-level dot and postfix optional types", () => {
        expect_eq(classify(A), 1);
        expect_eq(classify(A?), 2);
        expect_eq(classify(D), 3);
        expect_eq(classify(D?), 4);
        expect_eq(classify(i32), 0);
    });
}
