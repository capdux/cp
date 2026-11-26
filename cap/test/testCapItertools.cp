// cap/test/testCapItertools.cp
// Comprehensive unit tests for @cap/itertools (Rust itertools parity)

import { test, expect_eq } from "cap:test";
import {
    minmax,
    isSorted,
    isSortedDesc,
    allEqual,
    uniqueI64,
    dedupConsecutive,
    interleave,
    intersperse,
    mergeSorted,
    joinStrings,
    partitionByThreshold,
    cartesianProductCount,
    nthCartesianPair,
    chunkCount,
    windowCount
} from "../../packages/@cap/itertools/src/index.cp";

fn itTestBytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn itTestI64Eq(a: []const i64, b: []const i64): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/itertools — minmax, isSorted, isSortedDesc & allEqual (empty, single, multi)", () => {
        let empty: [1]i64 = [0];
        let mm0 = minmax(empty[0..0]);
        expect_eq(mm0.empty, true);
        expect_eq(isSorted(empty[0..0]), true);
        expect_eq(isSortedDesc(empty[0..0]), true);
        expect_eq(allEqual(empty[0..0]), true);

        let single: [1]i64 = [42];
        let mm1 = minmax(single[0..1]);
        expect_eq(mm1.empty, false);
        expect_eq(mm1.min, 42 as i64);
        expect_eq(mm1.max, 42 as i64);
        expect_eq(isSorted(single[0..1]), true);
        expect_eq(isSortedDesc(single[0..1]), true);
        expect_eq(allEqual(single[0..1]), true);

        let asc: [5]i64 = [-10, 0, 5, 5, 99];
        let mm2 = minmax(asc[0..5]);
        expect_eq(mm2.min, -10 as i64);
        expect_eq(mm2.max, 99 as i64);
        expect_eq(isSorted(asc[0..5]), true);
        expect_eq(isSortedDesc(asc[0..5]), false);
        expect_eq(allEqual(asc[0..5]), false);

        let desc: [4]i64 = [9, 5, 5, -1];
        expect_eq(isSorted(desc[0..4]), false);
        expect_eq(isSortedDesc(desc[0..4]), true);

        let same: [4]i64 = [7, 7, 7, 7];
        expect_eq(allEqual(same[0..4]), true);
        expect_eq(isSorted(same[0..4]), true);
        expect_eq(isSortedDesc(same[0..4]), true);
    });

    test("2. @cap/itertools — uniqueI64 order-preserving dedup & dedupConsecutive", () => {
        let src: [8]i64 = [3, 1, 3, 2, 2, 1, 4, 2];
        let out: [8]i64 = [0; 8];

        let nu = uniqueI64(src[0..8], out[0..8]);
        let exp_u: [4]i64 = [3, 1, 2, 4];
        expect_eq(nu, 4 as usize);
        expect_eq(itTestI64Eq(out[0..nu], exp_u[0..4]), true);

        let runs: [8]i64 = [1, 1, 2, 2, 2, 1, 3, 3];
        let nd = dedupConsecutive(runs[0..8], out[0..8]);
        let exp_d: [4]i64 = [1, 2, 1, 3];
        expect_eq(nd, 4 as usize);
        expect_eq(itTestI64Eq(out[0..nd], exp_d[0..4]), true);
    });

    test("3. @cap/itertools — interleave unequal lengths & intersperse", () => {
        let a: [2]i64 = [1, 3];
        let b: [4]i64 = [2, 4, 6, 8];
        let out: [16]i64 = [0; 16];

        let ni = interleave(a[0..2], b[0..4], out[0..16]);
        let exp_i: [6]i64 = [1, 2, 3, 4, 6, 8];
        expect_eq(ni, 6 as usize);
        expect_eq(itTestI64Eq(out[0..ni], exp_i[0..6]), true);

        let s: [3]i64 = [10, 20, 30];
        let ns = intersperse(s[0..3], 0, out[0..16]);
        let exp_s: [5]i64 = [10, 0, 20, 0, 30];
        expect_eq(ns, 5 as usize);
        expect_eq(itTestI64Eq(out[0..ns], exp_s[0..5]), true);

        // Single element intersperse emits no separator
        expect_eq(intersperse(s[0..1], 0, out[0..16]), 1 as usize);
        expect_eq(out[0], 10 as i64);
    });

    test("4. @cap/itertools — mergeSorted stable merge & partitionByThreshold", () => {
        let a: [4]i64 = [1, 4, 5, 9];
        let b: [3]i64 = [2, 4, 10];
        let out: [16]i64 = [0; 16];

        let nm = mergeSorted(a[0..4], b[0..3], out[0..16]);
        let exp_m: [7]i64 = [1, 2, 4, 4, 5, 9, 10];
        expect_eq(nm, 7 as usize);
        expect_eq(itTestI64Eq(out[0..nm], exp_m[0..7]), true);

        let hi: [8]i64 = [0; 8];
        let lo: [8]i64 = [0; 8];
        let pr = partitionByThreshold(out[0..nm], 5, hi[0..8], lo[0..8]);
        expect_eq(pr.true_count, 3 as usize);
        expect_eq(pr.false_count, 4 as usize);
        let exp_hi: [3]i64 = [5, 9, 10];
        let exp_lo: [4]i64 = [1, 2, 4, 4];
        expect_eq(itTestI64Eq(hi[0..pr.true_count], exp_hi[0..3]), true);
        expect_eq(itTestI64Eq(lo[0..pr.false_count], exp_lo[0..4]), true);
    });

    test("5. @cap/itertools — joinStrings zero-heap join (empty, single, multi)", () => {
        let buf: [64]u8 = undefined;
        let items: [3]string = ["alpha", "beta", "gamma"];

        expect_eq(joinStrings(items[0..0], ", ", buf[0..64]), 0 as usize);

        let n1 = joinStrings(items[0..1], ", ", buf[0..64]);
        expect_eq(itTestBytesEq(buf[0..n1], "alpha"), true);

        let n3 = joinStrings(items[0..3], ", ", buf[0..64]);
        expect_eq(itTestBytesEq(buf[0..n3], "alpha, beta, gamma"), true);

        let n_empty_sep = joinStrings(items[0..3], "", buf[0..64]);
        expect_eq(itTestBytesEq(buf[0..n_empty_sep], "alphabetagamma"), true);
    });

    test("6. @cap/itertools — nthCartesianPair traversal & chunkCount/windowCount boundaries", () => {
        let a: [2]i64 = [1, 2];
        let b: [3]i64 = [10, 20, 30];

        expect_eq(cartesianProductCount(2, 3), 6 as usize);

        let p0 = nthCartesianPair(a[0..2], b[0..3], 0);
        expect_eq(p0.found, true);
        expect_eq(p0.first, 1 as i64);
        expect_eq(p0.second, 10 as i64);

        let p4 = nthCartesianPair(a[0..2], b[0..3], 4);
        expect_eq(p4.found, true);
        expect_eq(p4.first, 2 as i64);
        expect_eq(p4.second, 20 as i64);

        let p5 = nthCartesianPair(a[0..2], b[0..3], 5);
        expect_eq(p5.found, true);
        expect_eq(p5.first, 2 as i64);
        expect_eq(p5.second, 30 as i64);

        // Out-of-bounds & empty slice return found=false
        expect_eq(nthCartesianPair(a[0..2], b[0..3], 6).found, false);
        expect_eq(nthCartesianPair(a[0..0], b[0..3], 0).found, false);

        // chunkCount & windowCount boundaries
        expect_eq(chunkCount(0, 3), 0 as usize);
        expect_eq(chunkCount(7, 0), 0 as usize);
        expect_eq(chunkCount(6, 3), 2 as usize);
        expect_eq(chunkCount(7, 3), 3 as usize);

        expect_eq(windowCount(5, 0), 0 as usize);
        expect_eq(windowCount(5, 6), 0 as usize);
        expect_eq(windowCount(5, 5), 1 as usize);
        expect_eq(windowCount(5, 3), 3 as usize);
    });
}
