import { test, expect_eq } from "cap:test";
import { PerlinNoise } from "../../packages/noise/src/index.cp";

fn absF64(x: f64): f64 {
    if (x < 0.0) {
        return -x;
    }
    return x;
}

fn main() {
    test("integer lattice points evaluate to exact 0.0", () => {
        let p = PerlinNoise.init(42);
        expect_eq(p.noise2D(0.0, 0.0), 0.0);
        expect_eq(p.noise2D(3.0, -7.0), 0.0);
        expect_eq(p.noise3D(1.0, 2.0, -4.0), 0.0);
    });

    test("determinism and seed isolation", () => {
        let a = PerlinNoise.init(12345);
        let b = PerlinNoise.init(12345);
        let c = PerlinNoise.init(99999);
        expect_eq(a.noise2D(1.37, 2.83), b.noise2D(1.37, 2.83));
        expect_eq(a.noise3D(0.5, 1.5, 2.5), b.noise3D(0.5, 1.5, 2.5));
        expect_eq(a.noise2D(1.37, 2.83) != c.noise2D(1.37, 2.83), true);
    });

    test("continuity and bounded output across negative and positive coords", () => {
        let p = PerlinNoise.init(7);
        let i: i32 = -20;
        let max_jump: f64 = 0.0;
        let prev: f64 = p.noise2D(-2.0, 0.35);
        while (i <= 20) {
            let x: f64 = (i as f64) * 0.1;
            let v: f64 = p.noise2D(x, 0.35);
            expect_eq(v >= -1.5 && v <= 1.5, true);
            let jump: f64 = absF64(v - prev);
            if (jump > max_jump) {
                max_jump = jump;
            }
            prev = v;
            i += 1;
        }
        expect_eq(max_jump < 0.35, true);
    });

    test("3D Perlin noise varies smoothly and stays bounded", () => {
        let p = PerlinNoise.init(2026);
        let v1 = p.noise3D(0.25, 0.75, 0.5);
        let v2 = p.noise3D(0.26, 0.75, 0.5);
        expect_eq(v1 >= -1.5 && v1 <= 1.5, true);
        expect_eq(absF64(v1 - v2) < 0.1, true);
    });

    test("fbm2D multi-octave synthesis and zero-octave safety", () => {
        let p = PerlinNoise.init(88);
        expect_eq(p.fbm2D(1.2, 3.4, 0, 2.0, 0.5), 0.0);
        let f1 = p.fbm2D(1.23, 4.56, 1, 2.0, 0.5);
        expect_eq(absF64(f1 - p.noise2D(1.23, 4.56)) < 0.000000001, true);
        let f4 = p.fbm2D(1.23, 4.56, 4, 2.0, 0.5);
        expect_eq(f4 >= -1.5 && f4 <= 1.5, true);
    });

    test("ridged2D stays in [0, 1] and handles zero octaves", () => {
        let p = PerlinNoise.init(555);
        expect_eq(p.ridged2D(0.5, 0.5, 0), 0.0);
        let r = p.ridged2D(2.34, -1.78, 5);
        expect_eq(r >= 0.0 && r <= 1.0, true);
    });
}
