import { test, expect_eq } from "cap:test";
import {
    PI,
    Vec3D,
    Mat4,
    identity,
    fromTranslation,
    fromScale,
    fromRotationX,
    fromRotationY,
    fromRotationZ,
    orthographic,
    perspective,
    lookAt,
    mul,
    transpose,
    transformPoint,
    transformDir,
    approxEq,
} from "../../packages/gl-mat4/src/index.cp";

fn main() {
    test("1. @cap/mat4 — identity & translation + scale composition", () => {
        let eps: f64 = 0.00001;
        let p: Vec3D = Vec3D.new(1.0, 2.0, 3.0);
        expect_eq(transformPoint(identity(), p).approxEq(p, eps), true);

        let t_mat: Mat4 = fromTranslation(10.0, 20.0, 30.0);
        let s_mat: Mat4 = fromScale(2.0, 3.0, 4.0);
        // Apply scale first, then translation: T * S * p
        let ts: Mat4 = mul(t_mat, s_mat);
        let out: Vec3D = transformPoint(ts, p);
        expect_eq(out.approxEq(Vec3D.new(12.0, 26.0, 42.0), eps), true);
    });

    test("2. @cap/mat4 — rotation around X, Y & Z axes", () => {
        let eps: f64 = 0.00001;
        let rz: Mat4 = fromRotationZ(PI * 0.5);
        expect_eq(transformPoint(rz, Vec3D.new(1.0, 0.0, 0.0)).approxEq(Vec3D.new(0.0, 1.0, 0.0), eps), true);

        let rx: Mat4 = fromRotationX(PI * 0.5);
        expect_eq(transformPoint(rx, Vec3D.new(0.0, 1.0, 0.0)).approxEq(Vec3D.new(0.0, 0.0, 1.0), eps), true);

        let ry: Mat4 = fromRotationY(PI * 0.5);
        expect_eq(transformPoint(ry, Vec3D.new(0.0, 0.0, 1.0)).approxEq(Vec3D.new(1.0, 0.0, 0.0), eps), true);
    });

    test("3. @cap/mat4 — transpose involution transpose(transpose(M)) == M", () => {
        let eps: f64 = 0.00001;
        let m: Mat4 = mul(fromTranslation(3.0, -4.0, 5.0), fromRotationZ(0.7));
        let tt: Mat4 = transpose(transpose(m));
        expect_eq(approxEq(tt, m, eps), true);

        let tr: Mat4 = transpose(fromTranslation(1.0, 2.0, 3.0));
        expect_eq(tr.m[12] == 1.0 && tr.m[13] == 2.0 && tr.m[14] == 3.0, true);
    });

    test("4. @cap/mat4 — orthographic projection maps frustum center to origin", () => {
        let eps: f64 = 0.00001;
        let ortho: Mat4 = orthographic(-10.0, 10.0, -5.0, 5.0, 1.0, 101.0);
        // Frustum center in right-handed camera space: (0, 0, -(near+far)/2) = (0, 0, -51)
        let center: Vec3D = transformPoint(ortho, Vec3D.new(0.0, 0.0, -51.0));
        expect_eq(center.approxEq(Vec3D.new(0.0, 0.0, 0.0), eps), true);

        let corner: Vec3D = transformPoint(ortho, Vec3D.new(10.0, 5.0, -1.0));
        expect_eq(corner.approxEq(Vec3D.new(1.0, 1.0, -1.0), eps), true);
    });

    test("5. @cap/mat4 — perspective projection near/far plane & aspect", () => {
        let eps: f64 = 0.00001;
        let persp: Mat4 = perspective(PI * 0.5, 2.0, 1.0, 100.0);
        // Point on optical axis at near plane z = -1 maps to NDC (0, 0, -1)
        let p_near: Vec3D = transformPoint(persp, Vec3D.new(0.0, 0.0, -1.0));
        expect_eq(p_near.approxEq(Vec3D.new(0.0, 0.0, -1.0), eps), true);

        // Point on optical axis at far plane z = -100 maps to NDC (0, 0, 1)
        let p_far: Vec3D = transformPoint(persp, Vec3D.new(0.0, 0.0, -100.0));
        expect_eq(p_far.approxEq(Vec3D.new(0.0, 0.0, 1.0), eps), true);
    });

    test("6. @cap/mat4 — lookAt maps eye to origin & transformDir ignores translation", () => {
        let eps: f64 = 0.00001;
        let eye: Vec3D = Vec3D.new(0.0, 2.0, 5.0);
        let target: Vec3D = Vec3D.new(0.0, 2.0, 0.0);
        let up: Vec3D = Vec3D.new(0.0, 1.0, 0.0);

        let view: Mat4 = lookAt(eye, target, up);
        expect_eq(transformPoint(view, eye).approxEq(Vec3D.new(0.0, 0.0, 0.0), eps), true);
        expect_eq(transformPoint(view, target).approxEq(Vec3D.new(0.0, 0.0, -5.0), eps), true);

        let t_mat: Mat4 = fromTranslation(100.0, -200.0, 300.0);
        let dir: Vec3D = Vec3D.new(1.0, 0.0, 0.0);
        expect_eq(transformDir(t_mat, dir).approxEq(dir, eps), true);
    });
}
