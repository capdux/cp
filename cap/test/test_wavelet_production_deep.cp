// Source: cp/cap/test/test_wavelet_production_deep.cp (Production deep tests for cap:wavelet)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    haarForward1d,
    haarInverse1d,
    haarForwardMulti1d,
    haarInverseMulti1d,
    db4Forward1d,
    db4Inverse1d,
    cdf53ForwardI32,
    cdf53InverseI32,
    softThreshold,
    hardThreshold,
    signalEnergy,
} from "cap:math";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

test("1. Haar single-level & 3-level multi-resolution DWT/IDWT roundtrip (< 1e-12) & Parseval energy conservation", () => {
    const orig: [16]f64 = [
        3.0, -1.5, 4.2, 0.8, 7.1, 7.1, -2.0, 5.5,
        1.2, 9.8, -6.3, 0.0, 2.4, 2.6, 8.0, -4.4,
    ];
    let sig: [16]f64 = orig;
    let scratch: [16]f64 = std.mem.zeroes([16]f64);

    const e_orig = signalEnergy(orig[0..]);

    // Single-level forward & inverse
    expect_eq(haarForward1d(sig[0..], scratch[0..]), true);
    expect_eq(absF64(signalEnergy(sig[0..]) - e_orig) < 1.0e-11, true);
    expect_eq(haarInverse1d(sig[0..], scratch[0..]), true);

    let i: usize = 0;
    while (i < 16) {
        expect_eq(absF64(sig[i] - orig[i]) < 1.0e-12, true);
        i += 1;
    }

    // 3-level multi-resolution decomposition & reconstruction
    expect_eq(haarForwardMulti1d(sig[0..], 3, scratch[0..]), true);
    expect_eq(absF64(signalEnergy(sig[0..]) - e_orig) < 1.0e-11, true);
    expect_eq(haarInverseMulti1d(sig[0..], 3, scratch[0..]), true);

    let j: usize = 0;
    while (j < 16) {
        expect_eq(absF64(sig[j] - orig[j]) < 1.0e-12, true);
        j += 1;
    }
});

test("2. Daubechies-4 (db2) DWT/IDWT roundtrip (< 1e-11), Parseval energy & 2 vanishing moments on linear ramp", () => {
    const orig: [16]f64 = [
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
        9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0,
    ];
    let sig: [16]f64 = orig;
    let scratch: [16]f64 = std.mem.zeroes([16]f64);
    const e_orig = signalEnergy(orig[0..]);

    expect_eq(db4Forward1d(sig[0..], scratch[0..]), true);
    expect_eq(absF64(signalEnergy(sig[0..]) - e_orig) < 1.0e-10, true);

    // 2 vanishing moments: interior high-frequency details (indices 8..14, away from periodic wrap) MUST be ~0!
    let k: usize = 8;
    while (k < 15) {
        expect_eq(absF64(sig[k]) < 1.0e-11, true);
        k += 1;
    }

    // Exact reconstruction
    expect_eq(db4Inverse1d(sig[0..], scratch[0..]), true);
    let i: usize = 0;
    while (i < 16) {
        expect_eq(absF64(sig[i] - orig[i]) < 1.0e-11, true);
        i += 1;
    }
});

test("3. JPEG 2000 lossless integer lifting CDF 5/3 bit-exact roundtrip on signed i32 signals", () => {
    const orig: [12]i32 = [
        -1024, 513, -7, 0, 2048, -99999, 123456, -54321,
        42, 42, -1, 1000000,
    ];
    let sig: [12]i32 = orig;
    let scratch: [12]i32 = std.mem.zeroes([12]i32);

    expect_eq(cdf53ForwardI32(sig[0..], scratch[0..]), true);
    expect_eq(cdf53InverseI32(sig[0..], scratch[0..]), true);

    // Zero bit error!
    let i: usize = 0;
    while (i < 12) {
        expect_eq(sig[i], orig[i]);
        i += 1;
    }

    // Minimal length N = 2 boundary
    const orig2: [2]i32 = [-17, 33];
    let sig2: [2]i32 = orig2;
    let sc2: [2]i32 = std.mem.zeroes([2]i32);
    expect_eq(cdf53ForwardI32(sig2[0..], sc2[0..]), true);
    expect_eq(cdf53InverseI32(sig2[0..], sc2[0..]), true);
    expect_eq(sig2[0], orig2[0]);
    expect_eq(sig2[1], orig2[1]);
});

test("4. Donoho-Johnstone softThreshold & hardThreshold denoising behavior", () => {
    let s: [6]f64 = [-5.0, -1.5, -0.5, 0.5, 1.5, 5.0];
    softThreshold(s[0..], 1.5);
    expect_eq(absF64(s[0] + 3.5) < 1.0e-12, true);
    expect_eq(absF64(s[1]) < 1.0e-12, true);
    expect_eq(absF64(s[2]) < 1.0e-12, true);
    expect_eq(absF64(s[3]) < 1.0e-12, true);
    expect_eq(absF64(s[4]) < 1.0e-12, true);
    expect_eq(absF64(s[5] - 3.5) < 1.0e-12, true);

    let h: [6]f64 = [-5.0, -1.5, -0.5, 0.5, 1.5, 5.0];
    hardThreshold(h[0..], 1.5);
    expect_eq(absF64(h[0] + 5.0) < 1.0e-12, true);
    expect_eq(absF64(h[1] + 1.5) < 1.0e-12, true);
    expect_eq(absF64(h[2]) < 1.0e-12, true);
    expect_eq(absF64(h[3]) < 1.0e-12, true);
    expect_eq(absF64(h[4] - 1.5) < 1.0e-12, true);
    expect_eq(absF64(h[5] - 5.0) < 1.0e-12, true);
});

test("5. Invalid length (odd, < 2, db4 < 4, too many levels) & short scratch rejection", () => {
    let odd5: [5]f64 = std.mem.zeroes([5]f64);
    let sc8: [8]f64 = std.mem.zeroes([8]f64);
    let len2: [2]f64 = std.mem.zeroes([2]f64);
    let len4: [4]f64 = std.mem.zeroes([4]f64);
    let sc3: [3]f64 = std.mem.zeroes([3]f64);

    // Odd length rejected
    expect_eq(haarForward1d(odd5[0..], sc8[0..]), false);
    expect_eq(haarInverse1d(odd5[0..], sc8[0..]), false);
    expect_eq(db4Forward1d(odd5[0..], sc8[0..]), false);
    expect_eq(db4Inverse1d(odd5[0..], sc8[0..]), false);

    // db4 requires N >= 4
    expect_eq(db4Forward1d(len2[0..], sc8[0..]), false);
    expect_eq(db4Inverse1d(len2[0..], sc8[0..]), false);

    // Short scratch rejected
    expect_eq(haarForward1d(len4[0..], sc3[0..]), false);
    expect_eq(db4Forward1d(len4[0..], sc3[0..]), false);

    // Invalid multi-level counts (0 or exceeding dyadic depth)
    expect_eq(haarForwardMulti1d(len4[0..], 0, sc8[0..]), false);
    expect_eq(haarForwardMulti1d(len4[0..], 3, sc8[0..]), false); // 4 only supports up to 2 levels
    expect_eq(haarInverseMulti1d(len4[0..], 3, sc8[0..]), false);

    // CDF 5/3 odd length & short scratch rejected
    let odd_i: [3]i32 = std.mem.zeroes([3]i32);
    let sc_i: [4]i32 = std.mem.zeroes([4]i32);
    expect_eq(cdf53ForwardI32(odd_i[0..], sc_i[0..]), false);
    expect_eq(cdf53InverseI32(odd_i[0..], sc_i[0..]), false);
    expect_eq(cdf53ForwardI32(sc_i[0..], odd_i[0..]), false);
});
