import { test, expect_eq } from "cap:test";
import {
    sumF64,
    meanF64,
    extentF64,
    varianceF64,
    deviationF64,
    quantileSorted,
    medianSorted,
    bisectLeft,
    bisectRight,
    cumsumF64,
    binCounts,
} from "../../packages/d3-array/src/index.cp";

fn main() {
    test("1. @cap/d3-array — sum, mean, and extent ignore NaN", () => {
        const nan_v: f64 = std.math.nan(f64);
        let arr: [5]f64 = [2.0, nan_v, 4.0, 6.0, 8.0];
        expect_eq(sumF64(arr[0..5]), 20.0);
        expect_eq(meanF64(arr[0..5]) ?? 0.0, 5.0);
        const ext = extentF64(arr[0..5]);
        expect_eq(ext.valid, true);
        expect_eq(ext.min, 2.0);
        expect_eq(ext.max, 8.0);
    });

    test("2. @cap/d3-array — Welford sample variance and deviation", () => {
        let arr: [4]f64 = [2.0, 4.0, 4.0, 6.0]; // mean=4, sum_sq=8, var=8/3
        const v = varianceF64(arr[0..4]) ?? 0.0;
        const diff = v - (8.0 / 3.0);
        expect_eq(diff > -1.0e-9 && diff < 1.0e-9, true);
        const sd = deviationF64(arr[0..4]) ?? 0.0;
        expect_eq(sd > 1.63 && sd < 1.64, true);
    });

    test("3. @cap/d3-array — R-7 quantileSorted and medianSorted", () => {
        let sorted: [4]f64 = [0.0, 10.0, 20.0, 30.0];
        expect_eq(quantileSorted(sorted[0..4], 0.0) ?? -1.0, 0.0);
        expect_eq(quantileSorted(sorted[0..4], 0.5) ?? -1.0, 15.0);
        expect_eq(medianSorted(sorted[0..4]) ?? -1.0, 15.0);
        expect_eq(quantileSorted(sorted[0..4], 0.25) ?? -1.0, 7.5);
        expect_eq(quantileSorted(sorted[0..4], 1.0) ?? -1.0, 30.0);
    });

    test("4. @cap/d3-array — bisectLeft and bisectRight on duplicates", () => {
        let sorted: [5]f64 = [1.0, 2.0, 2.0, 2.0, 5.0];
        expect_eq(bisectLeft(sorted[0..5], 2.0), 1 as usize);
        expect_eq(bisectRight(sorted[0..5], 2.0), 4 as usize);
        expect_eq(bisectLeft(sorted[0..5], 0.5), 0 as usize);
        expect_eq(bisectRight(sorted[0..5], 9.0), 5 as usize);
    });

    test("5. @cap/d3-array — cumsumF64 produces running totals", () => {
        let data: [4]f64 = [1.0, 2.0, 3.0, 4.0];
        let out: [4]f64 = [0.0; 4];
        expect_eq(cumsumF64(data[0..4], out[0..4]), 4 as usize);
        expect_eq(out[0], 1.0);
        expect_eq(out[1], 3.0);
        expect_eq(out[2], 6.0);
        expect_eq(out[3], 10.0);
    });

    test("6. @cap/d3-array — binCounts partitions values across equal-width histogram bins", () => {
        let data: [6]f64 = [0.0, 2.5, 5.0, 7.5, 9.9, 10.0];
        let bins: [4]usize = [0; 4]; // [0,2.5), [2.5,5), [5,7.5), [7.5,10]
        const total = binCounts(data[0..6], 0.0, 10.0, bins[0..4]);
        expect_eq(total, 6 as usize);
        expect_eq(bins[0], 1 as usize);
        expect_eq(bins[1], 1 as usize);
        expect_eq(bins[2], 1 as usize);
        expect_eq(bins[3], 3 as usize);
    });
}
