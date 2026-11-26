import { test, expect_eq } from "cap:test";
import {
    I64_MAX, I64_MIN, U64_MAX,
    checkedAddI64, checkedSubI64, checkedMulI64,
    checkedAddU64, checkedSubU64, checkedMulU64,
    saturatingAddU64, saturatingSubU64, saturatingMulU64,
    saturatingAddI64, saturatingSubI64,
    clampI64, clampF64, signumI64, absDiffU64, checkedPowU64,
    isPowerOfTwoU64, nextPowerOfTwoU64,
    countOnesU64, leadingZerosU64, trailingZerosU64,
    rotateLeftU64, rotateRightU64
} from "../../packages/@cap/num-traits/src/index.cp";

fn main() {
    test("1. @cap/num-traits — checked i64 add/sub/mul & boundary overflow", () => {
        expect_eq(checkedAddI64(10, 25) ?? 0, 35);
        expect_eq(checkedAddI64(I64_MAX, 1) == null, true);
        expect_eq(checkedAddI64(I64_MIN, -1) == null, true);
        expect_eq(checkedAddI64(I64_MAX, I64_MIN) ?? 0, -1);

        expect_eq(checkedSubI64(10, 25) ?? 0, -15);
        expect_eq(checkedSubI64(I64_MIN, 1) == null, true);
        expect_eq(checkedSubI64(I64_MAX, -1) == null, true);
        expect_eq(checkedSubI64(0, I64_MIN) == null, true);

        expect_eq(checkedMulI64(-7, 6) ?? 0, -42);
        expect_eq(checkedMulI64(I64_MIN, 1) ?? 0, I64_MIN);
        expect_eq(checkedMulI64(I64_MIN, -1) == null, true);
        expect_eq(checkedMulI64(I64_MAX, 2) == null, true);
        expect_eq(checkedMulI64(I64_MIN, 0) ?? 99, 0);
    });

    test("2. @cap/num-traits — checked u64 add/sub/mul & underflow/overflow", () => {
        expect_eq(checkedAddU64(100, 200) ?? 0, 300);
        expect_eq(checkedAddU64(U64_MAX, 1) == null, true);
        expect_eq(checkedAddU64(U64_MAX, 0) ?? 0, U64_MAX);

        expect_eq(checkedSubU64(10, 4) ?? 0, 6);
        expect_eq(checkedSubU64(4, 10) == null, true);
        expect_eq(checkedSubU64(0, 0) ?? 99, 0);

        expect_eq(checkedMulU64(1000000, 1000000) ?? 0, 1000000000000);
        expect_eq(checkedMulU64(U64_MAX, 2) == null, true);
        expect_eq(checkedMulU64(U64_MAX, 0) ?? 99, 0);
    });

    test("3. @cap/num-traits — saturating u64 & i64 arithmetic", () => {
        expect_eq(saturatingAddU64(U64_MAX - 5, 10), U64_MAX);
        expect_eq(saturatingAddU64(10, 20), 30);
        expect_eq(saturatingSubU64(5, 10), 0);
        expect_eq(saturatingSubU64(10, 4), 6);
        expect_eq(saturatingMulU64(U64_MAX, 2), U64_MAX);
        expect_eq(saturatingMulU64(U64_MAX, 0), 0);

        expect_eq(saturatingAddI64(I64_MAX, 100), I64_MAX);
        expect_eq(saturatingAddI64(I64_MIN, -100), I64_MIN);
        expect_eq(saturatingSubI64(I64_MIN, 100), I64_MIN);
        expect_eq(saturatingSubI64(I64_MAX, -100), I64_MAX);
        expect_eq(saturatingSubI64(50, 20), 30);
    });

    test("4. @cap/num-traits — clamp, signum, absDiff & checkedPowU64", () => {
        expect_eq(clampI64(5, 0, 10), 5);
        expect_eq(clampI64(-5, 0, 10), 0);
        expect_eq(clampI64(25, 0, 10), 10);

        expect_eq(clampF64(0.5, 0.0, 1.0), 0.5);
        expect_eq(clampF64(-1.5, 0.0, 1.0), 0.0);
        expect_eq(clampF64(2.5, 0.0, 1.0), 1.0);
        let zero: f64 = 0.0;
        let nan_clamped = clampF64(zero / zero, 0.0, 1.0);
        expect_eq(nan_clamped != nan_clamped, true);

        expect_eq(signumI64(-42), -1);
        expect_eq(signumI64(0), 0);
        expect_eq(signumI64(42), 1);

        expect_eq(absDiffU64(10, 3), 7);
        expect_eq(absDiffU64(3, 10), 7);
        expect_eq(absDiffU64(U64_MAX, 0), U64_MAX);

        expect_eq(checkedPowU64(0, 0) ?? 0, 1);
        expect_eq(checkedPowU64(2, 10) ?? 0, 1024);
        expect_eq(checkedPowU64(2, 63) ?? 0, (1 as u64) << 63);
        expect_eq(checkedPowU64(2, 64) == null, true);
        expect_eq(checkedPowU64((1 as u64) << 32, 1) ?? 0, (1 as u64) << 32);
    });

    test("5. @cap/num-traits — isPowerOfTwoU64 & nextPowerOfTwoU64", () => {
        expect_eq(isPowerOfTwoU64(0), false);
        expect_eq(isPowerOfTwoU64(1), true);
        expect_eq(isPowerOfTwoU64(2), true);
        expect_eq(isPowerOfTwoU64(3), false);
        expect_eq(isPowerOfTwoU64((1 as u64) << 63), true);
        expect_eq(isPowerOfTwoU64(U64_MAX), false);

        expect_eq(nextPowerOfTwoU64(0) ?? 0, 1);
        expect_eq(nextPowerOfTwoU64(1) ?? 0, 1);
        expect_eq(nextPowerOfTwoU64(2) ?? 0, 2);
        expect_eq(nextPowerOfTwoU64(3) ?? 0, 4);
        expect_eq(nextPowerOfTwoU64(17) ?? 0, 32);
        let top: u64 = (1 as u64) << 63;
        expect_eq(nextPowerOfTwoU64(top) ?? 0, top);
        expect_eq(nextPowerOfTwoU64(top + 1) == null, true);
    });

    test("6. @cap/num-traits — countOnes, leadingZeros, trailingZeros & rotations", () => {
        expect_eq(countOnesU64(0), 0);
        expect_eq(countOnesU64(U64_MAX), 64);
        let pattern: u64 = ((0xF0F00F0F as u64) << 32) | (0x5555AAAA as u64);
        expect_eq(countOnesU64(pattern), 32);

        expect_eq(leadingZerosU64(0), 64);
        expect_eq(leadingZerosU64(1), 63);
        expect_eq(leadingZerosU64((1 as u64) << 63), 0);
        expect_eq(leadingZerosU64(0x00FF000000000000), 8);

        expect_eq(trailingZerosU64(0), 64);
        expect_eq(trailingZerosU64(1), 0);
        expect_eq(trailingZerosU64(8), 3);
        expect_eq(trailingZerosU64((1 as u64) << 63), 63);

        let v: u64 = ((1 as u64) << 63) | 1;
        expect_eq(rotateLeftU64(v, 0), v);
        expect_eq(rotateLeftU64(v, 64), v);
        expect_eq(rotateLeftU64(v, 1), 3);
        expect_eq(rotateRightU64(v, 1), (3 as u64) << 62);
        expect_eq(rotateRightU64(rotateLeftU64(0x0123456789ABCDEF, 13), 13), 0x0123456789ABCDEF);
    });
}
