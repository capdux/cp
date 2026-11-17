import { test, expect_eq } from "cap:test";
import { Simd } from "cap:simd";

fn main() {
    test("Simd.suggestVectorLength", () => {
        let len_i32 = Simd.suggestVectorLength(i32);
        expect_eq(len_i32 >= 4, true);
        let len_f32 = Simd.suggestVectorLength(f32);
        expect_eq(len_f32 >= 4, true);
    });

    test("Simd.iotaI32", () => {
        let v: [8]i32 = Simd.iotaI32(8);
        expect_eq(v[0], 0);
        expect_eq(v[1], 1);
        expect_eq(v[7], 7);
    });

    test("Simd.reduceAddI32", () => {
        let arr: [10]i32 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
        let sum = Simd.reduceAddI32(arr[0..]);
        expect_eq(sum, 55);
    });

    test("Simd.reduceMinI32 and reduceMaxI32", () => {
        let arr: [10]i32 = [5, 2, 8, 1, 9, 3, 7, 4, 6, 0];
        let min_val = Simd.reduceMinI32(arr[0..]);
        let max_val = Simd.reduceMaxI32(arr[0..]);
        expect_eq(min_val, 0);
        expect_eq(max_val, 9);
    });

    test("Simd.dotProductF32", () => {
        let a: [4]f32 = [1.0, 2.0, 3.0, 4.0];
        let b: [4]f32 = [2.0, 3.0, 4.0, 5.0];
        let dot = Simd.dotProductF32(a[0..], b[0..]);
        // 1*2 + 2*3 + 3*4 + 4*5 = 2 + 6 + 12 + 20 = 40.0
        expect_eq(dot, 40.0);
    });

    test("Simd.prefixSumI32", () => {
        let arr: [5]i32 = [1, 2, 3, 4, 5];
        Simd.prefixSumI32(arr[0..]);
        expect_eq(arr[0], 1);
        expect_eq(arr[1], 3);
        expect_eq(arr[2], 6);
        expect_eq(arr[3], 10);
        expect_eq(arr[4], 15);
    });
}
