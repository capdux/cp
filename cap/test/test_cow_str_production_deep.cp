import std from "std";
import { test, expect } from "cap:test";
import { CowStr, CowStrKind } from "cap:cow_str";

test("cow_str: empty and borrowed slices incur zero heap allocations", () => {
    const e = CowStr.initEmpty();
    expect(e.isEmpty()).toBeTrue();
    expect(e.slice()).toBe("");
    expect(e.refCount()).toBe(0);

    const b = CowStr.initBorrowed("hello-borrowed");
    expect(b.isBorrowed()).toBeTrue();
    expect(b.isOwned()).toBeFalse();
    expect(b.slice()).toBe("hello-borrowed");
    expect(b.len()).toBe(14);
    expect(b.memoryCost()).toBe(14);

    const b2 = b.clone();
    expect(b2.eql(b)).toBeTrue();
    expect(b2.isBorrowed()).toBeTrue();
});

test("cow_str: refcounted owned string clone, amortized memoryCost, and zero-leak ~init()", () => {
    let s1 = CowStr.dupeOwned(std.testing.allocator, "0123456789abcdef") catch unreachable;
    expect(s1.isOwned()).toBeTrue();
    expect(s1.refCount()).toBe(1);
    expect(s1.memoryCost()).toBe(16);
    expect(s1.eqlStr("0123456789abcdef")).toBeTrue();

    {
        let s2 = s1.clone();
        expect(s1.refCount()).toBe(2);
        expect(s2.refCount()).toBe(2);
        expect(s1.memoryCost()).toBe(8);
        expect(s2.eql(s1)).toBeTrue();
        // s2 ~init() runs here -> refcount drops back to 1
    }

    expect(s1.refCount()).toBe(1);
    expect(s1.slice()).toBe("0123456789abcdef");
    // s1 ~init() runs here -> refcount drops to 0 and frees with zero leaks
});
