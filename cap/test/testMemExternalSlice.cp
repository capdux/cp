// Deep production regression tests for cap:mem externalSlice.cp
// Covers compact (off, len) ExternalSlice<T>, sentinel invalidation, bounds clamping & strict checks,
// overflow-safe subslice/contains, ExternalSliceMap<T>, and zero-heap ExternalBytePool.

import { test, expect } from "cap:test";
import {
    EXTERNAL_SLICE_INVALID_SENTINEL,
    ExternalSlice,
    ExternalByteSlice,
    ExternalSliceMap,
    ExternalBytePool,
} from "cap:mem";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("externalSlice: init, fromRange, empty, invalid sentinel and size invariants", () => {
    expect(EXTERNAL_SLICE_INVALID_SENTINEL == 0xFFFF_FFFF);

    const e = ExternalSlice<u8>.empty();
    expect(e.isEmpty());
    expect(!e.isInvalid());
    expect(e.begin() == 0);
    expect(e.end() == 0);

    const inv = ExternalSlice<u8>.invalid();
    expect(inv.isInvalid());
    expect(!inv.isEmpty());
    expect(!inv.contains(0));
    expect(!inv.contains(EXTERNAL_SLICE_INVALID_SENTINEL));
    expect(!inv.isWithin(1024));

    const r_ok = ExternalSlice<u32>.fromRange(10, 15);
    expect(r_ok != null);
    if (r := r_ok) {
        expect(r.off == 10);
        expect(r.len == 5);
        expect(r.begin() == 10);
        expect(r.end() == 15);
        expect(!r.contains(9));
        expect(r.contains(10));
        expect(r.contains(14));
        expect(!r.contains(15));
    }

    // Reversed range rejected
    expect(ExternalSlice<u32>.fromRange(20, 10) == null);
});

test("externalSlice: zero-copy get, mut, getChecked, mutChecked and boundary clamping", () => {
    let data: [8]u8 = [10, 20, 30, 40, 50, 60, 70, 80];
    const s = ExternalByteSlice.init(2, 4);

    expect(s.isWithin(data.len));
    const view = s.get(data[0..]);
    expect(view.len == 4);
    expect(view[0] == 30 && view[3] == 60);

    // Zero-copy mutable view modifies backing buffer in place
    let mview = s.mut(data[0..]);
    expect(mview.len == 4);
    mview[0] = 99;
    expect(data[2] == 99);

    // Checked views succeed in-bounds
    const ck = s.getChecked(data[0..]);
    expect(ck != null);
    if (v := ck) {
        expect(v.len == 4 && v[0] == 99);
    }
    const mck = s.mutChecked(data[0..]);
    expect(mck != null);
    if (mck != null) {
        let mv = mck.?;
        mv[3] = 123;
    }
    expect(data[5] == 123);

    // Out-of-bounds: clamping get() truncates safely; getChecked() returns null
    const oob = ExternalByteSlice.init(6, 10);
    expect(!oob.isWithin(data.len));
    const clamped = oob.get(data[0..]);
    expect(clamped.len == 2);
    expect(clamped[0] == 70 && clamped[1] == 80);
    expect(oob.getChecked(data[0..]) == null);
    expect(oob.mutChecked(data[0..]) == null);

    // Start beyond buffer & invalid sentinel both yield empty slice on get()
    const far = ExternalByteSlice.init(100, 5);
    expect(far.get(data[0..]).len == 0);
    expect(ExternalByteSlice.invalid().get(data[0..]).len == 0);
    expect(ExternalByteSlice.invalid().mut(data[0..]).len == 0);
});

test("externalSlice: subslice derivation and u32 overflow protection", () => {
    const base = ExternalSlice<u8>.init(10, 20);
    const sub = base.subslice(5, 8);
    expect(sub != null);
    if (s := sub) {
        expect(s.off == 15);
        expect(s.len == 8);
        expect(s.eql(ExternalSlice<u8>.init(15, 8)));
    }

    // Subslice exceeding parent length is rejected
    expect(base.subslice(15, 10) == null);
    expect(base.subslice(21, 0) == null);

    // Overflowing u32 arithmetic in contains / end / subslice does not wrap around
    const high = ExternalSlice<u8>.init(0xFFFF_FFF0, 32);
    expect(high.end() == (@as(u64, 0xFFFF_FFF0) + 32));
    expect(!high.contains(0));
    expect(high.contains(0xFFFF_FFF0));
    expect(high.subslice(30, 10) == null);
    expect(ExternalSlice<u8>.invalid().subslice(0, 0) == null);
});

test("externalSlice: ExternalSliceMap paired key/value ranges", () => {
    let arena: [6]u32 = [101, 102, 103, 901, 902, 903];
    const keys = ExternalSlice<u32>.init(0, 3);
    const vals = ExternalSlice<u32>.init(3, 3);

    const map = ExternalSliceMap<u32>.init(keys, vals);
    expect(map.isBalanced());
    expect(map.count() == 3);

    const ks = map.getKeys(arena[0..]);
    const vs = map.getValues(arena[0..]);
    expect(ks.len == 3 && vs.len == 3);
    expect(ks[0] == 101 && vs[0] == 901);
    expect(ks[2] == 103 && vs[2] == 903);

    // Unbalanced map reports isBalanced() == false and count() == 0
    const bad = ExternalSliceMap<u32>.init(ExternalSlice<u32>.init(0, 2), ExternalSlice<u32>.init(2, 3));
    expect(!bad.isBalanced());
    expect(bad.count() == 0);

    const empty_map = ExternalSliceMap<u32>.empty();
    expect(empty_map.isBalanced());
    expect(empty_map.count() == 0);
});

test("externalSlice: ExternalBytePool zero-heap string interning and reset", () => {
    let storage: [32]u8 = [0; 32];
    let pool = ExternalBytePool.init(storage[0..]);

    expect(pool.used() == 0);
    expect(pool.remaining() == 32);

    const h1 = pool.append("react");
    const h2 = pool.append("typescript");
    const h3 = pool.append("bun");
    expect(h1 != null && h2 != null && h3 != null);

    if (s1 := h1) {
        expect(s1.off == 0 && s1.len == 5);
        expect(strEq(pool.get(s1), "react"));
    }
    if (s2 := h2) {
        expect(s2.off == 5 && s2.len == 10);
        expect(strEq(pool.get(s2), "typescript"));
    }
    if (s3 := h3) {
        expect(s3.off == 15 && s3.len == 3);
        expect(strEq(pool.get(s3), "bun"));
    }

    expect(pool.used() == 18);
    expect(pool.remaining() == 14);

    // Oversized append fails cleanly without mutating pos
    expect(pool.append("this-string-exceeds-14-bytes") == null);
    expect(pool.used() == 18);

    // Reset reclaims pool in O(1); old handles become out-of-bounds for getChecked()
    pool.reset();
    expect(pool.used() == 0);
    expect(pool.remaining() == 32);
    if (s1 := h1) {
        expect(pool.getChecked(s1) == null);
    }
});

test("externalSlice: survives relocation across independent backing copies", () => {
    let buf_a: [16]u8 = [0; 16];
    let pool = ExternalBytePool.init(buf_a[0..]);
    const h = pool.append("relocatable").?;

    // Simulate backing buffer relocation (e.g. arena growth / mmap reload)
    let buf_b: [32]u8 = [0; 32];
    let i: usize = 0;
    while (i < buf_a.len) {
        buf_b[i] = buf_a[i];
        i += 1;
    }

    // Same 8-byte handle resolves seamlessly against the relocated buffer
    expect(strEq(h.get(buf_b[0..]), "relocatable"));
    const ck = h.getChecked(buf_b[0..]);
    expect(ck != null);
    if (v := ck) {
        expect(strEq(v, "relocatable"));
    }
});
