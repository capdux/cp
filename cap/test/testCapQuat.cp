import { test, expect_eq } from "cap:test";
import {
    PI,
    HALF_PI,
    Quat,
    Point3,
    absF64,
    sqrtF64,
    new,
    identity,
    fromAxisAngle,
    fromEulerXYZ,
    mul,
    conjugate,
    inverse,
    dot,
    length,
    normalize,
    nlerp,
    slerp,
    rotatePoint,
    approxEq,
} from "../../packages/@cap/quat/src/index.cp";

const EPS: f64 = 0.000000001;

fn pointApproxEq(a: Point3, b: Point3, eps: f64): bool {
    return absF64(a.x - b.x) <= eps &&
        absF64(a.y - b.y) <= eps &&
        absF64(a.z - b.z) <= eps;
}

fn main() {
    test("1. @cap/quat — Identity quaternion leaves points & quats unchanged", () => {
        const id = identity();
        expect_eq(approxEq(id, new(0.0, 0.0, 0.0, 1.0), EPS), true);
        expect_eq(absF64(length(id) - 1.0) <= EPS, true);

        const p = Point3 { x: 3.5, y: -2.0, z: 7.25 };
        const rp = rotatePoint(id, p);
        expect_eq(pointApproxEq(rp, p, EPS), true);

        const q = fromAxisAngle(1.0, 2.0, 3.0, 0.7);
        expect_eq(approxEq(mul(id, q), q, EPS), true);
        expect_eq(approxEq(mul(q, id), q, EPS), true);
    });

    test("2. @cap/quat — 90-degree Z-axis rotation maps (1,0,0) -> (0,1,0)", () => {
        const qz90 = fromAxisAngle(0.0, 0.0, 1.0, HALF_PI);
        expect_eq(absF64(length(qz90) - 1.0) <= EPS, true);

        const p = Point3 { x: 1.0, y: 0.0, z: 0.0 };
        const rp = rotatePoint(qz90, p);
        expect_eq(pointApproxEq(rp, Point3 { x: 0.0, y: 1.0, z: 0.0 }, EPS), true);

        const p2 = Point3 { x: 0.0, y: 1.0, z: 0.0 };
        const rp2 = rotatePoint(qz90, p2);
        expect_eq(pointApproxEq(rp2, Point3 { x: -1.0, y: 0.0, z: 0.0 }, EPS), true);
    });

    test("3. @cap/quat — Quaternion composition: two 90-deg rotations equal 180-deg", () => {
        const q90 = fromAxisAngle(0.0, 0.0, 1.0, HALF_PI);
        const q180_composed = mul(q90, q90);
        const q180_direct = fromAxisAngle(0.0, 0.0, 1.0, PI);
        expect_eq(approxEq(q180_composed, q180_direct, EPS), true);

        const p = Point3 { x: 2.0, y: 3.0, z: -5.0 };
        const rp = rotatePoint(q180_composed, p);
        expect_eq(pointApproxEq(rp, Point3 { x: -2.0, y: -3.0, z: -5.0 }, EPS), true);
    });

    test("4. @cap/quat — Conjugate & inverse satisfy q * inv(q) == identity", () => {
        const q_unit = fromAxisAngle(1.0, -2.0, 0.5, 1.23);
        const conj = conjugate(q_unit);
        expect_eq(approxEq(mul(q_unit, conj), identity(), EPS), true);
        expect_eq(approxEq(mul(conj, q_unit), identity(), EPS), true);

        const q_scaled = new(q_unit.x * 3.0, q_unit.y * 3.0, q_unit.z * 3.0, q_unit.w * 3.0);
        const inv = inverse(q_scaled);
        expect_eq(approxEq(mul(q_scaled, inv), identity(), EPS), true);
        expect_eq(approxEq(mul(inv, q_scaled), identity(), EPS), true);
        expect_eq(absF64(dot(q_unit, q_unit) - 1.0) <= EPS, true);
    });

    test("5. @cap/quat — SLERP & NLERP midpoint between 0 deg and 90 deg is 45 deg", () => {
        const q0 = identity();
        const q90 = fromAxisAngle(0.0, 0.0, 1.0, HALF_PI);
        const q45_expected = fromAxisAngle(0.0, 0.0, 1.0, HALF_PI * 0.5);

        const s_mid = slerp(q0, q90, 0.5);
        const n_mid = nlerp(q0, q90, 0.5);
        expect_eq(approxEq(s_mid, q45_expected, EPS), true);
        expect_eq(approxEq(n_mid, q45_expected, EPS), true);

        const inv_sqrt2: f64 = 1.0 / sqrtF64(2.0);
        const rp = rotatePoint(s_mid, Point3 { x: 1.0, y: 0.0, z: 0.0 });
        expect_eq(pointApproxEq(rp, Point3 { x: inv_sqrt2, y: inv_sqrt2, z: 0.0 }, EPS), true);

        // Shortest-path test when q90 is negated
        const q90_neg = new(-q90.x, -q90.y, -q90.z, -q90.w);
        const s_mid_neg = slerp(q0, q90_neg, 0.5);
        expect_eq(approxEq(s_mid_neg, q45_expected, EPS), true);
    });

    test("6. @cap/quat — fromEulerXYZ matches axis-angle & zero-length safety", () => {
        const ex = fromEulerXYZ(HALF_PI, 0.0, 0.0);
        const ax = fromAxisAngle(1.0, 0.0, 0.0, HALF_PI);
        expect_eq(approxEq(ex, ax, EPS), true);

        const ey = fromEulerXYZ(0.0, HALF_PI, 0.0);
        const ay = fromAxisAngle(0.0, 1.0, 0.0, HALF_PI);
        expect_eq(approxEq(ey, ay, EPS), true);

        const ez = fromEulerXYZ(0.0, 0.0, HALF_PI);
        const az = fromAxisAngle(0.0, 0.0, 1.0, HALF_PI);
        expect_eq(approxEq(ez, az, EPS), true);

        // Combined XYZ Euler matches qx * qy * qz
        const e_all = fromEulerXYZ(0.3, -0.5, 0.8);
        const q_all = mul(
            mul(fromAxisAngle(1.0, 0.0, 0.0, 0.3), fromAxisAngle(0.0, 1.0, 0.0, -0.5)),
            fromAxisAngle(0.0, 0.0, 1.0, 0.8)
        );
        expect_eq(approxEq(e_all, q_all, EPS), true);

        // Zero-length safety
        const zero_q = new(0.0, 0.0, 0.0, 0.0);
        expect_eq(approxEq(normalize(zero_q), identity(), EPS), true);
        expect_eq(approxEq(inverse(zero_q), identity(), EPS), true);
        expect_eq(approxEq(fromAxisAngle(0.0, 0.0, 0.0, 1.0), identity(), EPS), true);
    });
}
