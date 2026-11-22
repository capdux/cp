// Source: cp/cap/test/test_perlin_production_deep.cp (Production deep tests for cap:perlin)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    fade,
    lerp,
    PerlinNoise,
} from "cap:gfx";

fn absF(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn approxEq(a: f64, b: f64, eps: f64): bool {
    return absF(a - b) <= eps;
}

test("1. fade/lerp identities & 1D/2D/3D exact zero value invariant at integer lattice points", () => {
    expect_eq(approxEq(fade(0.0), 0.0, 1.0e-12), true);
    expect_eq(approxEq(fade(0.5), 0.5, 1.0e-12), true);
    expect_eq(approxEq(fade(1.0), 1.0, 1.0e-12), true);
    expect_eq(approxEq(lerp(10.0, 20.0, 0.25), 12.5, 1.0e-12), true);

    const p = PerlinNoise.classic();
    const coords: [7]f64 = [-512.0, -7.0, -1.0, 0.0, 1.0, 42.0, 1024.0];

    let i: usize = 0;
    while (i < 7) {
        expect_eq(approxEq(p.noise1d(coords[i]), 0.0, 1.0e-12), true);
        let j: usize = 0;
        while (j < 7) {
            expect_eq(approxEq(p.noise2d(coords[i], coords[j]), 0.0, 1.0e-12), true);
            expect_eq(approxEq(p.noise3d(coords[i], coords[j], coords[i]), 0.0, 1.0e-12), true);
            j += 1;
        }
        i += 1;
    }
});

test("2. Strict [-1, 1] bounds & non-degenerate distribution across positive and negative coordinates", () => {
    const p = PerlinNoise.classic();
    let seen_pos1 = false;
    let seen_neg1 = false;
    let seen_pos2 = false;
    let seen_neg2 = false;
    let seen_pos3 = false;
    let seen_neg3 = false;

    let i: i32 = -50;
    while (i < 50) {
        const x = (i as f64) * 0.37 + 0.13;
        const y = (i as f64) * -0.29 + 0.41;
        const z = (i as f64) * 0.19 - 0.73;

        const n1 = p.noise1d(x);
        const n2 = p.noise2d(x, y);
        const n3 = p.noise3d(x, y, z);

        expect_eq(n1 >= -1.0 && n1 <= 1.0, true);
        expect_eq(n2 >= -1.0 && n2 <= 1.0, true);
        expect_eq(n3 >= -1.0 && n3 <= 1.0, true);

        if (n1 > 0.05) { seen_pos1 = true; }
        if (n1 < -0.05) { seen_neg1 = true; }
        if (n2 > 0.05) { seen_pos2 = true; }
        if (n2 < -0.05) { seen_neg2 = true; }
        if (n3 > 0.05) { seen_pos3 = true; }
        if (n3 < -0.05) { seen_neg3 = true; }
        i += 1;
    }

    expect_eq(seen_pos1 && seen_neg1, true);
    expect_eq(seen_pos2 && seen_neg2, true);
    expect_eq(seen_pos3 && seen_neg3, true);
});

test("3. C^2 spatial continuity across 1e-4 micro-steps including negative & integer crossings", () => {
    const p = PerlinNoise.fromSeed(2026);
    const eps: f64 = 1.0e-4;
    const anchors: [4]f64 = [-12.345, -1.0, 0.0, 7.891];

    let i: usize = 0;
    while (i < 4) {
        const a = anchors[i];
        expect_eq(absF(p.noise1d(a + eps) - p.noise1d(a)) < 0.01, true);
        expect_eq(absF(p.noise2d(a + eps, a - eps) - p.noise2d(a, a)) < 0.01, true);
        expect_eq(absF(p.noise3d(a + eps, a - eps, a + eps) - p.noise3d(a, a, a)) < 0.01, true);
        i += 1;
    }
});

test("4. classic() determinism, fromSeed() reproducibility, distinct seeds & permutation bijection", () => {
    const c1 = PerlinNoise.classic();
    const c2 = PerlinNoise.classic();
    expect_eq(approxEq(c1.noise3d(1.25, 2.5, 3.75), c2.noise3d(1.25, 2.5, 3.75), 1.0e-15), true);

    const s1a = PerlinNoise.fromSeed(42);
    const s1b = PerlinNoise.fromSeed(42);
    const s2 = PerlinNoise.fromSeed(43);

    // Same seed reproduces bit-for-bit identical values
    expect_eq(approxEq(s1a.noise2d(3.14, 2.71), s1b.noise2d(3.14, 2.71), 1.0e-15), true);
    expect_eq(approxEq(s1a.noise3d(-5.5, 1.2, 8.8), s1b.noise3d(-5.5, 1.2, 8.8), 1.0e-15), true);

    // Different seeds produce independent permutations
    let diffs: usize = 0;
    let k: usize = 0;
    while (k < 256) {
        expect_eq(s1a.perm[k], s1a.perm[k + 256]);
        if (s1a.perm[k] != s2.perm[k]) {
            diffs += 1;
        }
        k += 1;
    }
    expect_eq(diffs > 200, true);

    // Verify s1a.perm[0..256] is a true bijection of 0..255
    let seen: [256]bool = std.mem.zeroes([256]bool);
    k = 0;
    while (k < 256) {
        const v = s1a.perm[k] as usize;
        expect_eq(seen[v], false);
        seen[v] = true;
        k += 1;
    }
});

test("5. fbm2d/fbm3d [-1,1] & ridged2d/turbulence2d [0,1] normalization + algebraic identity", () => {
    const p = PerlinNoise.fromSeed(1337);

    let i: i32 = -20;
    while (i < 20) {
        const x = (i as f64) * 0.23 + 0.31;
        const y = (i as f64) * 0.17 - 0.59;
        const z = (i as f64) * -0.11 + 0.77;

        const f2 = p.fbm2d(x, y, 5, 2.0, 0.5);
        const f3 = p.fbm3d(x, y, z, 4, 2.0, 0.5);
        const r2 = p.ridged2d(x, y, 5, 2.0, 0.5);
        const t2 = p.turbulence2d(x, y, 5, 2.0, 0.5);

        expect_eq(f2 >= -1.0 && f2 <= 1.0, true);
        expect_eq(f3 >= -1.0 && f3 <= 1.0, true);
        expect_eq(r2 >= 0.0 && r2 <= 1.0, true);
        expect_eq(t2 >= 0.0 && t2 <= 1.0, true);

        // Exact normalized identity: ridged2d + turbulence2d == 1.0
        expect_eq(approxEq(r2 + t2, 1.0, 1.0e-12), true);
        i += 1;
    }
});

test("6. octaves == 0 boundary returns 0.0 & octaves == 1 matches base noise", () => {
    const p = PerlinNoise.classic();

    expect_eq(approxEq(p.fbm2d(1.5, 2.5, 0, 2.0, 0.5), 0.0, 1.0e-12), true);
    expect_eq(approxEq(p.fbm3d(1.5, 2.5, 3.5, 0, 2.0, 0.5), 0.0, 1.0e-12), true);
    expect_eq(approxEq(p.ridged2d(1.5, 2.5, 0, 2.0, 0.5), 0.0, 1.0e-12), true);
    expect_eq(approxEq(p.turbulence2d(1.5, 2.5, 0, 2.0, 0.5), 0.0, 1.0e-12), true);

    // 1-octave fBm equals raw noise exactly
    expect_eq(approxEq(p.fbm2d(1.3, 2.7, 1, 2.0, 0.5), p.noise2d(1.3, 2.7), 1.0e-12), true);
    expect_eq(approxEq(p.fbm3d(1.3, 2.7, 4.1, 1, 2.0, 0.5), p.noise3d(1.3, 2.7, 4.1), 1.0e-12), true);
});
