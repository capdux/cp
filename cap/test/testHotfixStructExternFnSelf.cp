// Regression test for task-bun-20261125-struct-extern-fn-self:
// extern fn / @c fn declared inside struct/enum must be treated as static C-ABI
// functions (no implicit `self` first param, no `self.Foo__bar(...)` rewrite).

pub struct Foo {
    ptr: [*]const u8,
    len: usize,

    extern fn Foo__toJS(this_foo: *const Foo, arg: usize): usize;
    @c fn Foo__withUnderscore(_: *const Foo, arg: usize): usize;

    @c fn cCallback(a: usize, b: usize): usize {
        return a + b;
    }

    pub fn toJS(): usize {
        return Foo__toJS(&_, 1);
    }

    pub fn callCallback(): usize {
        return cCallback(_.len, 42);
    }
}

pub enum Tag : u8 {
    A = 0,
    B = 1,

    extern fn Tag__toInt(t: Tag): u8;
}

fn main() {
    let buf: string = "abcd";
    let f = Foo { ptr: buf.ptr, len: buf.len };
    let cb_res = f.callCallback();
    if (cb_res != 46) {
        @panic("expected 46 from @c fn inside struct");
    }
    let cb_direct = Foo.cCallback(10, 20);
    if (cb_direct != 30) {
        @panic("expected 30 from direct Foo.cCallback");
    }
    @print("PASS testHotfixStructExternFnSelf");
}
