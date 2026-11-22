import { test, expect, expect_eq } from "cap:test";

struct MyBox {
    val: u32,
}

fn setOptU8(p: *(u8?), v: u8?) {
    p.* = v;
}

fn readOptU8(p: *const (u8?)): u8 {
    if (val := p.*) {
        return val;
    }
    return 0;
}

fn setOptBox(p: *(MyBox?), v: MyBox?) {
    p.* = v;
}

fn writeOptionalPtr(p: *u8?, v: u8): bool {
    if (ptr := p) {
        ptr.* = v;
        return true;
    }
    return false;
}

fn writeParenOptionalPtr(p: (*u8)?, v: u8): bool {
    if (ptr := p) {
        ptr.* = v;
        return true;
    }
    return false;
}

fn readOptionalConstPtr(p: *const u8?): u8 {
    if (ptr := p) {
        return ptr.*;
    }
    return 0;
}

fn setArrayOpt(arr: *[2](u8?), idx: usize, v: u8?) {
    arr.*[idx] = v;
}

fn sumSliceOpt(items: []const (u8?)): u32 {
    let total: u32 = 0;
    for (item of items) {
        if (v := item) {
            total += v;
        }
    }
    return total;
}

fn fillSliceOpt(items: [](u8?), v: u8?) {
    for (i of 0..items.len) {
        items[i] = v;
    }
}

test("*(u8?) and *const (u8?) lowering and mutation", () => {
    let opt_val: u8? = null;
    expect_eq(readOptU8(&opt_val), 0);

    setOptU8(&opt_val, 42);
    expect_eq(readOptU8(&opt_val), 42);

    setOptU8(&opt_val, null);
    expect_eq(readOptU8(&opt_val), 0);
});

test("*(MyBox?) pointer to optional struct", () => {
    let maybe_box: MyBox? = null;
    expect(maybe_box == null);

    setOptBox(&maybe_box, MyBox { val: 99 });
    if (b := maybe_box) {
        expect_eq(b.val, 99);
    } else {
        expect(false);
    }

    setOptBox(&maybe_box, null);
    expect(maybe_box == null);
});

test("*u8?, (*u8)? and *const u8? remain optional pointers", () => {
    let x: u8 = 7;
    expect(!writeOptionalPtr(null, 10));
    expect(writeOptionalPtr(&x, 15));
    expect_eq(x, 15);

    expect(!writeParenOptionalPtr(null, 20));
    expect(writeParenOptionalPtr(&x, 25));
    expect_eq(x, 25);

    expect_eq(readOptionalConstPtr(null), 0);
    expect_eq(readOptionalConstPtr(&x), 25);
});

test("*[2](u8?), [](u8?) and []const (u8?) parenthesized optional elements", () => {
    let arr: [2](u8?) = [null, 5];
    setArrayOpt(&arr, 0, 10);
    expect_eq(sumSliceOpt(arr[0..2]), 15);

    fillSliceOpt(arr[0..2], 3);
    expect_eq(sumSliceOpt(arr[0..2]), 6);
});
