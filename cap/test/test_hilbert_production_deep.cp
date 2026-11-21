// Source: cp/cap/test/test_hilbert_production_deep.cp (Production deep tests for cap:hilbert)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    HILBERT_MIN_ORDER,
    HILBERT_MAX_ORDER,
    MORTON3D_MAX_COORD,
    Point2D,
    Point3D,
    hilbert2dMaxIndex,
    hilbert2dEncode,
    hilbert2dDecode,
    morton2dEncode,
    morton2dDecode,
    morton3dEncode,
    morton3dDecode,
    manhattanDist2d,
} from "cap:hilbert";

test("1. 2D Hilbert order=1..5 exhaustive bijection & adjacent Manhattan distance == 1 invariant", () => {
    expect_eq(HILBERT_MIN_ORDER, 1 as u8);
    expect_eq(HILBERT_MAX_ORDER, 32 as u8);
    expect_eq(MORTON3D_MAX_COORD, 0x1FFFFF as u32);

    // Known order=1 curve: (0,0)->0, (0,1)->1, (1,1)->2, (1,0)->3
    expect_eq(hilbert2dEncode(1, 0, 0) ?? unreachable, 0 as u64);
    expect_eq(hilbert2dEncode(1, 0, 1) ?? unreachable, 1 as u64);
    expect_eq(hilbert2dEncode(1, 1, 1) ?? unreachable, 2 as u64);
    expect_eq(hilbert2dEncode(1, 1, 0) ?? unreachable, 3 as u64);

    // Exhaustively test orders 1 through 5 (up to 32x32 = 1024 grid points)
    let seen: [1024]bool = std.mem.zeroes([1024]bool);
    let order: u8 = 1;
    while (order <= 5) {
        const side: u32 = (1 as u32) << (order as u5);
        const total: u64 = (side as u64) * (side as u64);
        const max_idx = hilbert2dMaxIndex(order) ?? unreachable;
        expect_eq(max_idx, total - 1);

        // Clear seen flags for [0..total)
        let k: usize = 0;
        while (k < (total as usize)) {
            seen[k] = false;
            k += 1;
        }

        // Check every index d in 0..total-1: decode -> encode == d, uniqueness, and adj dist == 1
        let prev = Point2D { x: 0, y: 0 };
        let d: u64 = 0;
        while (d < total) {
            const p = hilbert2dDecode(order, d) ?? unreachable;
            expect_eq(p.x < side, true);
            expect_eq(p.y < side, true);

            const flat: usize = (p.y as usize) * (side as usize) + (p.x as usize);
            expect_eq(seen[flat], false);
            seen[flat] = true;

            const re_enc = hilbert2dEncode(order, p.x, p.y) ?? unreachable;
            expect_eq(re_enc, d);

            if (d > 0) {
                expect_eq(manhattanDist2d(prev, p), 1 as u64);
            }
            prev = p;
            d += 1;
        }

        order += 1;
    }
});

test("2. 2D Hilbert order=32 full 64-bit extreme coordinates, endpoints & high-bit adjacency", () => {
    expect_eq(hilbert2dMaxIndex(32) ?? unreachable, 0xFFFFFFFFFFFFFFFF as u64);
    expect_eq(hilbert2dMaxIndex(31) ?? unreachable, 0x3FFFFFFFFFFFFFFF as u64);

    // Endpoints of order=32: index 0 -> (0, 0), max index -> (0xFFFFFFFF, 0)
    const p_start = hilbert2dDecode(32, 0) ?? unreachable;
    expect_eq(p_start.x, 0 as u32);
    expect_eq(p_start.y, 0 as u32);
    expect_eq(hilbert2dEncode(32, 0, 0) ?? unreachable, 0 as u64);

    const p_end = hilbert2dDecode(32, 0xFFFFFFFFFFFFFFFF) ?? unreachable;
    expect_eq(p_end.x, 0xFFFFFFFF as u32);
    expect_eq(p_end.y, 0 as u32);
    expect_eq(hilbert2dEncode(32, 0xFFFFFFFF, 0) ?? unreachable, 0xFFFFFFFFFFFFFFFF as u64);

    // Extreme 32-bit coordinates roundtrip
    const xs: [6]u32 = [0, 1, 0x7FFFFFFF, 0x80000000, 0xDEADBEEF, 0xFFFFFFFF];
    const ys: [6]u32 = [0xFFFFFFFF, 0xCAFEBABE, 0x80000000, 0x7FFFFFFF, 123456789, 0xFFFFFFFF];
    let i: usize = 0;
    while (i < 6) {
        const idx = hilbert2dEncode(32, xs[i], ys[i]) ?? unreachable;
        const p = hilbert2dDecode(32, idx) ?? unreachable;
        expect_eq(p.x, xs[i]);
        expect_eq(p.y, ys[i]);
        i += 1;
    }

    // Verify adjacency across major 64-bit quadrant boundaries (2^62 - 1 -> 2^62, 2^63 - 1 -> 2^63, max-1 -> max)
    const boundaries: [4]u64 = [
        0x3FFFFFFFFFFFFFFF,
        0x7FFFFFFFFFFFFFFF,
        0xBFFFFFFFFFFFFFFF,
        0xFFFFFFFFFFFFFFFE,
    ];
    i = 0;
    while (i < 4) {
        const b = boundaries[i];
        const pa = hilbert2dDecode(32, b) ?? unreachable;
        const pb = hilbert2dDecode(32, b + 1) ?? unreachable;
        expect_eq(manhattanDist2d(pa, pb), 1 as u64);
        expect_eq(hilbert2dEncode(32, pa.x, pa.y) ?? unreachable, b);
        expect_eq(hilbert2dEncode(32, pb.x, pb.y) ?? unreachable, b + 1);
        i += 1;
    }
});

test("3. 2D Morton Z-Order full 32-bit SWAR bit interleaving & exact roundtrip", () => {
    // Axis bit-mask invariants
    expect_eq(morton2dEncode(0, 0), 0 as u64);
    expect_eq(morton2dEncode(1, 0), 0b01 as u64);
    expect_eq(morton2dEncode(0, 1), 0b10 as u64);
    expect_eq(morton2dEncode(1, 1), 0b11 as u64);
    expect_eq(morton2dEncode(0xFFFFFFFF, 0), 0x5555555555555555 as u64);
    expect_eq(morton2dEncode(0, 0xFFFFFFFF), 0xAAAAAAAAAAAAAAAA as u64);
    expect_eq(morton2dEncode(0xFFFFFFFF, 0xFFFFFFFF), 0xFFFFFFFFFFFFFFFF as u64);

    // Single-bit sweeps across all 32 bit positions
    let bit: u5 = 0;
    while (true) {
        const v: u32 = (1 as u32) << bit;
        const zx = morton2dEncode(v, 0);
        const zy = morton2dEncode(0, v);
        const shift_x: u6 = (bit as u6) * 2;
        const shift_y: u6 = shift_x + 1;
        expect_eq(zx, (1 as u64) << shift_x);
        expect_eq(zy, (1 as u64) << shift_y);

        const px = morton2dDecode(zx);
        expect_eq(px.x, v);
        expect_eq(px.y, 0 as u32);

        const py = morton2dDecode(zy);
        expect_eq(py.x, 0 as u32);
        expect_eq(py.y, v);

        if (bit == 31) {
            break;
        }
        bit += 1;
    }

    // Mixed full-width 32-bit roundtrip
    const p = morton2dDecode(morton2dEncode(0xA5A5F00D, 0x12345678));
    expect_eq(p.x, 0xA5A5F00D as u32);
    expect_eq(p.y, 0x12345678 as u32);
});

test("4. 3D Morton Z-Order full 21-bit SWAR bit interleaving & exact roundtrip", () => {
    // Axis bit-mask invariants for 21-bit max coordinate 0x1FFFFF
    expect_eq(morton3dEncode(0, 0, 0) ?? unreachable, 0 as u64);
    expect_eq(morton3dEncode(1, 0, 0) ?? unreachable, 0b001 as u64);
    expect_eq(morton3dEncode(0, 1, 0) ?? unreachable, 0b010 as u64);
    expect_eq(morton3dEncode(0, 0, 1) ?? unreachable, 0b100 as u64);
    expect_eq(morton3dEncode(MORTON3D_MAX_COORD, 0, 0) ?? unreachable, 0x1249249249249249 as u64);
    expect_eq(morton3dEncode(0, MORTON3D_MAX_COORD, 0) ?? unreachable, 0x2492492492492492 as u64);
    expect_eq(morton3dEncode(0, 0, MORTON3D_MAX_COORD) ?? unreachable, 0x4924924924924924 as u64);
    expect_eq(
        morton3dEncode(MORTON3D_MAX_COORD, MORTON3D_MAX_COORD, MORTON3D_MAX_COORD) ?? unreachable,
        0x7FFFFFFFFFFFFFFF as u64,
    );

    // Single-bit sweep across all 21 bit positions for X, Y, Z
    let bit: u5 = 0;
    while (bit < 21) {
        const v: u32 = (1 as u32) << bit;
        const cx = morton3dEncode(v, 0, 0) ?? unreachable;
        const cy = morton3dEncode(0, v, 0) ?? unreachable;
        const cz = morton3dEncode(0, 0, v) ?? unreachable;
        const s: u6 = (bit as u6) * 3;
        expect_eq(cx, (1 as u64) << s);
        expect_eq(cy, (1 as u64) << (s + 1));
        expect_eq(cz, (1 as u64) << (s + 2));

        const dx = morton3dDecode(cx);
        expect_eq(dx.x, v);
        expect_eq(dx.y, 0 as u32);
        expect_eq(dx.z, 0 as u32);

        const dy = morton3dDecode(cy);
        expect_eq(dy.x, 0 as u32);
        expect_eq(dy.y, v);
        expect_eq(dy.z, 0 as u32);

        const dz = morton3dDecode(cz);
        expect_eq(dz.x, 0 as u32);
        expect_eq(dz.y, 0 as u32);
        expect_eq(dz.z, v);

        bit += 1;
    }

    // Mixed 21-bit coordinates roundtrip
    const code = morton3dEncode(0x15A5A5, 0x0B3C7E, 0x1F0F0F) ?? unreachable;
    const pt = morton3dDecode(code);
    expect_eq(pt.x, 0x15A5A5 as u32);
    expect_eq(pt.y, 0x0B3C7E as u32);
    expect_eq(pt.z, 0x1F0F0F as u32);
});

test("5. Strict rejection of invalid orders, out-of-range coordinates & out-of-range indices", () => {
    // Invalid orders: 0, 33, 255
    expect_eq(hilbert2dMaxIndex(0) == null, true);
    expect_eq(hilbert2dMaxIndex(33) == null, true);
    expect_eq(hilbert2dMaxIndex(255) == null, true);

    expect_eq(hilbert2dEncode(0, 0, 0) == null, true);
    expect_eq(hilbert2dEncode(33, 0, 0) == null, true);
    expect_eq(hilbert2dDecode(0, 0) == null, true);
    expect_eq(hilbert2dDecode(33, 0) == null, true);

    // Out-of-bounds coordinates for order=1 (valid 0..1), order=4 (valid 0..15), order=31 (valid 0..2^31-1)
    expect_eq(hilbert2dEncode(1, 2, 0) == null, true);
    expect_eq(hilbert2dEncode(1, 0, 2) == null, true);
    expect_eq(hilbert2dEncode(4, 16, 15) == null, true);
    expect_eq(hilbert2dEncode(4, 15, 16) == null, true);
    expect_eq(hilbert2dEncode(31, 0x80000000, 0) == null, true);
    expect_eq(hilbert2dEncode(31, 0, 0x80000000) == null, true);

    // Out-of-bounds Hilbert indices
    expect_eq(hilbert2dDecode(1, 4) == null, true);
    expect_eq(hilbert2dDecode(2, 16) == null, true);
    expect_eq(hilbert2dDecode(31, 0x4000000000000000) == null, true);

    // Out-of-bounds 3D Morton coordinates (> 0x1FFFFF)
    expect_eq(morton3dEncode(0x200000, 0, 0) == null, true);
    expect_eq(morton3dEncode(0, 0x200000, 0) == null, true);
    expect_eq(morton3dEncode(0, 0, 0x200000) == null, true);
    expect_eq(morton3dEncode(0xFFFFFFFF, 0, 0) == null, true);
});

test("6. manhattanDist2d symmetry, zero distance & full 32-bit overflow-free span", () => {
    const a = Point2D { x: 10, y: 20 };
    const b = Point2D { x: 25, y: 5 };
    expect_eq(manhattanDist2d(a, a), 0 as u64);
    expect_eq(manhattanDist2d(a, b), 30 as u64);
    expect_eq(manhattanDist2d(b, a), 30 as u64);

    // Maximum possible 32-bit corner-to-corner distance = 2 * (2^32 - 1) = 0x1FFFFFFFE
    const origin = Point2D { x: 0, y: 0 };
    const corner = Point2D { x: 0xFFFFFFFF, y: 0xFFFFFFFF };
    expect_eq(manhattanDist2d(origin, corner), 0x1FFFFFFFE as u64);
    expect_eq(manhattanDist2d(corner, origin), 0x1FFFFFFFE as u64);
});
