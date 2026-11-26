import { test, expect_eq } from "cap:test";
import { Array2D, addArrays, matmul, dotProduct } from "../../packages/@cap/ndarray/src/index.cp";

fn main() {
    test("1. @cap/ndarray — zeros, filled, eye & get/set bounds", () => {
        let id = Array2D.eye(3);
        expect_eq(id.elemCount(), 9);
        expect_eq(id.get(0, 0) ?? -1.0, 1.0);
        expect_eq(id.get(0, 1) ?? -1.0, 0.0);
        expect_eq(id.get(3, 0) == null, true);

        let a = Array2D.filled(2, 2, 5.0);
        expect_eq(a.set(1, 0, 9.0), true);
        expect_eq(a.get(1, 0) ?? 0.0, 9.0);
        expect_eq(a.set(5, 5, 1.0), false);
    });

    test("2. @cap/ndarray — reductions sum, mean, minVal & maxVal", () => {
        let vals: [4]f64 = [2.0, 4.0, 6.0, 8.0];
        let a = Array2D.fromSlice(2, 2, vals[0..4]) ?? Array2D.zeros(0, 0);
        expect_eq(a.sum(), 20.0);
        expect_eq(a.mean() ?? 0.0, 5.0);
        expect_eq(a.minVal() ?? 0.0, 2.0);
        expect_eq(a.maxVal() ?? 0.0, 8.0);
    });

    test("3. @cap/ndarray — transpose & reshape", () => {
        let vals: [6]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
        let a = Array2D.fromSlice(2, 3, vals[0..6]) ?? Array2D.zeros(0, 0);
        let t = a.transpose();
        expect_eq(t.rows, 3);
        expect_eq(t.cols, 2);
        expect_eq(t.get(2, 0) ?? 0.0, 3.0);
        expect_eq(t.get(0, 1) ?? 0.0, 4.0);

        let r = a.reshape(3, 2) ?? Array2D.zeros(0, 0);
        expect_eq(r.rows, 3);
        expect_eq(r.get(1, 0) ?? 0.0, 3.0);
        expect_eq(a.reshape(4, 4) == null, true);
    });

    test("4. @cap/ndarray — elementwise addArrays & scale", () => {
        let a = Array2D.filled(2, 2, 3.0);
        let b = a.scale(2.0);
        let c = addArrays(a, b) ?? Array2D.zeros(0, 0);
        expect_eq(c.get(0, 0) ?? 0.0, 9.0);
        expect_eq(c.sum(), 36.0);
        expect_eq(addArrays(a, Array2D.zeros(3, 2)) == null, true);
    });

    test("5. @cap/ndarray — matmul (2x3) * (3x2) = (2x2)", () => {
        let va: [6]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
        let vb: [6]f64 = [7.0, 8.0, 9.0, 10.0, 11.0, 12.0];
        let a = Array2D.fromSlice(2, 3, va[0..6]) ?? Array2D.zeros(0, 0);
        let b = Array2D.fromSlice(3, 2, vb[0..6]) ?? Array2D.zeros(0, 0);
        let prod = matmul(a, b) ?? Array2D.zeros(0, 0);
        expect_eq(prod.rows, 2);
        expect_eq(prod.cols, 2);
        expect_eq(prod.get(0, 0) ?? 0.0, 58.0);
        expect_eq(prod.get(0, 1) ?? 0.0, 64.0);
        expect_eq(prod.get(1, 0) ?? 0.0, 139.0);
        expect_eq(prod.get(1, 1) ?? 0.0, 154.0);
        expect_eq(matmul(a, a) == null, true);
    });

    test("6. @cap/ndarray — dotProduct & dimension mismatch guard", () => {
        let x: [3]f64 = [1.0, 3.0, -5.0];
        let y: [3]f64 = [4.0, -2.0, -1.0];
        let z: [2]f64 = [1.0, 2.0];
        expect_eq(dotProduct(x[0..3], y[0..3]) ?? 0.0, 3.0);
        expect_eq(dotProduct(x[0..3], z[0..2]) == null, true);
    });
}
