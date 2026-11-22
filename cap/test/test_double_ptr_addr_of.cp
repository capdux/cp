import { test, expect_eq } from "cap:test";

pub struct S {
    x: i32 = 0,
}

type SelfPtr = *S;

pub struct Holder {
    ptr: *S,
}

fn takeOut(out: **S, target: *S) {
    out.* = target;
}

fn takeAliasOut(out: *SelfPtr, target: *S) {
    out.* = target;
}

fn readDoubleConst(pp: *const *S) -> i32 {
    return pp.*.*.x;
}

fn takeConstInnerOut(out: **const S, target: *const S) {
    out.* = target;
}

fn readDoubleConstBoth(pp: *const *const S) -> i32 {
    return pp.*.*.x;
}

fn takeOptPtrOut(out: *((*S)?), target: *S) {
    out.* = target;
}

class HeapBox {
    v: i32;
    fn init(val: i32) {
        this.v = val;
    }
}

fn readHeapBox(b: &HeapBox) -> i32 {
    return b.v;
}

fn main() {
    test("double pointer **S and &p on local pointer var", () => {
        let s1 = S { x: 42 };
        let s2 = S { x: 99 };
        let p: *S = &s1;
        expect_eq(p.x, 42);

        takeOut(&p, &s2);
        expect_eq(p.x, 99);
        expect_eq(readDoubleConst(&p), 99);

        let pp: **S = &p;
        expect_eq(pp.*.*.x, 99);
    });

    test("type alias SelfPtr = *S and *SelfPtr out param", () => {
        let s1 = S { x: 10 };
        let s2 = S { x: 77 };
        let p: *S = &s1;
        takeAliasOut(&p, &s2);
        expect_eq(p.x, 77);

        let p2: SelfPtr = &s1;
        takeAliasOut(&p2, &s2);
        expect_eq(p2.x, 77);
        takeOut(&p2, &s1);
        expect_eq(p2.x, 10);
    });

    test("address of struct pointer field &holder.ptr", () => {
        let s1 = S { x: 1 };
        let s2 = S { x: 123 };
        let h = Holder { ptr: &s1 };
        takeOut(&h.ptr, &s2);
        expect_eq(h.ptr.x, 123);
        takeAliasOut(&h.ptr, &s1);
        expect_eq(h.ptr.x, 1);
    });

    test("const double pointers **const S and *const *const S", () => {
        let s1 = S { x: 55 };
        let s2 = S { x: 88 };
        let cp: *const S = &s1;
        takeConstInnerOut(&cp, &s2);
        expect_eq(cp.x, 88);
        expect_eq(readDoubleConstBoth(&cp), 88);
    });

    test("optional pointer address *((*S)?)", () => {
        let s = S { x: 314 };
        let op: (*S)? = null;
        takeOptPtrOut(&op, &s);
        if (p := op) {
            expect_eq(p.x, 314);
        } else {
            @panic("expected non-null optional pointer");
        }
    });

    test("implicit heap class borrow still collapses &b without double ptr", () => {
        let b = new HeapBox(512);
        expect_eq(readHeapBox(&b), 512);
    });
}
