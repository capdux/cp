import { test, expect_eq } from "cap:test";
import {
    zeroizeBytes, zeroizeU32, zeroizeU64, isAllZeroBytes,
    ZeroizingKey32, ZeroizingBuf64
} from "../../packages/zeroize/src/index.cp";

fn main() {
    test("1. @cap/zeroize — zeroizeBytes clears secret byte buffer completely", () => {
        let secret: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
        expect_eq(isAllZeroBytes(secret[0..8]), false);
        expect_eq(zeroizeBytes(secret[0..8]), 8);
        expect_eq(isAllZeroBytes(secret[0..8]), true);
    });

    test("2. @cap/zeroize — zeroizeU32 & zeroizeU64 clear integer arrays", () => {
        let w32: [4]u32 = [11, 22, 33, 44];
        expect_eq(zeroizeU32(w32[0..4]), 4);
        expect_eq(w32[0] + w32[1] + w32[2] + w32[3], 0);

        let w64: [3]u64 = [100, 200, 300];
        expect_eq(zeroizeU64(w64[0..3]), 3);
        expect_eq(w64[0] + w64[1] + w64[2], 0);
    });

    test("3. @cap/zeroize — ZeroizingKey32 copies 32-byte key and wipes on zeroize()", () => {
        let raw: [32]u8 = [170; 32];
        let k = ZeroizingKey32.fromSlice(raw[0..32]) ?? ZeroizingKey32 { bytes: [0; 32], active: false };
        expect_eq(k.active, true);
        expect_eq(k.bytes[0], 170);
        expect_eq(k.isZeroed(), false);

        k.zeroize();
        expect_eq(k.isZeroed(), true);
        expect_eq(k.bytes[31], 0);
    });

    test("4. @cap/zeroize — ZeroizingKey32 rejects non-32-byte slices", () => {
        let short_k: [16]u8 = [1; 16];
        expect_eq(ZeroizingKey32.fromSlice(short_k[0..16]) == null, true);
    });

    test("5. @cap/zeroize — ZeroizingBuf64 stores variable-length secrets and wipes cleanly", () => {
        let pw: [12]u8 = [99; 12];
        let b = ZeroizingBuf64.fromSlice(pw[0..12]) ?? ZeroizingBuf64 { bytes: [0; 64], len: 0, active: false };
        expect_eq(b.len, 12);
        expect_eq(b.active, true);
        b.zeroize();
        expect_eq(b.isZeroed(), true);
    });

    test("6. @cap/zeroize — ZeroizingBuf64 rejects slices > 64 bytes & empty isAllZeroBytes", () => {
        let too_big: [65]u8 = [1; 65];
        expect_eq(ZeroizingBuf64.fromSlice(too_big[0..65]) == null, true);
        expect_eq(isAllZeroBytes(too_big[0..0]), true);
    });
}
