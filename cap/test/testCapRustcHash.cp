import { test, expect_eq } from "cap:test";
import {
    FX_SEED_64,
    FX_SEED_32,
    FxHasher64,
    FxHasher32,
    fxHash64,
    fxHash64Str,
    fxHash32
} from "../../packages/rustc-hash/src/index.cp";

fn main() {
    test("1. @cap/rustc-hash — exact FX_SEED_64 & FX_SEED_32 constants and single-word formula", () => {
        let expected_seed64: u64 = ((0x517cc1b7 as u64) << 32) | (0x27220a95 as u64);
        expect_eq(FX_SEED_64, expected_seed64);
        expect_eq(FX_SEED_32, 2654435769);

        // From state 0, writing word 1 yields (rotl64(0, 5) ^ 1) * FX_SEED_64 == FX_SEED_64
        let h64 = FxHasher64.new();
        h64.writeU64(1);
        expect_eq(h64.finish(), FX_SEED_64);

        // From state 0, writing word 1 in 32-bit yields FX_SEED_32
        let h32 = FxHasher32.new();
        h32.writeU32(1);
        expect_eq(h32.finish(), FX_SEED_32);
    });

    test("2. @cap/rustc-hash — empty input and reset behavior", () => {
        let empty: [1]u8 = [0];
        expect_eq(fxHash64(empty[0..0]), 0);
        expect_eq(fxHash64Str(""), 0);
        expect_eq(fxHash32(empty[0..0]), 0);

        let h = FxHasher64.new();
        h.writeStr("rustc-hash");
        expect_eq(h.finish() != 0, true);
        h.reset();
        expect_eq(h.finish(), 0);
    });

    test("3. @cap/rustc-hash — writeBytes and writeStr produce identical hashes across all tail lengths", () => {
        // Length 15 exercises 8 + 4 + 2 + 1 byte tail path in FxHasher64 and 4*3 + 2 + 1 in FxHasher32
        let raw: [15]u8 = [
            104, 101, 108, 108, 111, 32, 102, 120,
            104, 97, 115, 104, 33, 33, 33
        ]; // "hello fxhash!!!"
        let h_bytes = fxHash64(raw[0..15]);
        let h_str = fxHash64Str("hello fxhash!!!");
        expect_eq(h_bytes, h_str);
        expect_eq(h_bytes != 0, true);

        // Length 8 byte slice matches single little-endian writeU64
        let le8: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
        let w64: u64 = ((0x08070605 as u64) << 32) | (0x04030201 as u64);
        let hw = FxHasher64.new();
        hw.writeU64(w64);
        expect_eq(fxHash64(le8[0..8]), hw.finish());
    });

    test("4. @cap/rustc-hash — FxHasher64 withSeed, writeUsize, writeU32 and collision resistance", () => {
        let h1 = FxHasher64.withSeed(42);
        let h2 = FxHasher64.withSeed(43);
        h1.writeUsize(1024);
        h2.writeUsize(1024);
        expect_eq(h1.finish() != h2.finish(), true);

        let ha = FxHasher64.new();
        ha.writeU32(12345);
        let hb = FxHasher64.new();
        hb.writeU32(12346);
        expect_eq(ha.finish() != hb.finish(), true);
    });

    test("5. @cap/rustc-hash — FxHasher32 matches little-endian writeU32 and handles tail bytes", () => {
        let le4: [4]u8 = [0x78, 0x56, 0x34, 0x12];
        let h32 = FxHasher32.new();
        h32.writeU32(305419896); // 0x12345678
        expect_eq(fxHash32(le4[0..4]), h32.finish());

        // Length 7 exercises 4 + 2 + 1 path
        let raw7: [7]u8 = [10, 20, 30, 40, 50, 60, 70];
        let h_a = fxHash32(raw7[0..7]);
        raw7[6] = 71;
        let h_b = fxHash32(raw7[0..7]);
        expect_eq(h_a != h_b, true);
    });

    test("6. @cap/rustc-hash — multi-step wrapping multiplication stress without overflow", () => {
        let h64 = FxHasher64.new();
        let h32 = FxHasher32.new();
        let max64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        let i: u32 = 0;
        while (i < 32) {
            h64.writeU64(max64 - (i as u64));
            h32.writeU32(4294967295 - i);
            i = i + 1;
        }
        expect_eq(h64.finish() != 0, true);
        expect_eq(h32.finish() != 0, true);
    });
}
