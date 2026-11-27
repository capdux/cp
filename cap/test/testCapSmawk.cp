import { test, expect_eq } from "cap:test";
import {
    rowMinima,
    colMinima,
    isMonge,
    isTotallyMonotone,
    reduce,
    interpolate,
    mongeRowMinima,
    smawkRowMinima,
    smawkColMinima,
    onlineColumnMinima,
} from "../../packages/smawk/src/index.cp";

fn main() {
    test("1. @cap/smawk — rowMinima finds leftmost minimum column per row", () => {
        const m: [12]f64 = [
            3.0, 1.0, 1.0, 4.0,
            5.0, 2.0, 2.0, 0.5,
            9.0, 9.0, 9.0, 9.0,
        ];
        let out: [3]usize = [0; 3];
        const n = rowMinima(m[0..12], 3, 4, out[0..3]);
        expect_eq(n, 3 as usize);
        expect_eq(out[0], 1 as usize);
        expect_eq(out[1], 3 as usize);
        expect_eq(out[2], 0 as usize);
    });

    test("2. @cap/smawk — isTotallyMonotone accepts a classic Monge matrix", () => {
        // M[i, j] = (i - j)^2 is a Monge (hence totally monotone) matrix
        const m: [16]f64 = [
            0.0, 1.0, 4.0, 9.0,
            1.0, 0.0, 1.0, 4.0,
            4.0, 1.0, 0.0, 1.0,
            9.0, 4.0, 1.0, 0.0,
        ];
        expect_eq(isTotallyMonotone(m[0..16], 4, 4), true);
    });

    test("3. @cap/smawk — isTotallyMonotone rejects non-monotone matrix", () => {
        const m: [4]f64 = [
            5.0, 1.0,
            1.0, 5.0,
        ];
        expect_eq(isTotallyMonotone(m[0..4], 2, 2), false);
    });

    test("4. @cap/smawk — mongeRowMinima matches rowMinima on diagonal Monge matrix", () => {
        const m: [16]f64 = [
            0.0, 1.0, 4.0, 9.0,
            1.0, 0.0, 1.0, 4.0,
            4.0, 1.0, 0.0, 1.0,
            9.0, 4.0, 1.0, 0.0,
        ];
        let brute: [4]usize = [0; 4];
        let fast: [4]usize = [0; 4];
        expect_eq(rowMinima(m[0..16], 4, 4, brute[0..4]), 4 as usize);
        expect_eq(mongeRowMinima(m[0..16], 4, 4, fast[0..4]), 4 as usize);
        expect_eq(fast[0], brute[0]);
        expect_eq(fast[1], brute[1]);
        expect_eq(fast[2], brute[2]);
        expect_eq(fast[3], brute[3]);
    });

    test("5. @cap/smawk — mongeRowMinima matches rowMinima on rectangular 5x4 Monge matrix", () => {
        // Classic Wikipedia/SMAWK Monge matrix
        const m: [20]f64 = [
            10.0, 17.0, 13.0, 28.0,
            17.0, 22.0, 16.0, 29.0,
            24.0, 28.0, 22.0, 34.0,
            11.0, 13.0,  6.0, 17.0,
            45.0, 44.0, 32.0, 37.0,
        ];
        expect_eq(isTotallyMonotone(m[0..20], 5, 4), true);
        let brute: [5]usize = [0; 5];
        let fast: [5]usize = [0; 5];
        expect_eq(rowMinima(m[0..20], 5, 4, brute[0..5]), 5 as usize);
        expect_eq(mongeRowMinima(m[0..20], 5, 4, fast[0..5]), 5 as usize);
        expect_eq(fast[0], brute[0]);
        expect_eq(fast[1], brute[1]);
        expect_eq(fast[2], brute[2]);
        expect_eq(fast[3], brute[3]);
        expect_eq(fast[4], brute[4]);
    });

    test("6. @cap/smawk — handles empty and 1x1 edge cases safely", () => {
        const m1: [1]f64 = [42.0];
        let out: [2]usize = [99; 2];
        expect_eq(isTotallyMonotone(m1[0..1], 1, 1), true);
        expect_eq(mongeRowMinima(m1[0..1], 1, 1, out[0..2]), 1 as usize);
        expect_eq(out[0], 0 as usize);
        expect_eq(rowMinima(m1[0..1], 0, 1, out[0..2]), 0 as usize);
        expect_eq(mongeRowMinima(m1[0..1], 1, 0, out[0..2]), 0 as usize);
    });

    test("7. @cap/smawk — isMonge, reduce & interpolate primitives", () => {
        // 3x5 Monge matrix
        const m: [15]f64 = [
            10.0, 17.0, 13.0, 28.0, 23.0,
            17.0, 22.0, 16.0, 29.0, 23.0,
            24.0, 28.0, 22.0, 34.0, 24.0,
        ];
        expect_eq(isMonge(m[0..15], 3, 5), true);
        expect_eq(isTotallyMonotone(m[0..15], 3, 5), true);

        const r_idx: [3]usize = [0, 1, 2];
        const c_idx: [5]usize = [0, 1, 2, 3, 4];
        let red_cols: [5]usize = [0; 5];
        const k = reduce(m[0..15], 5, r_idx[0..3], c_idx[0..5], red_cols[0..5]);
        expect_eq(k <= 3, true);
        expect_eq(k > 0, true);

        // Verify interpolate fills even rows 0 and 2 given odd row 1 minimum
        let minima: [3]usize = [0, 2, 0]; // row 1 min is col 2 (16.0)
        interpolate(m[0..15], 5, r_idx[0..3], c_idx[0..5], minima[0..3]);
        expect_eq(minima[0], 0 as usize); // row 0 min is col 0 (10.0)
        expect_eq(minima[2], 2 as usize); // row 2 min is col 2 (22.0)
    });

    test("8. @cap/smawk — smawkRowMinima matches rowMinima on 5x4 & 4x6 Monge matrices", () => {
        const m: [20]f64 = [
            10.0, 17.0, 13.0, 28.0,
            17.0, 22.0, 16.0, 29.0,
            24.0, 28.0, 22.0, 34.0,
            11.0, 13.0,  6.0, 17.0,
            45.0, 44.0, 32.0, 37.0,
        ];
        let brute: [5]usize = [0; 5];
        let fast: [5]usize = [0; 5];
        expect_eq(rowMinima(m[0..20], 5, 4, brute[0..5]), 5 as usize);
        expect_eq(smawkRowMinima(m[0..20], 5, 4, fast[0..5]), 5 as usize);
        let i: usize = 0;
        while (i < 5) : (i += 1) {
            expect_eq(fast[i], brute[i]);
        }
    });

    test("9. @cap/smawk — colMinima & smawkColMinima agree on Monge matrix", () => {
        const m: [16]f64 = [
            0.0, 1.0, 4.0, 9.0,
            1.0, 0.0, 1.0, 4.0,
            4.0, 1.0, 0.0, 1.0,
            9.0, 4.0, 1.0, 0.0,
        ];
        let brute: [4]usize = [0; 4];
        let fast: [4]usize = [0; 4];
        expect_eq(colMinima(m[0..16], 4, 4, brute[0..4]), 4 as usize);
        expect_eq(smawkColMinima(m[0..16], 4, 4, fast[0..4]), 4 as usize);
        expect_eq(fast[0], 0 as usize);
        expect_eq(fast[1], 1 as usize);
        expect_eq(fast[2], 2 as usize);
        expect_eq(fast[3], 3 as usize);
        expect_eq(fast[0], brute[0]);
        expect_eq(fast[3], brute[3]);
    });

    test("10. @cap/smawk — onlineColumnMinima finds exact argmin_{i < j} M[i, j]", () => {
        // Upper-triangular Monge cost matrix M[i, j] = (j - i - 2)^2 for i < j (lower triangle poisoned with -999)
        const m: [25]f64 = [
            -999.0,  1.0,  0.0,  1.0,  4.0,
            -999.0, -999.0, 1.0,  0.0,  1.0,
            -999.0, -999.0, -999.0, 1.0,  0.0,
            -999.0, -999.0, -999.0, -999.0, 1.0,
            -999.0, -999.0, -999.0, -999.0, -999.0,
        ];
        let out: [5]usize = [99; 5];
        expect_eq(onlineColumnMinima(m[0..25], 5, out[0..5]), 5 as usize);
        expect_eq(out[0], 0 as usize);
        expect_eq(out[1], 0 as usize); // only i=0 < 1
        expect_eq(out[2], 0 as usize); // M[0,2]=0 < M[1,2]=1
        expect_eq(out[3], 1 as usize); // M[1,3]=0 < M[0,3]=1, M[2,3]=1
        expect_eq(out[4], 2 as usize); // M[2,4]=0
    });
}
