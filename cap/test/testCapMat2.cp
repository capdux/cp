// cap/test/testCapMat2.cp
// Comprehensive unit tests for @cap/glam (Vec3, Quat & Mat4 linear algebra)

import { test, expect_eq } from "cap:test";
import { Vec3, Quat, Mat4 } from "../../packages/glam/src/index.cp";

fn matApproxEq(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < 0.0) { d = -d; }
    return d <= eps;
}

fn main() {
    test("1. @cap/glam — Vec3 arithmetic, dot, cross & normalize", () => {
        const a: Vec3 = Vec3.init(3.0, 4.0, 0.0);
        const b: Vec3 = Vec3.init(1.0, 0.0, 0.0);
        expect_eq(matApproxEq(a.length(), 5.0, 0.000001), true);
        expect_eq(matApproxEq(a.dot(b), 3.0, 0.000001), true);

        const c: Vec3 = Vec3.init(1.0, 0.0, 0.0).cross(Vec3.init(0.0, 1.0, 0.0));
        expect_eq(matApproxEq(c.x, 0.0, 0.000001), true);
        expect_eq(matApproxEq(c.y, 0.0, 0.000001), true);
        expect_eq(matApproxEq(c.z, 1.0, 0.000001), true);

        const n: Vec3 = a.normalize();
        expect_eq(matApproxEq(n.x, 0.6, 0.000001), true);
        expect_eq(matApproxEq(n.y, 0.8, 0.000001), true);
    });

    test("2. @cap/glam — Mat4 scaling, translation, rotationZ & determinant", () => {
        const id: Mat4 = Mat4.identity();
        expect_eq(matApproxEq(id.determinant(), 1.0, 0.000001), true);

        const sc: Mat4 = Mat4.scaling(Vec3.init(2.0, 3.0, 4.0));
        expect_eq(matApproxEq(sc.determinant(), 24.0, 0.000001), true);

        const tr: Mat4 = Mat4.translation(Vec3.init(10.0, 20.0, 30.0));
        const p: Vec3 = tr.mul(sc).transformPoint(Vec3.init(1.0, 2.0, 3.0));
        expect_eq(matApproxEq(p.x, 12.0, 0.000001), true);
        expect_eq(matApproxEq(p.y, 26.0, 0.000001), true);
        expect_eq(matApproxEq(p.z, 42.0, 0.000001), true);

        const q: Quat = Quat.fromAxisAngle(Vec3.init(0.0, 0.0, 1.0), 1.5707963267948966);
        const r: Vec3 = q.rotateVec3(Vec3.init(1.0, 0.0, 0.0));
        expect_eq(matApproxEq(r.x, 0.0, 0.000001), true);
        expect_eq(matApproxEq(r.y, 1.0, 0.000001), true);
    });
}
