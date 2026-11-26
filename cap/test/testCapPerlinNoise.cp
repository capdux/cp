import { test, expect_eq } from "cap:test";
import {
    fade,
    noise1D,
    noise2D,
    noise3D,
    fbm2D,
    ridge2D,
} from "../../packages/@cap/perlin-noise/src/index.cp";

const EPS: f64 = 0.000001;

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn main() {
    test("1. @cap/perlin-noise — Determinism & fade() curve invariants", () => {
        expect_eq(absF64(fade(0.0) - 0.0) <= EPS, true);
        expect_eq(absF64(fade(0.5) - 0.5) <= EPS, true);
        expect_eq(absF64(fade(1.0) - 1.0) <= EPS, true);

        expect_eq(noise1D(3.1415, 7), noise1D(3.1415, 7));
        expect_eq(noise2D(12.34, -56.78, 99), noise2D(12.34, -56.78, 99));
        expect_eq(noise3D(1.2, 3.4, 5.6, 12345), noise3D(1.2, 3.4, 5.6, 12345));
    });

    test("2. @cap/perlin-noise — Integer lattice coordinates evaluate to 0", () => {
        expect_eq(absF64(noise1D(0.0, 0)) <= EPS, true);
        expect_eq(absF64(noise1D(5.0, 42)) <= EPS, true);
        expect_eq(absF64(noise1D(-11.0, 42)) <= EPS, true);

        expect_eq(absF64(noise2D(0.0, 0.0, 0)) <= EPS, true);
        expect_eq(absF64(noise2D(7.0, -3.0, 17)) <= EPS, true);

        expect_eq(absF64(noise3D(0.0, 0.0, 0.0, 0)) <= EPS, true);
        expect_eq(absF64(noise3D(-4.0, 9.0, 2.0, 88)) <= EPS, true);
    });

    test("3. @cap/perlin-noise — Different seeds produce distinct noise fields", () => {
        let diff_count: usize = 0;
        let i: usize = 0;
        while (i < 16) {
            const x: f64 = (i as f64) * 0.37 + 0.13;
            const y: f64 = (i as f64) * 0.53 + 0.29;
            const a: f64 = noise2D(x, y, 1);
            const b: f64 = noise2D(x, y, 2);
            if (absF64(a - b) > 0.001) {
                diff_count += 1;
            }
            i += 1;
        }
        expect_eq(diff_count >= 12, true);
    });

    test("4. @cap/perlin-noise — Spatial continuity: |n(x+0.01) - n(x)| < 0.1", () => {
        let i: usize = 0;
        while (i < 40) {
            const x: f64 = (i as f64) * 0.23 - 4.0;
            const y: f64 = (i as f64) * 0.17 + 1.5;
            const d1: f64 = absF64(noise1D(x + 0.01, 5) - noise1D(x, 5));
            const d2: f64 = absF64(noise2D(x + 0.01, y, 5) - noise2D(x, y, 5));
            const d3: f64 = absF64(noise3D(x + 0.01, y, 0.4, 5) - noise3D(x, y, 0.4, 5));
            expect_eq(d1 < 0.1, true);
            expect_eq(d2 < 0.1, true);
            expect_eq(d3 < 0.1, true);
            i += 1;
        }
    });

    test("5. @cap/perlin-noise — 2D/3D outputs stay within [-1.05, 1.05]", () => {
        let i: usize = 0;
        while (i < 100) {
            const x: f64 = (i as f64) * 0.31 - 15.0;
            const y: f64 = (i as f64) * 0.47 - 20.0;
            const z: f64 = (i as f64) * 0.19 - 8.0;
            const v2: f64 = noise2D(x, y, 77);
            const v3: f64 = noise3D(x, y, z, 77);
            expect_eq(v2 >= -1.05 && v2 <= 1.05, true);
            expect_eq(v3 >= -1.05 && v3 <= 1.05, true);
            i += 1;
        }
    });

    test("6. @cap/perlin-noise — fbm2D & ridge2D bounds, zero-octave & negative coords", () => {
        expect_eq(fbm2D(1.5, 2.5, 3, 0, 2.0, 0.5), 0.0);
        expect_eq(ridge2D(1.5, 2.5, 3, 0), 0.0);

        let i: usize = 0;
        while (i < 50) {
            const x: f64 = -100.25 + (i as f64) * 0.73;
            const y: f64 = -250.75 + (i as f64) * 0.61;
            const f: f64 = fbm2D(x, y, 2026, 5, 2.0, 0.5);
            const r: f64 = ridge2D(x, y, 2026, 4);
            expect_eq(f >= -1.05 && f <= 1.05, true);
            expect_eq(r >= 0.0 && r <= 1.05, true);
            i += 1;
        }
    });
}
