// cap/test/testCapGlam.cp
// Comprehensive unit tests for @cap/glam (Rust glam parity)

import { test, expect_eq } from "cap:test";
import { Vec3, Quat, Mat4 } from "../../packages/@cap/glam/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff: f64 = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn vecApprox(a: Vec3, b: Vec3, eps: f64) -> bool {
    return approxEq(a.x, b.x, eps) && approxEq(a.y, b.y, eps) && approxEq(a.z, b.z, eps);
}

fn main() {
    test("1. @cap/glam — Vec3 dot, right-hand cross, normalize & distance", () => {
        const vx: Vec3 = Vec3.init(1.0, 0.0, 0.0);
        const vy: Vec3 = Vec3.init(0.0, 1.0, 0.0);
        const vz: Vec3 = vx.cross(vy);

        expect_eq(vecApprox(vz, Vec3.init(0.0, 0.0, 1.0), 0.000000001), true);
        expect_eq(approxEq(vx.dot(vy), 0.0, 0.000000001), true);
        expect_eq(approxEq(vx.dot(vx), 1.0, 0.000000001), true);

        const v: Vec3 = Vec3.init(3.0, 4.0, 0.0);
        expect_eq(approxEq(v.length(), 5.0, 0.000000001), true);
        expect_eq(vecApprox(v.normalize(), Vec3.init(0.6, 0.8, 0.0), 0.000000001), true);

        const p1: Vec3 = Vec3.init(1.0, 2.0, 3.0);
        const p2: Vec3 = Vec3.init(4.0, 6.0, 3.0);
        expect_eq(approxEq(p1.distance(p2), 5.0, 0.000000001), true);
        expect_eq(vecApprox(p1.lerp(p2, 0.5), Vec3.init(2.5, 4.0, 3.0), 0.000000001), true);
    });

    test("2. @cap/glam — Quat 90-deg Z rotation & composed quaternion multiplication", () => {
        const half_pi: f64 = 1.5707963267948966;
        const axis_z: Vec3 = Vec3.init(0.0, 0.0, 1.0);
        const q90: Quat = Quat.fromAxisAngle(axis_z, half_pi);

        const r1: Vec3 = q90.rotateVec3(Vec3.init(1.0, 0.0, 0.0));
        expect_eq(vecApprox(r1, Vec3.init(0.0, 1.0, 0.0), 0.000000001), true);

        // Composing two 90-deg Z rotations yields 180-deg rotation: (1,0,0) -> (-1,0,0)
        const q180: Quat = q90.mul(q90);
        const r2: Vec3 = q180.rotateVec3(Vec3.init(1.0, 0.0, 0.0));
        expect_eq(vecApprox(r2, Vec3.init(-1.0, 0.0, 0.0), 0.000000001), true);
    });

    test("3. @cap/glam — Quat conjugate inverse rotation restores original vector", () => {
        const axis: Vec3 = Vec3.init(1.0, 2.0, 3.0);
        const q: Quat = Quat.fromAxisAngle(axis, 1.2345).normalize();
        const q_inv: Quat = q.conjugate();

        const orig: Vec3 = Vec3.init(5.0, -3.0, 2.5);
        const rotated: Vec3 = q.rotateVec3(orig);
        const restored: Vec3 = q_inv.rotateVec3(rotated);

        expect_eq(vecApprox(restored, orig, 0.000000001), true);
    });

    test("4. @cap/glam — Mat4 composed translation * rotationZ * scaling transform", () => {
        const half_pi: f64 = 1.5707963267948966;
        const s: Mat4 = Mat4.scaling(Vec3.init(2.0, 3.0, 1.0));
        const r: Mat4 = Mat4.rotationZ(half_pi);
        const t: Mat4 = Mat4.translation(Vec3.init(10.0, 20.0, 5.0));

        // T * R * S applied to (1, 0, 0):
        // S -> (2, 0, 0), R(90deg) -> (0, 2, 0), T -> (10, 22, 5)
        const trs: Mat4 = t.mul(r).mul(s);
        const out: Vec3 = trs.transformPoint(Vec3.init(1.0, 0.0, 0.0));
        expect_eq(vecApprox(out, Vec3.init(10.0, 22.0, 5.0), 0.000000001), true);
    });

    test("5. @cap/glam — Mat4 determinant & identity multiplication invariance", () => {
        const id: Mat4 = Mat4.identity();
        expect_eq(approxEq(id.determinant(), 1.0, 0.000000001), true);

        const s: Mat4 = Mat4.scaling(Vec3.init(2.0, 3.0, 4.0));
        expect_eq(approxEq(s.determinant(), 24.0, 0.000000001), true);

        const r: Mat4 = Mat4.rotationZ(0.75);
        const t: Mat4 = Mat4.translation(Vec3.init(5.0, -2.0, 8.0));
        const m: Mat4 = t.mul(r).mul(s);
        // Rigid transform preserves determinant of scaling: det(T*R*S) = 1 * 1 * 24 = 24
        expect_eq(approxEq(m.determinant(), 24.0, 0.000000001), true);

        const m_id: Mat4 = m.mul(id);
        const id_m: Mat4 = id.mul(m);
        let i: usize = 0;
        while (i < 16) {
            expect_eq(approxEq(m_id.m[i], m.m[i], 0.000000001), true);
            expect_eq(approxEq(id_m.m[i], m.m[i], 0.000000001), true);
            i += 1;
        }
    });

    test("6. @cap/glam — zero vector & zero quaternion normalize safely without NaN", () => {
        const zero: Vec3 = Vec3.init(0.0, 0.0, 0.0);
        const zn: Vec3 = zero.normalize();
        // NaN != NaN, so checking equality with 0.0 proves no NaN
        expect_eq(zn.x == 0.0 && zn.y == 0.0 && zn.z == 0.0, true);

        const qz = Quat { x: 0.0, y: 0.0, z: 0.0, w: 0.0 };
        const qn: Quat = qz.normalize();
        expect_eq(qn.x == 0.0 && qn.y == 0.0 && qn.z == 0.0 && qn.w == 1.0, true);

        const q_zero_axis: Quat = Quat.fromAxisAngle(zero, 1.0);
        expect_eq(q_zero_axis.w == 1.0 && q_zero_axis.x == 0.0, true);
    });
}
