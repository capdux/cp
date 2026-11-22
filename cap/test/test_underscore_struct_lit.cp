import { test, expect_eq } from "cap:test";

pub struct _S {
    x: i32,
}

pub struct __Inner {
    a: i32,
    b: i32,
}

fn makeS(v: i32): _S {
    return _S{ x: v };
}

fn makeInner(a: i32, b: i32): __Inner {
    return __Inner{ a: a, b: b };
}

fn main() {
    test("underscore-prefixed struct type literals _S{...} and __Inner{...}", () => {
        const s = makeS(42);
        expect_eq(s.x, 42);
        const inner = makeInner(3, 4);
        expect_eq(inner.a + inner.b, 7);
    });
}
