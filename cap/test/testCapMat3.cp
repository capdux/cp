// cap/test/testCapMat3.cp
// Comprehensive unit tests for @cap/mat3

import { test, expect_eq } from "cap:test";
import {
    Mat3,
    Vec2D,
    identity,
    zero,
    fromTranslation,
    fromScale,
    fromRotation,
    mul,
    transpose,
    determinant,
    isInvertible,
    inverse,
    transformPoint,
    transformDir,
    approxEq,
} from "../../packages/gl-mat3/src/index.cp";

fn nearF64(a: f64, b: f64): bool {
    let d: f64 = a - b;
    if (d < (0.0 as f64)) {
        d = -d;
    }
    return d <= (0.000000001 as f64);
}

fn main() {
    test("1. @cap/mat3 — Identity matrix properties & neutral transform", () => {
        const id = identity();
        const eps: f64 = 0.000000001;
        expect_eq(nearF64(determinant(id), 1.0 as f64), true);
        expect_eq(isInvertible(id), true);
        expect_eq(approxEq(mul(id, id), id, eps), true);

        const p = transformPoint(id, Vec2D{ x: 3.5 as f64, y: -2.0 as f64 });
        expect_eq(nearF64(p.x, 3.5 as f64), true);
        expect_eq(nearF64(p.y, -2.0 as f64), true);
    });

    test("2. @cap/mat3 — Composite translation + scale + 90-deg rotation on point", () => {
        const halfPi: f64 = 1.57079632679489661923;
        const s = fromScale(2.0 as f64, 3.0 as f64);
        const r = fromRotation(halfPi);
        const t = fromTranslation(10.0 as f64, 5.0 as f64);

        // Apply scale -> rotate 90° CCW -> translate:
        // (1, 0) --scale--> (2, 0) --rot90--> (0, 2) --trans--> (10, 7)
        const combined = mul(t, mul(r, s));
        const out = transformPoint(combined, Vec2D{ x: 1.0 as f64, y: 0.0 as f64 });
        expect_eq(nearF64(out.x, 10.0 as f64), true);
        expect_eq(nearF64(out.y, 7.0 as f64), true);
    });

    test("3. @cap/mat3 — transformDir ignores translation (w=0)", () => {
        const halfPi: f64 = 1.57079632679489661923;
        const m = mul(fromTranslation(100.0 as f64, -50.0 as f64), fromRotation(halfPi));
        const d = transformDir(m, Vec2D{ x: 1.0 as f64, y: 0.0 as f64 });
        expect_eq(nearF64(d.x, 0.0 as f64), true);
        expect_eq(nearF64(d.y, 1.0 as f64), true);
    });

    test("4. @cap/mat3 — Determinant & transpose identities", () => {
        const eps: f64 = 0.000000001;
        const m = mul(fromTranslation(4.0 as f64, -7.0 as f64), fromScale(2.0 as f64, 3.0 as f64));
        expect_eq(nearF64(determinant(m), 6.0 as f64), true);

        const mt = transpose(m);
        expect_eq(nearF64(determinant(mt), 6.0 as f64), true);
        expect_eq(nearF64(mt.m[6], 4.0 as f64), true);
        expect_eq(nearF64(mt.m[7], -7.0 as f64), true);
        expect_eq(approxEq(transpose(mt), m, eps), true);
    });

    test("5. @cap/mat3 — Invertible matrix M * inverse(M) == identity", () => {
        const eps: f64 = 0.000000001;
        const m = mul(fromTranslation(5.0 as f64, -3.0 as f64), mul(fromRotation(0.75 as f64), fromScale(2.0 as f64, 0.5 as f64)));
        expect_eq(isInvertible(m), true);

        const inv = inverse(m);
        expect_eq(approxEq(mul(m, inv), identity(), eps), true);
        expect_eq(approxEq(mul(inv, m), identity(), eps), true);
    });

    test("6. @cap/mat3 — Singular matrix safety (isInvertible == false & inverse == zero)", () => {
        const eps: f64 = 0.000000001;
        const z = zero();
        expect_eq(isInvertible(z), false);
        expect_eq(approxEq(inverse(z), z, eps), true);

        // Zero X-scale makes matrix singular (det == 0).
        const collapsed = fromScale(0.0 as f64, 5.0 as f64);
        expect_eq(nearF64(determinant(collapsed), 0.0 as f64), true);
        expect_eq(isInvertible(collapsed), false);
        expect_eq(approxEq(inverse(collapsed), z, eps), true);
    });
}
