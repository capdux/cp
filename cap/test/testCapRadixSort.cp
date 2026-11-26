// cap/test/testCapRadixSort.cp
// Comprehensive unit tests for @cap/radix-sort

import { test, expect_eq } from "cap:test";
import {
    sortU32,
    sortI32,
    sortU64,
    sortI64,
    sortF64,
    isSortedI64,
    isSortedU64,
} from "../../packages/@cap/radix-sort/src/index.cp";

fn main() {
    test("1. @cap/radix-sort — Empty, single-element & undersized scratch rejection", () => {
        let one: [1]u32 = [42];
        let scr1: [1]u32 = undefined;
        expect_eq(sortU32(one[0..0], scr1[0..0]), true);
        expect_eq(sortU32(one[0..1], scr1[0..1]), true);
        expect_eq(one[0], 42 as u32);

        let arr: [4]u32 = [4, 3, 2, 1];
        let small: [2]u32 = undefined;
        expect_eq(sortU32(arr[0..4], small[0..2]), false);
    });

    test("2. @cap/radix-sort — sortU32 wide unsigned range", () => {
        let data: [8]u32 = [0xFFFFFFFF, 0, 100000, 1, 0x80000000, 42, 0x00FF00FF, 7];
        let scratch: [8]u32 = undefined;
        expect_eq(sortU32(data[0..8], scratch[0..8]), true);
        expect_eq(data[0], 0 as u32);
        expect_eq(data[1], 1 as u32);
        expect_eq(data[2], 7 as u32);
        expect_eq(data[3], 42 as u32);
        expect_eq(data[4], 100000 as u32);
        expect_eq(data[5], 0x00FF00FF as u32);
        expect_eq(data[6], 0x80000000 as u32);
        expect_eq(data[7], 0xFFFFFFFF as u32);
    });

    test("3. @cap/radix-sort — sortI32 signed negative, zero & positive extremes", () => {
        const i32_min: i32 = -2147483647 - 1;
        const i32_max: i32 = 2147483647;
        let data: [7]i32 = [100, -50, 0, i32_max, i32_min, -1, 1];
        let scratch: [7]i32 = undefined;
        expect_eq(sortI32(data[0..7], scratch[0..7]), true);
        expect_eq(data[0], i32_min);
        expect_eq(data[1], -50 as i32);
        expect_eq(data[2], -1 as i32);
        expect_eq(data[3], 0 as i32);
        expect_eq(data[4], 1 as i32);
        expect_eq(data[5], 100 as i32);
        expect_eq(data[6], i32_max);
    });

    test("4. @cap/radix-sort — sortU64 across all 8 high/low bytes", () => {
        const hi1: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        const hi2: u64 = ((0x80000000 as u64) << 32) | (0x00000001 as u64);
        const mid: u64 = ((0x00010000 as u64) << 32) | (0x000000FF as u64);
        let data: [6]u64 = [hi1, 5, mid, 0, hi2, 5];
        let scratch: [6]u64 = undefined;
        expect_eq(isSortedU64(data[0..6]), false);
        expect_eq(sortU64(data[0..6], scratch[0..6]), true);
        expect_eq(isSortedU64(data[0..6]), true);
        expect_eq(data[0], 0 as u64);
        expect_eq(data[1], 5 as u64);
        expect_eq(data[2], 5 as u64);
        expect_eq(data[3], mid);
        expect_eq(data[4], hi2);
        expect_eq(data[5], hi1);
    });

    test("5. @cap/radix-sort — sortI64 with large negative, zero & large positive values", () => {
        const i64_min: i64 = -9223372036854775807 - 1;
        const i64_max: i64 = 9223372036854775807;
        let data: [7]i64 = [1000000000000, -1000000000000, 0, i64_max, i64_min, -42, 42];
        let scratch: [7]i64 = undefined;
        expect_eq(isSortedI64(data[0..7]), false);
        expect_eq(sortI64(data[0..7], scratch[0..7]), true);
        expect_eq(isSortedI64(data[0..7]), true);
        expect_eq(data[0], i64_min);
        expect_eq(data[1], -1000000000000 as i64);
        expect_eq(data[2], -42 as i64);
        expect_eq(data[3], 0 as i64);
        expect_eq(data[4], 42 as i64);
        expect_eq(data[5], 1000000000000 as i64);
        expect_eq(data[6], i64_max);
    });

    test("6. @cap/radix-sort — sortF64 IEEE-754 negative, -0.0, +0.0 & positive order", () => {
        let data: [8]f64 = [3.14, -100.5, 0.0, -0.001, 1000000000000.0, -1000000000.0, -0.0, 0.001];
        let scratch: [8]f64 = undefined;
        expect_eq(sortF64(data[0..8], scratch[0..8]), true);
        expect_eq(data[0], -1000000000.0);
        expect_eq(data[1], -100.5);
        expect_eq(data[2], -0.001);
        expect_eq(data[3] == 0.0 && data[4] == 0.0, true);
        expect_eq(data[5], 0.001);
        expect_eq(data[6], 3.14);
        expect_eq(data[7], 1000000000000.0);
    });
}
