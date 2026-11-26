import { test, expect_eq } from "cap:test";
import { Vector2, Vector3, RowMatrix2, RowMatrix3, RowMatrix4 } from "../../packages/@cap/mint/src/index.cp";
import {
    vec2Norm, vec3Norm, vec3Normalize, vec3Lerp, vec3Distance,
    mat2Mul, mat2Inverse, mat3Det, mat3Mul, mat3Inverse,
    mat4Mul, mat4Translation, mat4Scaling, mat4TransformPoint
} from "../../packages/@cap/nalgebra/src/index.cp";

fn naClose(a: f64, b: f64): bool {
    let d = if (a > b) { a - b } else { b - a };
    return d < 1.0e-9;
}

fn main() {
    test("1. @cap/nalgebra — vector norms, normalize, lerp & distance", () => {
        expect_eq(naClose(vec2Norm(Vector2.new(3.0, 4.0)), 5.0), true);
        let u = vec3Normalize(Vector3.new(0.0, 3.0, 4.0)) ?? Vector3.new(0.0, 0.0, 0.0);
        expect_eq(naClose(u.y, 0.6), true);
        expect_eq(naClose(u.z, 0.8), true);
        expect_eq(vec3Normalize(Vector3.new(0.0, 0.0, 0.0)) == null, true);

        let mid = vec3Lerp(Vector3.new(0.0, 0.0, 0.0), Vector3.new(10.0, 20.0, 30.0), 0.5);
        expect_eq(naClose(mid.x, 5.0), true);
        expect_eq(naClose(mid.y, 10.0), true);
        expect_eq(naClose(vec3Distance(Vector3.new(1.0, 2.0, 3.0), Vector3.new(4.0, 6.0, 3.0)), 5.0), true);
    });

    test("2. @cap/nalgebra — mat2Mul & mat2Inverse satisfy M * M^-1 = I", () => {
        let m = RowMatrix2 { x: Vector2.new(4.0, 7.0), y: Vector2.new(2.0, 6.0) };
        let inv = mat2Inverse(m) ?? RowMatrix2.identity();
        let id = mat2Mul(m, inv);
        expect_eq(naClose(id.x.x, 1.0), true);
        expect_eq(naClose(id.x.y, 0.0), true);
        expect_eq(naClose(id.y.x, 0.0), true);
        expect_eq(naClose(id.y.y, 1.0), true);
    });

    test("3. @cap/nalgebra — singular mat2 & mat3 return null inverse", () => {
        let sing2 = RowMatrix2 { x: Vector2.new(2.0, 4.0), y: Vector2.new(1.0, 2.0) };
        expect_eq(mat2Inverse(sing2) == null, true);
        let sing3 = RowMatrix3 {
            x: Vector3.new(1.0, 2.0, 3.0),
            y: Vector3.new(2.0, 4.0, 6.0),
            z: Vector3.new(0.0, 0.0, 1.0),
        };
        expect_eq(mat3Inverse(sing3) == null, true);
    });

    test("4. @cap/nalgebra — mat3Det & mat3Inverse satisfy M * M^-1 = I", () => {
        let m = RowMatrix3 {
            x: Vector3.new(1.0, 2.0, 3.0),
            y: Vector3.new(0.0, 1.0, 4.0),
            z: Vector3.new(5.0, 6.0, 0.0),
        };
        expect_eq(naClose(mat3Det(m), 1.0), true);
        let inv = mat3Inverse(m) ?? RowMatrix3.identity();
        let id = mat3Mul(m, inv);
        expect_eq(naClose(id.x.x, 1.0), true);
        expect_eq(naClose(id.y.y, 1.0), true);
        expect_eq(naClose(id.z.z, 1.0), true);
        expect_eq(naClose(id.x.y, 0.0), true);
    });

    test("5. @cap/nalgebra — mat4Translation + mat4Scaling composition", () => {
        let t = mat4Translation(10.0, 20.0, 30.0);
        let s = mat4Scaling(2.0, 3.0, 4.0);
        let m = mat4Mul(t, s); // scale then translate
        let p = mat4TransformPoint(m, Vector3.new(1.0, 2.0, 3.0));
        expect_eq(naClose(p.x, 12.0), true);
        expect_eq(naClose(p.y, 26.0), true);
        expect_eq(naClose(p.z, 42.0), true);
    });

    test("6. @cap/nalgebra — mat4Mul with identity is no-op", () => {
        let t = mat4Translation(-5.0, 3.5, 9.0);
        let r = mat4Mul(RowMatrix4.identity(), t);
        expect_eq(naClose(r.x.w, -5.0), true);
        expect_eq(naClose(r.y.w, 3.5), true);
        expect_eq(naClose(r.z.w, 9.0), true);
    });
}
