import { test, expect_eq } from "cap:test";
import {
    sgemm,
    dgemm,
    matmulF64,
} from "../../packages/matrixmultiply/src/index.cp";

fn main() {
    test("1. @cap/matrixmultiply — dgemm row-major 2x3 * 3x2", () => {
        const a: [6]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
        const b: [6]f64 = [7.0, 8.0, 9.0, 10.0, 11.0, 12.0];
        let c: [4]f64 = [99.0, 99.0, 99.0, 99.0];
        dgemm(2, 3, 2, 1.0, a[0..6], 3, 1, b[0..6], 2, 1, 0.0, c[0..4], 2, 1);
        expect_eq(c[0], 58.0);
        expect_eq(c[1], 64.0);
        expect_eq(c[2], 139.0);
        expect_eq(c[3], 154.0);
    });

    test("2. @cap/matrixmultiply — dgemm alpha and beta scaling", () => {
        const a: [4]f64 = [1.0, 2.0, 3.0, 4.0];
        const b: [4]f64 = [5.0, 6.0, 7.0, 8.0];
        let c: [4]f64 = [1.0, 2.0, 3.0, 4.0];
        // A*B = [[19, 22], [43, 50]]
        // 2*A*B + 3*C = [[41, 50], [95, 112]]
        dgemm(2, 2, 2, 2.0, a[0..4], 2, 1, b[0..4], 2, 1, 3.0, c[0..4], 2, 1);
        expect_eq(c[0], 41.0);
        expect_eq(c[1], 50.0);
        expect_eq(c[2], 95.0);
        expect_eq(c[3], 112.0);
    });

    test("3. @cap/matrixmultiply — dgemm arbitrary strides (transposed A^T * B)", () => {
        // Stored 3x2 row-major: [[1, 4], [2, 5], [3, 6]]
        // With rsa=1, csa=2 => logical 2x3 matrix A^T = [[1, 2, 3], [4, 5, 6]]
        const a_t: [6]f64 = [1.0, 4.0, 2.0, 5.0, 3.0, 6.0];
        const b: [6]f64 = [7.0, 8.0, 9.0, 10.0, 11.0, 12.0];
        // Write C in column-major order: rsc=1, csc=2
        let c: [4]f64 = [0.0, 0.0, 0.0, 0.0];
        dgemm(2, 3, 2, 1.0, a_t[0..6], 1, 2, b[0..6], 2, 1, 0.0, c[0..4], 1, 2);
        // Column-major layout: [c00, c10, c01, c11]
        expect_eq(c[0], 58.0);
        expect_eq(c[1], 139.0);
        expect_eq(c[2], 64.0);
        expect_eq(c[3], 154.0);
    });

    test("4. @cap/matrixmultiply — sgemm f32 and alpha=0 beta=0 zeroing", () => {
        const a: [4]f32 = [2.0, 1.0, 0.0, 3.0];
        const b: [4]f32 = [4.0, 5.0, 6.0, 7.0];
        let c: [4]f32 = [1.0, 1.0, 1.0, 1.0];
        // A*B = [[14, 17], [18, 21]], + 1*C = [[15, 18], [19, 22]]
        sgemm(2, 2, 2, 1.0, a[0..4], 2, 1, b[0..4], 2, 1, 1.0, c[0..4], 2, 1);
        expect_eq(c[0], 15.0);
        expect_eq(c[1], 18.0);
        expect_eq(c[2], 19.0);
        expect_eq(c[3], 22.0);
        // alpha=0, beta=0 should zero C
        sgemm(2, 2, 2, 0.0, a[0..4], 2, 1, b[0..4], 2, 1, 0.0, c[0..4], 2, 1);
        expect_eq(c[0], 0.0);
        expect_eq(c[3], 0.0);
    });

    test("5. @cap/matrixmultiply — matmulF64 row-major helper with identity", () => {
        const a: [9]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0];
        const eye: [9]f64 = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0];
        let c: [9]f64 = [0.0; 9];
        matmulF64(3, 3, 3, a[0..9], eye[0..9], c[0..9]);
        expect_eq(c[0], 1.0);
        expect_eq(c[4], 5.0);
        expect_eq(c[8], 9.0);
    });

    test("6. @cap/matrixmultiply — blocked multiplication exceeding tile size (>32)", () => {
        // 36x35 matrix of 1.0 multiplied by 35x34 matrix of 2.0 => every entry is 70.0
        let a: [1260]f64 = [1.0; 1260];
        let b: [1190]f64 = [2.0; 1190];
        let c: [1224]f64 = [0.0; 1224];
        matmulF64(36, 35, 34, a[0..1260], b[0..1190], c[0..1224]);
        expect_eq(c[0], 70.0);
        expect_eq(c[612], 70.0);
        expect_eq(c[1223], 70.0);
    });
}
