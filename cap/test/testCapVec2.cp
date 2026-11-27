// cap/test/testCapVec2.cp
// Comprehensive unit tests for @cap/vec2

import { test, expect_eq } from "cap:test";
import {
    Vec2,
    new,
    zero,
    splat,
    add,
    sub,
    mul,
    scale,
    neg,
    dot,
    cross,
    lengthSq,
    length,
    distance,
    normalize,
    lerp,
    perp,
    reflect,
    project,
    rotate,
    clampLength,
    approxEq,
} from "../../packages/victor/src/index.cp";

fn nearF64(a: f64, b: f64): bool {
    let d: f64 = a - b;
    if (d < (0.0 as f64)) {
        d = -d;
    }
    return d <= (0.000000001 as f64);
}

fn main() {
    test("1. @cap/vec2 — Arithmetic (add/sub/mul/scale/neg) & dot/cross products", () => {
        const a = new(3.0 as f64, 4.0 as f64);
        const b = new(1.0 as f64, -2.0 as f64);
        const eps: f64 = 0.000000001;

        expect_eq(approxEq(add(a, b), new(4.0 as f64, 2.0 as f64), eps), true);
        expect_eq(approxEq(sub(a, b), new(2.0 as f64, 6.0 as f64), eps), true);
        expect_eq(approxEq(mul(a, b), new(3.0 as f64, -8.0 as f64), eps), true);
        expect_eq(approxEq(scale(a, 2.0 as f64), new(6.0 as f64, 8.0 as f64), eps), true);
        expect_eq(approxEq(neg(b), new(-1.0 as f64, 2.0 as f64), eps), true);
        expect_eq(approxEq(splat(5.0 as f64), new(5.0 as f64, 5.0 as f64), eps), true);
        expect_eq(nearF64(dot(a, b), -5.0 as f64), true);
        expect_eq(nearF64(cross(a, b), -10.0 as f64), true);
    });

    test("2. @cap/vec2 — Length, distance & normalize", () => {
        const v = new(3.0 as f64, 4.0 as f64);
        const eps: f64 = 0.000000001;

        expect_eq(nearF64(lengthSq(v), 25.0 as f64), true);
        expect_eq(nearF64(length(v), 5.0 as f64), true);
        expect_eq(nearF64(distance(new(1.0 as f64, 1.0 as f64), new(4.0 as f64, 5.0 as f64)), 5.0 as f64), true);

        const u = normalize(v);
        expect_eq(approxEq(u, new(0.6 as f64, 0.8 as f64), eps), true);
        expect_eq(nearF64(length(u), 1.0 as f64), true);
    });

    test("3. @cap/vec2 — Perpendicular & reflection across surface normal", () => {
        const v = new(2.0 as f64, 3.0 as f64);
        const p = perp(v);
        const eps: f64 = 0.000000001;

        expect_eq(approxEq(p, new(-3.0 as f64, 2.0 as f64), eps), true);
        expect_eq(nearF64(dot(v, p), 0.0 as f64), true);

        // Incident (1, -1) reflecting off horizontal floor normal (0, 1) -> (1, 1)
        const r = reflect(new(1.0 as f64, -1.0 as f64), new(0.0 as f64, 1.0 as f64));
        expect_eq(approxEq(r, new(1.0 as f64, 1.0 as f64), eps), true);
    });

    test("4. @cap/vec2 — Orthogonal projection & clampLength", () => {
        const eps: f64 = 0.000000001;
        const proj = project(new(3.0 as f64, 4.0 as f64), new(10.0 as f64, 0.0 as f64));
        expect_eq(approxEq(proj, new(3.0 as f64, 0.0 as f64), eps), true);

        const clamped = clampLength(new(3.0 as f64, 4.0 as f64), 2.5 as f64);
        expect_eq(approxEq(clamped, new(1.5 as f64, 2.0 as f64), eps), true);

        // Vector already shorter than max_len remains unchanged.
        const kept = clampLength(new(1.0 as f64, 1.0 as f64), 10.0 as f64);
        expect_eq(approxEq(kept, new(1.0 as f64, 1.0 as f64), eps), true);
    });

    test("5. @cap/vec2 — Rotation by 90 degrees (PI/2) & 180 degrees (PI)", () => {
        const halfPi: f64 = 1.57079632679489661923;
        const pi: f64 = 3.14159265358979323846;
        const eps: f64 = 0.000000001;

        const r90 = rotate(new(1.0 as f64, 0.0 as f64), halfPi);
        expect_eq(approxEq(r90, new(0.0 as f64, 1.0 as f64), eps), true);

        const r180 = rotate(new(2.0 as f64, -3.0 as f64), pi);
        expect_eq(approxEq(r180, new(-2.0 as f64, 3.0 as f64), eps), true);
    });

    test("6. @cap/vec2 — Linear interpolation & zero-vector safety", () => {
        const eps: f64 = 0.000000001;
        const mid = lerp(new(0.0 as f64, 10.0 as f64), new(10.0 as f64, 20.0 as f64), 0.25 as f64);
        expect_eq(approxEq(mid, new(2.5 as f64, 12.5 as f64), eps), true);

        const z = zero();
        expect_eq(approxEq(normalize(z), z, eps), true);
        expect_eq(approxEq(project(new(5.0 as f64, 5.0 as f64), z), z, eps), true);
        expect_eq(approxEq(clampLength(new(5.0 as f64, 5.0 as f64), 0.0 as f64), z, eps), true);
    });
}
