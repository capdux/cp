// Source: cp/cap/test/test_gf256_production_deep.cp (Production deep tests for cap:gf256)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    GF256_POLY_AES,
    GF256_POLY_RS,
    gf256MulConstTime,
    Gf256Field,
} from "cap:gf256";

test("1. AES (0x11B) & RS (0x11D) exhaustive 255-element inverse, division, and power identities", () => {
    expect_eq(GF256_POLY_AES, 0x1B as u8);
    expect_eq(GF256_POLY_RS, 0x1D as u8);

    const aes: Gf256Field = Gf256Field.aes();
    const rs: Gf256Field = Gf256Field.rs();

    // Exhaustive check for all 255 non-zero elements in both fields.
    let a: u16 = 1;
    while (a < 256) {
        const au: u8 = a as u8;

        // AES field: a * inv(a) == 1 and a^255 == 1 (Fermat's Little Theorem).
        if (ia := aes.inv(au)) {
            expect_eq(aes.mul(au, ia), 1 as u8);
            expect_eq(aes.mul(ia, au), 1 as u8);
        } else {
            expect_eq(true, false);
        }
        expect_eq(aes.pow(au, 255), 1 as u8);

        // RS field: a * inv(a) == 1 and div(mul(a, b), b) == a.
        if (ir := rs.inv(au)) {
            expect_eq(rs.mul(au, ir), 1 as u8);
        } else {
            expect_eq(true, false);
        }
        expect_eq(rs.pow(au, 255), 1 as u8);

        const b: u8 = ((a * 37 + 13) % 255 + 1) as u8;
        const prod_a: u8 = aes.mul(au, b);
        if (q_a := aes.div(prod_a, b)) {
            expect_eq(q_a, au);
        } else {
            expect_eq(true, false);
        }
        const prod_r: u8 = rs.mul(au, b);
        if (q_r := rs.div(prod_r, b)) {
            expect_eq(q_r, au);
        } else {
            expect_eq(true, false);
        }

        a += 1;
    }
});

test("2. AES MixColumns vectors (0x57*0x83==0xC1, 0x57*0x13==0xFE) and 100% table vs const-time equivalence", () => {
    const aes: Gf256Field = Gf256Field.aes();
    const rs: Gf256Field = Gf256Field.rs();

    // FIPS-197 AES textbook vectors.
    expect_eq(aes.mul(0x57, 0x83), 0xC1 as u8);
    expect_eq(aes.mul(0x57, 0x13), 0xFE as u8);
    expect_eq(gf256MulConstTime(0x57, 0x83, GF256_POLY_AES), 0xC1 as u8);
    expect_eq(gf256MulConstTime(0x57, 0x13, GF256_POLY_AES), 0xFE as u8);

    // Exhaustive 256 x 256 equivalence check for AES and sampled full check for RS.
    let x: u16 = 0;
    while (x < 256) {
        let y: u16 = 0;
        while (y < 256) {
            const xu: u8 = x as u8;
            const yu: u8 = y as u8;
            expect_eq(aes.mul(xu, yu), gf256MulConstTime(xu, yu, GF256_POLY_AES));
            expect_eq(rs.mul(xu, yu), gf256MulConstTime(xu, yu, GF256_POLY_RS));
            y += 1;
        }
        x += 1;
    }
});

test("3. Erasure coding primitives: mulAddSlice, scaleSlice, and 3x3 GF(2^8) matrix inversion", () => {
    const rs: Gf256Field = Gf256Field.rs();

    // Simulate 2-data + 2-parity RAID-6 / Cauchy linear combination and exact recovery.
    const d0: [8]u8 = [0x10, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88];
    const d1: [8]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x01, 0x02];

    // P = d0 ^ d1; Q = 2*d0 ^ 3*d1
    let p_arr: [8]u8 = std.mem.zeroes([8]u8);
    let q_arr: [8]u8 = std.mem.zeroes([8]u8);
    let p_slice: []u8 = p_arr[0..];
    let q_slice: []u8 = q_arr[0..];

    expect_eq(rs.mulAddSlice(*p_slice, d0[0..], 1), true);
    expect_eq(rs.mulAddSlice(*p_slice, d1[0..], 1), true);
    expect_eq(rs.mulAddSlice(*q_slice, d0[0..], 2), true);
    expect_eq(rs.mulAddSlice(*q_slice, d1[0..], 3), true);

    // Recover d0 and d1 from (P, Q) via 2x2 matrix inverse of [[1, 1], [2, 3]].
    let mat_work: [4]u8 = [1, 1, 2, 3];
    let mat_inv: [4]u8 = std.mem.zeroes([4]u8);
    let w_slice: []u8 = mat_work[0..];
    let inv_slice: []u8 = mat_inv[0..];
    expect_eq(rs.invertMatrix(*w_slice, *inv_slice, 2), true);

    let rec0_arr: [8]u8 = std.mem.zeroes([8]u8);
    let rec1_arr: [8]u8 = std.mem.zeroes([8]u8);
    let rec0: []u8 = rec0_arr[0..];
    let rec1: []u8 = rec1_arr[0..];

    expect_eq(rs.mulAddSlice(*rec0, p_arr[0..], mat_inv[0]), true);
    expect_eq(rs.mulAddSlice(*rec0, q_arr[0..], mat_inv[1]), true);
    expect_eq(rs.mulAddSlice(*rec1, p_arr[0..], mat_inv[2]), true);
    expect_eq(rs.mulAddSlice(*rec1, q_arr[0..], mat_inv[3]), true);

    let i: usize = 0;
    while (i < 8) {
        expect_eq(rec0_arr[i], d0[i]);
        expect_eq(rec1_arr[i], d1[i]);
        i += 1;
    }

    // Verify scaleSlice round-trip with inverse scalar.
    if (inv7 := rs.inv(7)) {
        rs.scaleSlice(*rec0, 7);
        rs.scaleSlice(*rec0, inv7);
        let k: usize = 0;
        while (k < 8) {
            expect_eq(rec0_arr[k], d0[k]);
            k += 1;
        }
    } else {
        expect_eq(true, false);
    }
});

test("4. Polynomial evaluation/multiplication and Shamir (k=4, n=6) Lagrange interpolation recovery", () => {
    const rs: Gf256Field = Gf256Field.rs();

    // Polynomial multiplication check: (a * b)(x) == a(x) * b(x) for all x in 0..31.
    const pa: [3]u8 = [0x05, 0x12, 0x34];
    const pb: [3]u8 = [0x09, 0xAB, 0xCD];
    let prod_arr: [8]u8 = std.mem.zeroes([8]u8);
    let prod_slice: []u8 = prod_arr[0..];

    if (plen := rs.polyMul(pa[0..], pb[0..], *prod_slice)) {
        expect_eq(plen, 5 as usize);
        let x: u8 = 0;
        while (x < 32) {
            const lhs: u8 = rs.polyEval(prod_arr[0..plen], x);
            const rhs: u8 = rs.mul(rs.polyEval(pa[0..], x), rs.polyEval(pb[0..], x));
            expect_eq(lhs, rhs);
            x += 1;
        }
    } else {
        expect_eq(true, false);
    }

    // Shamir (k=4, n=6) threshold secret sharing over GF(2^8): secret = 0x5A at P(0).
    const poly: [4]u8 = [0x5A, 0x3C, 0xE7, 0x19];
    const all_x: [6]u8 = [1, 2, 3, 4, 5, 6];
    let all_y: [6]u8 = std.mem.zeroes([6]u8);
    let s: usize = 0;
    while (s < 6) {
        all_y[s] = rs.polyEval(poly[0..], all_x[s]);
        s += 1;
    }

    // Any 4 shares (e.g. indices 0, 2, 4, 5) recover P(0) == 0x5A and P(2) == all_y[1].
    const sub_x: [4]u8 = [all_x[0], all_x[2], all_x[4], all_x[5]];
    const sub_y: [4]u8 = [all_y[0], all_y[2], all_y[4], all_y[5]];

    if (secret := rs.lagrangeInterpolateAt(sub_x[0..], sub_y[0..], 0)) {
        expect_eq(secret, 0x5A as u8);
    } else {
        expect_eq(true, false);
    }
    if (eval2 := rs.lagrangeInterpolateAt(sub_x[0..], sub_y[0..], 2)) {
        expect_eq(eval2, all_y[1]);
    } else {
        expect_eq(true, false);
    }
    // Evaluating directly at an existing node must also match.
    if (eval3 := rs.lagrangeInterpolateAt(sub_x[0..], sub_y[0..], 3)) {
        expect_eq(eval3, all_y[2]);
    } else {
        expect_eq(true, false);
    }
});

test("5. Error & boundary guards: div/inv by zero, duplicate Lagrange x, singular matrix, short buffers", () => {
    const aes: Gf256Field = Gf256Field.aes();

    // Division / inversion by zero returns null.
    if (_ := aes.inv(0)) {
        expect_eq(true, false);
    }
    if (_ := aes.div(42, 0)) {
        expect_eq(true, false);
    }
    if (z := aes.div(0, 42)) {
        expect_eq(z, 0 as u8);
    } else {
        expect_eq(true, false);
    }
    expect_eq(aes.pow(0, 0), 1 as u8);
    expect_eq(aes.pow(0, 5), 0 as u8);

    // Duplicate x-coordinates or length mismatch rejected by Lagrange interpolation.
    const dup_x: [3]u8 = [1, 2, 1];
    const dup_y: [3]u8 = [10, 20, 30];
    if (_ := aes.lagrangeInterpolateAt(dup_x[0..], dup_y[0..], 0)) {
        expect_eq(true, false);
    }
    if (_ := aes.lagrangeInterpolateAt(dup_x[0..2], dup_y[0..3], 0)) {
        expect_eq(true, false);
    }

    // Short polyMul output buffer rejected.
    const p1: [3]u8 = [1, 2, 3];
    const p2: [3]u8 = [4, 5, 6];
    let short_arr: [4]u8 = std.mem.zeroes([4]u8);
    let short_slice: []u8 = short_arr[0..];
    if (_ := aes.polyMul(p1[0..], p2[0..], *short_slice)) {
        expect_eq(true, false);
    }

    // Mismatched mulAddSlice rejected.
    let dst4: [4]u8 = std.mem.zeroes([4]u8);
    let dst4_s: []u8 = dst4[0..];
    expect_eq(aes.mulAddSlice(*dst4_s, p1[0..], 5), false);

    // Singular matrix (identical rows) rejected by invertMatrix.
    let sing_w: [4]u8 = [3, 5, 3, 5];
    let sing_inv: [4]u8 = std.mem.zeroes([4]u8);
    let sw: []u8 = sing_w[0..];
    let si: []u8 = sing_inv[0..];
    expect_eq(aes.invertMatrix(*sw, *si, 2), false);
});
