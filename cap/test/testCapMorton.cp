// cap/test/testCapMorton.cp
// Comprehensive unit tests for @cap/morton

import { test, expect_eq } from "cap:test";
import {
    Point2D,
    Point3D,
    encode2D,
    decode2D,
    encode3D,
    decode3D,
    hilbert2DEncode,
    hilbert2DDecode,
} from "../../packages/@cap/morton/src/index.cp";

fn absDiffU32(a: u32, b: u32) -> u32 {
    return if (a > b) a - b else b - a;
}

fn main() {
    test("1. @cap/morton — 2D Morton standard vectors", () => {
        expect_eq(encode2D(0, 0), 0 as u64);
        expect_eq(encode2D(1, 0), 1 as u64);
        expect_eq(encode2D(0, 1), 2 as u64);
        expect_eq(encode2D(1, 1), 3 as u64);
        expect_eq(encode2D(3, 2), 13 as u64);

        const p = decode2D(13);
        expect_eq(p.x, 3 as u32);
        expect_eq(p.y, 2 as u32);
    });

    test("2. @cap/morton — 2D full 32-bit extremes (0xFFFFFFFF) lossless round-trip", () => {
        const u32_max: u32 = 0xFFFFFFFF;
        const u64_max: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        expect_eq(encode2D(u32_max, u32_max), u64_max);

        const p_max = decode2D(u64_max);
        expect_eq(p_max.x, u32_max);
        expect_eq(p_max.y, u32_max);

        const c = encode2D(0x12345678, 0x9ABCDEF0);
        const back = decode2D(c);
        expect_eq(back.x, 0x12345678 as u32);
        expect_eq(back.y, 0x9ABCDEF0 as u32);
    });

    test("3. @cap/morton — 3D Morton standard vectors & 21-bit max (0x1FFFFF) round-trip", () => {
        expect_eq(encode3D(0, 0, 0), 0 as u64);
        expect_eq(encode3D(1, 0, 0), 1 as u64);
        expect_eq(encode3D(0, 1, 0), 2 as u64);
        expect_eq(encode3D(0, 0, 1), 4 as u64);
        expect_eq(encode3D(1, 1, 1), 7 as u64);

        const max21: u32 = 0x1FFFFF;
        const c3_max = encode3D(max21, max21, max21);
        const mask63: u64 = ((0x7FFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        expect_eq(c3_max, mask63);

        const p3 = decode3D(c3_max);
        expect_eq(p3.x, max21);
        expect_eq(p3.y, max21);
        expect_eq(p3.z, max21);

        // Arbitrary 21-bit coordinates
        const arb = encode3D(123456, 654321, 1048575);
        const back = decode3D(arb);
        expect_eq(back.x, 123456 as u32);
        expect_eq(back.y, 654321 as u32);
        expect_eq(back.z, 1048575 as u32);
    });

    test("4. @cap/morton — Hilbert 2D consecutive indices always have Manhattan distance 1", () => {
        // Order 4 => 16x16 grid = 256 points
        let d: u64 = 0;
        let prev = hilbert2DDecode(4, 0);
        d = 1;
        while (d < 256) {
            const cur = hilbert2DDecode(4, d);
            const manhattan = absDiffU32(cur.x, prev.x) + absDiffU32(cur.y, prev.y);
            expect_eq(manhattan, 1 as u32);
            prev = cur;
            d += 1;
        }
    });

    test("5. @cap/morton — Hilbert 2D lossless encode/decode bijection across entire grid", () => {
        // Exhaustively verify all 64 points of order-3 (8x8) grid
        let d: u64 = 0;
        while (d < 64) {
            const pt = hilbert2DDecode(3, d);
            expect_eq(hilbert2DEncode(3, pt.x, pt.y), d);
            d += 1;
        }
    });

    test("6. @cap/morton — Hilbert 2D high-order (order=16) & clamping boundary checks", () => {
        const d16 = hilbert2DEncode(16, 43210, 12345);
        const p16 = hilbert2DDecode(16, d16);
        expect_eq(p16.x, 43210 as u32);
        expect_eq(p16.y, 12345 as u32);

        // Order clamped to 1..=16 (order=0 -> 1, order=99 -> 16)
        const d0 = hilbert2DEncode(0, 1, 1);
        const p0 = hilbert2DDecode(0, d0);
        expect_eq(p0.x, 1 as u32);
        expect_eq(p0.y, 1 as u32);
    });
}
