// Source: cp/cap/test/test_noise_production_deep.cp
// Deep Production Verification Suite for cap:noise & cap.noise
// Covers:
//   1. NoiseGen determinism & seed isolation (bit-exact same seed, distinct different seeds)
//   2. perlin2 & perlin3 range bounds (1000+ samples in [-1.0, 1.0]) and spatial continuity
//   3. fbm2, ridged2, worley2 range bounds and parameter validation (InvalidOctaves / InvalidFrequency)
//   4. generateGrid2D batch heightmap [0.0, 1.0] validation, InvalidDimensions guard, std.testing.allocator zero-leak
//   5. `import cap from "cap"; cap.noise` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { NoiseError, FractalConfig, NoiseGen } from "cap:noise";

test("cap:noise - determinism and seed isolation") {
    const g1 = NoiseGen.init(123456789);
    const g2 = NoiseGen.init(123456789);
    const g3 = NoiseGen.init(987654321);

    let diff_count: usize = 0;
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        const x = f64(i) * 0.37 - 5.0;
        const y = f64(i) * 0.23 + 1.5;
        const z = f64(i) * 0.19 - 2.2;

        // Bit-exact equality for identical seeds
        expect_eq(g1.perlin2(x, y) == g2.perlin2(x, y), true);
        expect_eq(g1.perlin3(x, y, z) == g2.perlin3(x, y, z), true);
        expect_eq(g1.worley2(x, y) == g2.worley2(x, y), true);

        if (@abs(g1.perlin2(x, y) - g3.perlin2(x, y)) > 0.0001) {
            diff_count += 1;
        }
    }

    // Distinct seeds must produce significantly different terrain
    expect_eq(diff_count > 50, true);
}

test("cap:noise - perlin2 and perlin3 bounds (1000+ samples) & spatial continuity") {
    const gen = NoiseGen.init(42);

    let min2: f64 = 10.0;
    let max2: f64 = -10.0;
    let min3: f64 = 10.0;
    let max3: f64 = -10.0;

    // 35 * 35 = 1225 samples
    let iy: usize = 0;
    while (iy < 35) : (iy += 1) {
        const y = (f64(iy) - 17.0) * 0.31;
        let ix: usize = 0;
        while (ix < 35) : (ix += 1) {
            const x = (f64(ix) - 17.0) * 0.29;
            const z = (f64(ix + iy) - 34.0) * 0.17;

            const v2 = gen.perlin2(x, y);
            const v3 = gen.perlin3(x, y, z);

            expect_eq(v2 >= -1.0 && v2 <= 1.0, true);
            expect_eq(v3 >= -1.0 && v3 <= 1.0, true);

            if (v2 < min2) min2 = v2;
            if (v2 > max2) max2 = v2;
            if (v3 < min3) min3 = v3;
            if (v3 > max3) max3 = v3;
        }
    }

    // Non-trivial spread
    expect_eq(max2 - min2 > 0.5, true);
    expect_eq(max3 - min3 > 0.5, true);

    // Local spatial continuity check across integer cell boundary (x = 1.0, y = 2.0)
    const eps: f64 = 0.001;
    const p_left = gen.perlin2(1.0 - eps, 2.0);
    const p_mid = gen.perlin2(1.0, 2.0);
    const p_right = gen.perlin2(1.0 + eps, 2.0);
    expect_eq(@abs(p_mid - p_left) < 0.01, true);
    expect_eq(@abs(p_right - p_mid) < 0.01, true);

    const p3_a = gen.perlin3(1.0 - eps, 2.0, 3.0);
    const p3_b = gen.perlin3(1.0 + eps, 2.0, 3.0);
    expect_eq(@abs(p3_b - p3_a) < 0.02, true);
}

test("cap:noise - fbm2, ridged2, worley2 bounds and parameter guards") {
    const gen = NoiseGen.init(777);
    const cfg = FractalConfig.init(5, 0.25, 2.0, 0.5);

    let i: usize = 0;
    while (i < 200) : (i += 1) {
        const x = f64(i) * 0.43 - 20.0;
        const y = f64(i) * 0.61 - 15.0;

        const f = gen.fbm2(x, y, cfg) catch unreachable;
        const r = gen.ridged2(x, y, cfg) catch unreachable;
        const w = gen.worley2(x, y);

        expect_eq(f >= -1.0 && f <= 1.0, true);
        expect_eq(r >= 0.0 && r <= 1.0, true);
        expect_eq(w >= 0.0 && w <= 1.45, true);
    }

    // Parameter validation: octaves == 0
    const bad_oct = FractalConfig.init(0, 1.0, 2.0, 0.5);
    let caught_oct = false;
    * = gen.fbm2(1.0, 1.0, bad_oct) catch e => {
        expect_eq(e == NoiseError.InvalidOctaves, true);
        caught_oct = true;
    };
    expect_eq(caught_oct, true);

    let caught_ridged_oct = false;
    * = gen.ridged2(1.0, 1.0, bad_oct) catch e => {
        expect_eq(e == NoiseError.InvalidOctaves, true);
        caught_ridged_oct = true;
    };
    expect_eq(caught_ridged_oct, true);

    // Parameter validation: frequency <= 0
    const bad_freq = FractalConfig.init(4, 0.0, 2.0, 0.5);
    let caught_freq = false;
    * = gen.fbm2(1.0, 1.0, bad_freq) catch e => {
        expect_eq(e == NoiseError.InvalidFrequency, true);
        caught_freq = true;
    };
    expect_eq(caught_freq, true);

    const neg_freq = FractalConfig.init(4, -0.5, 2.0, 0.5);
    let caught_neg_freq = false;
    * = gen.ridged2(1.0, 1.0, neg_freq) catch e => {
        expect_eq(e == NoiseError.InvalidFrequency, true);
        caught_neg_freq = true;
    };
    expect_eq(caught_neg_freq, true);
}

test("cap:noise - generateGrid2D heightmap bounds, guards & zero-leak") {
    const alloc = std.testing.allocator;
    const gen = NoiseGen.init(2026);
    const cfg = FractalConfig.default();

    const width: usize = 24;
    const height: usize = 16;
    const grid = gen.generateGrid2D(alloc, width, height, -2.0, -2.0, 0.15, cfg) catch unreachable;
    defer alloc.free(grid);

    expect_eq(grid.len, width * height);

    let min_v: f64 = 2.0;
    let max_v: f64 = -1.0;
    for (v of grid) {
        expect_eq(v >= 0.0 && v <= 1.0, true);
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
    }
    expect_eq(max_v > min_v, true);

    // InvalidDimensions guard (width == 0 or height == 0)
    let caught_w0 = false;
    * = gen.generateGrid2D(alloc, 0, 10, 0.0, 0.0, 0.1, cfg) catch e => {
        expect_eq(e == NoiseError.InvalidDimensions, true);
        caught_w0 = true;
    };
    expect_eq(caught_w0, true);

    let caught_h0 = false;
    * = gen.generateGrid2D(alloc, 10, 0, 0.0, 0.0, 0.1, cfg) catch e => {
        expect_eq(e == NoiseError.InvalidDimensions, true);
        caught_h0 = true;
    };
    expect_eq(caught_h0, true);
}

test("cap.noise - dynamic mounting via import cap") {
    const alloc = std.testing.allocator;
    const g_dyn = cap.noise.NoiseGen.init(42);
    const g_dir = NoiseGen.init(42);
    const cfg = cap.noise.FractalConfig.init(3, 0.5, 2.0, 0.5);

    expect_eq(g_dyn.perlin2(1.25, -0.75) == g_dir.perlin2(1.25, -0.75), true);
    expect_eq(g_dyn.perlin3(0.5, 1.5, 2.5) == g_dir.perlin3(0.5, 1.5, 2.5), true);
    expect_eq(g_dyn.worley2(3.1, 4.2) == g_dir.worley2(3.1, 4.2), true);

    const f_dyn = g_dyn.fbm2(2.0, 3.0, cfg) catch unreachable;
    const r_dyn = g_dyn.ridged2(2.0, 3.0, cfg) catch unreachable;
    expect_eq(f_dyn >= -1.0 && f_dyn <= 1.0, true);
    expect_eq(r_dyn >= 0.0 && r_dyn <= 1.0, true);

    const grid = g_dyn.generateGrid2D(alloc, 8, 8, 0.0, 0.0, 0.25, cfg) catch unreachable;
    defer alloc.free(grid);
    expect_eq(grid.len, 64);
}
