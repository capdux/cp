import { test, expect_eq } from "cap:test";
import {
    Point2, Point3, Vector2, Vector3, Vector4,
    Quaternion, EulerAngles, RowMatrix2, RowMatrix3, RowMatrix4, ColumnMatrix4,
    point3ToVector3, vector3ToPoint3
} from "../../packages/mint/src/index.cp";

fn main() {
    test("1. @cap/mint — Point2/Point3 conversions & array views", () => {
        let p2 = Point2.new(3.0, 4.0);
        let a2 = p2.toArray();
        expect_eq(a2[0], 3.0);
        expect_eq(a2[1], 4.0);

        let p3 = Point3.new(1.0, 2.0, 3.0);
        let v3 = point3ToVector3(p3);
        let back = vector3ToPoint3(v3);
        expect_eq(back.x, 1.0);
        expect_eq(back.y, 2.0);
        expect_eq(back.z, 3.0);
    });

    test("2. @cap/mint — Vector2/Vector3 dot, cross & lenSq", () => {
        let a = Vector3.new(1.0, 0.0, 0.0);
        let b = Vector3.new(0.0, 1.0, 0.0);
        let c = a.cross(b);
        expect_eq(c.x, 0.0);
        expect_eq(c.y, 0.0);
        expect_eq(c.z, 1.0);
        expect_eq(a.dot(b), 0.0);
        expect_eq(Vector2.new(3.0, 4.0).lenSq(), 25.0);
    });

    test("3. @cap/mint — Quaternion identity, conjugate & Hamilton product", () => {
        let q = Quaternion.new(0.0, 0.0, 1.0, 0.0); // 180 deg around Z
        let qi = q.mul(Quaternion.identity());
        expect_eq(qi.v.z, 1.0);
        expect_eq(qi.s, 0.0);
        // q * conj(q) = (0, 0, 0, normSq)
        let prod = q.mul(q.conjugate());
        expect_eq(prod.v.x, 0.0);
        expect_eq(prod.v.y, 0.0);
        expect_eq(prod.v.z, 0.0);
        expect_eq(prod.s, 1.0);
    });

    test("4. @cap/mint — RowMatrix2 determinant & transpose", () => {
        let m = RowMatrix2 {
            x: Vector2.new(2.0, 3.0),
            y: Vector2.new(1.0, 4.0),
        };
        expect_eq(m.det(), 5.0);
        let t = m.transpose();
        expect_eq(t.x.y, 1.0);
        expect_eq(t.y.x, 3.0);
    });

    test("5. @cap/mint — RowMatrix3 identity & mulVec3", () => {
        let id = RowMatrix3.identity();
        let v = Vector3.new(7.0, -2.0, 5.0);
        let r = id.mulVec3(v);
        expect_eq(r.x, 7.0);
        expect_eq(r.y, -2.0);
        expect_eq(r.z, 5.0);
    });

    test("6. @cap/mint — RowMatrix4 <-> ColumnMatrix4 conversion & EulerAngles", () => {
        let rm = RowMatrix4 {
            x: Vector4.new(1.0, 2.0, 3.0, 4.0),
            y: Vector4.new(5.0, 6.0, 7.0, 8.0),
            z: Vector4.new(9.0, 10.0, 11.0, 12.0),
            w: Vector4.new(13.0, 14.0, 15.0, 16.0),
        };
        let cm: ColumnMatrix4 = rm.toColumnMajor();
        expect_eq(cm.x.y, 5.0);
        expect_eq(cm.y.x, 2.0);
        let back = cm.toRowMajor();
        expect_eq(back.x.y, 2.0);
        expect_eq(back.w.w, 16.0);
        let eu = EulerAngles.new(0.1, 0.2, 0.3);
        expect_eq(eu.c, 0.3);
    });
}
