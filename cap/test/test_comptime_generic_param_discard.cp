import { test, expect_eq } from "cap:test";

pub struct S<T> {
    val: T,
    static fn f(): i32 {
        return @intCast(@sizeOf(T));
    }
}

pub struct Pair<A, B> {
    a: A,
    b: B,
    static fn totalSize(): i32 {
        return @intCast(@sizeOf(A) + @sizeOf(B));
    }
}

pub struct Holder<T> {
    val: T,
}

fn wrap(comptime W: type): i32 {
    return S<W>.f();
}

fn wrapMulti(comptime A: type, comptime B: type): i32 {
    return Pair<A, B>.totalSize();
}

fn wrapStructLit(comptime T: type, v: T): T {
    const b = Holder<T>{ val: v };
    return b.val;
}

fn wrapUnused(comptime U: type): i32 {
    return 7;
}

fn main() {
    test("comptime generic param not pointlessly discarded", () => {
        expect_eq(wrap(i32), 4);
        expect_eq(wrap(i64), 8);
        expect_eq(wrapMulti(i32, i64), 12);
        expect_eq(wrapStructLit(i32, 99), 99);
        expect_eq(wrapUnused(i32), 7);
    });
}
