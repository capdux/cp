// Source: cp/cap/test/test_quat_production_deep.cp (Production deep tests for cap:quat)

import std from "std";
import { test, expect_eq } from "cap:test";
import { Vec3, EulerAngles, Mat3, Quat } from "cap:quat";

fn absDiff(a: f64, b: f64): f64 {
    if (a >= b) {
        return a - b;
    }
    return b - a;
}

fn vec3Close(a: Vec3, b: Vec3, tol: f64): bool {
    return absDiff(a.x, b.x) < tol && absDiff(a.y, b.y) < tol && absDiff(a.z, b.z) < tol;
}

test("1. Principal-axis 90/180 deg rotations and composite rotation equivalence to Mat3.mulVec3 (< 1e-12)", () => {
    const half_pi: f64 = std.math.pi * 0.5;
    const vx: Vec3 = Vec3 { x: 1.0, y: 0.0, z: 0.0 };
    const vy: Vec3 = Vec3 { x: 0.0, y: 1.0, z: 0.0 };
    const vz: Vec3 = Vec3 { x: 0.0, y: 0.0, z: 1.0 };

    // +90 deg around Z sends X -> Y; +90 deg around X sends Y -> Z; +180 deg around Y sends X -> -X.
    if (qz := Quat.fromAxisAngle(vz, half_pi)) {
        expect_eq(vec3Close(qz.rotateVec3(vx), vy, 1.0e-12), true);
        if (qx := Quat.fromAxisAngle(vx, half_pi)) {
            expect_eq(vec3Close(qx.rotateVec3(vy), vz, 1.0e-12), true);
            if (qy180 := Quat.fromAxisAngle(vy, std.math.pi)) {
                expect_eq(vec3Close(qy180.rotateVec3(vx), Vec3 { x: -1.0, y: 0.0, z: 0.0 }, 1.0e-12), true);

                // Composite rotation q = qy180 * qx * qz must match toMat3().mulVec3(p) and have det(M) == 1.
                const comp: Quat = qy180.mul(qx).mul(qz);
                const mat: Mat3 = comp.toMat3();
                expect_eq(absDiff(mat.det(), 1.0) < 1.0e-12, true);

                const p: Vec3 = Vec3 { x: 1.25, y: -3.5, z: 2.75 };
                const r_quat: Vec3 = comp.rotateVec3(p);
                const r_mat: Vec3 = mat.mulVec3(p);
                expect_eq(vec3Close(r_quat, r_mat, 1.0e-12), true);

                // Inverse rotation recovers p exactly.
                if (inv_q := comp.inverse()) {
                    expect_eq(vec3Close(inv_q.rotateVec3(r_quat), p, 1.0e-12), true);
                    expect_eq(vec3Close(mat.transpose().mulVec3(r_mat), p, 1.0e-12), true);
                } else {
                    expect_eq(true, false);
                }
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("2. fromEulerZYX <-> toEulerZYX round-trip including +-pi/2 gimbal-lock singularities", () => {
    const r0: f64 = 0.42;
    const p0: f64 = -0.31;
    const y0: f64 = 1.15;
    const q: Quat = Quat.fromEulerZYX(r0, p0, y0);
    const e: EulerAngles = q.toEulerZYX();
    expect_eq(absDiff(e.roll, r0) < 1.0e-12, true);
    expect_eq(absDiff(e.pitch, p0) < 1.0e-12, true);
    expect_eq(absDiff(e.yaw, y0) < 1.0e-12, true);

    // Gimbal lock at pitch = +pi/2 and pitch = -pi/2: reconstructed quaternion must represent the exact same rotation!
    const half_pi: f64 = std.math.pi * 0.5;
    const probe: Vec3 = Vec3 { x: 0.7, y: -1.3, z: 2.1 };

    const q_north: Quat = Quat.fromEulerZYX(0.35, half_pi, 0.85);
    const e_north: EulerAngles = q_north.toEulerZYX();
    const q_north_re: Quat = Quat.fromEulerZYX(e_north.roll, e_north.pitch, e_north.yaw);
    expect_eq(absDiff(e_north.pitch, half_pi) < 1.0e-12, true);
    expect_eq(q_north.angleTo(q_north_re) < 1.0e-12, true);
    expect_eq(vec3Close(q_north.rotateVec3(probe), q_north_re.rotateVec3(probe), 1.0e-12), true);

    const q_south: Quat = Quat.fromEulerZYX(-0.6, 0.0 - half_pi, 1.2);
    const e_south: EulerAngles = q_south.toEulerZYX();
    const q_south_re: Quat = Quat.fromEulerZYX(e_south.roll, e_south.pitch, e_south.yaw);
    expect_eq(absDiff(e_south.pitch, 0.0 - half_pi) < 1.0e-12, true);
    expect_eq(q_south.angleTo(q_south_re) < 1.0e-12, true);
    expect_eq(vec3Close(q_south.rotateVec3(probe), q_south_re.rotateVec3(probe), 1.0e-12), true);
});

test("3. Shepperd fromMat3(q.toMat3()) round-trip across all 4 trace/diagonal branches (double-cover aware)", () => {
    const samples: [4]Quat = [
        Quat.fromEulerZYX(0.2, 0.3, 0.4),                        // tr > 0
        Quat.fromEulerZYX(std.math.pi * 0.95, 0.05, 0.05),       // m00 dominant
        Quat.fromEulerZYX(0.05, std.math.pi * 0.95, 0.05),       // m11 dominant
        Quat.fromEulerZYX(0.05, 0.05, std.math.pi * 0.95),       // m22 dominant
    ];
    const probe: Vec3 = Vec3 { x: -2.0, y: 1.5, z: 0.8 };

    let i: usize = 0;
    while (i < 4) {
        const q: Quat = samples[i];
        const m: Mat3 = q.toMat3();
        const recovered: Quat = Quat.fromMat3(m);
        // q and -q represent the same SO(3) rotation; angleTo must be 0.
        expect_eq(q.angleTo(recovered) < 1.0e-12, true);
        expect_eq(vec3Close(q.rotateVec3(probe), recovered.rotateVec3(probe), 1.0e-12), true);
        i += 1;
    }
});

test("4. Shortest-arc SLERP & NLERP: midpoint halves angle, q vs -q has zero distance, near-zero angle fallback", () => {
    const axis: Vec3 = Vec3 { x: 0.0, y: 1.0, z: 0.0 };
    if (q0 := Quat.fromAxisAngle(axis, 0.2)) {
        if (q1 := Quat.fromAxisAngle(axis, 1.4)) {
            // Double-cover test: negate q1 -> slerp must still take shortest arc (delta = 1.2 rad, midpoint = 0.6 rad).
            const q1_neg: Quat = q1.negate();
            expect_eq(q1.angleTo(q1_neg) < 1.0e-12, true);

            const mid: Quat = Quat.slerp(q0, q1_neg, 0.5);
            expect_eq(absDiff(q0.angleTo(mid), 0.6) < 1.0e-12, true);
            expect_eq(absDiff(mid.angleTo(q1), 0.6) < 1.0e-12, true);
            expect_eq(absDiff(mid.norm(), 1.0) < 1.0e-12, true);

            // Quarter point at t = 0.25 must be constant angular velocity (0.3 rad).
            const qtr: Quat = Quat.slerp(q0, q1_neg, 0.25);
            expect_eq(absDiff(q0.angleTo(qtr), 0.3) < 1.0e-12, true);

            // Tiny angle triggers NLERP fallback smoothly without NaN or div-by-zero.
            if (q_near := Quat.fromAxisAngle(axis, 0.200001)) {
                const tiny_mid: Quat = Quat.slerp(q0, q_near, 0.5);
                expect_eq(absDiff(tiny_mid.norm(), 1.0) < 1.0e-12, true);
                expect_eq(absDiff(q0.angleTo(tiny_mid), 0.0000005) < 1.0e-9, true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. fromToRotation (general, parallel, 180 deg antiparallel) and zero-vector / zero-norm rejection", () => {
    const a: Vec3 = Vec3 { x: 1.0, y: 2.0, z: 3.0 };
    const b: Vec3 = Vec3 { x: -2.0, y: 0.5, z: 1.5 };

    // General vectors.
    if (q_ab := Quat.fromToRotation(a, b)) {
        if (ua := a.normalize()) {
            if (ub := b.normalize()) {
                expect_eq(vec3Close(q_ab.rotateVec3(ua), ub, 1.0e-12), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Parallel (same direction) -> identity.
    if (q_same := Quat.fromToRotation(a, a.scale(5.0))) {
        expect_eq(q_same.angleTo(Quat.identity()) < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }

    // 180 deg antiparallel along both X axis and general axis -> exact negation with unit norm.
    const x_pos: Vec3 = Vec3 { x: 1.0, y: 0.0, z: 0.0 };
    const x_neg: Vec3 = Vec3 { x: -1.0, y: 0.0, z: 0.0 };
    if (q_180x := Quat.fromToRotation(x_pos, x_neg)) {
        expect_eq(absDiff(q_180x.norm(), 1.0) < 1.0e-12, true);
        expect_eq(vec3Close(q_180x.rotateVec3(x_pos), x_neg, 1.0e-12), true);
    } else {
        expect_eq(true, false);
    }
    if (q_180a := Quat.fromToRotation(a, a.scale(-1.0))) {
        if (ua := a.normalize()) {
            expect_eq(vec3Close(q_180a.rotateVec3(ua), ua.scale(-1.0), 1.0e-12), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Zero axis / zero norm rejections.
    const zero_v: Vec3 = Vec3 { x: 0.0, y: 0.0, z: 0.0 };
    const zero_q: Quat = Quat { w: 0.0, x: 0.0, y: 0.0, z: 0.0 };
    if (_ := zero_v.normalize()) { expect_eq(true, false); }
    if (_ := Quat.fromAxisAngle(zero_v, 1.0)) { expect_eq(true, false); }
    if (_ := Quat.fromToRotation(zero_v, a)) { expect_eq(true, false); }
    if (_ := Quat.fromToRotation(a, zero_v)) { expect_eq(true, false); }
    if (_ := zero_q.normalize()) { expect_eq(true, false); }
    if (_ := zero_q.inverse()) { expect_eq(true, false); }
});
