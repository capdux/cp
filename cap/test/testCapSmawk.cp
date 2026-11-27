import { test, expect_eq } from "cap:test";
import {
    rowMinima,
    isTotallyMonotone,
    mongeRowMinima,
} from "../../packages/@cap/smawk/src/index.cp";

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
}
