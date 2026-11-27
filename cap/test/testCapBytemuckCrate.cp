import { test, expect_eq } from "cap:test";
import {
    POD_OK, POD_ERR_SIZE_MISMATCH, POD_ERR_ALIGNMENT,
    checkCastLayout, writeU16Le, readU16Le, writeU32Le, readU32Le,
    writeU32Be, readU32Be, writeU64Le, readU64Le,
    castU32SliceToBytesLe, castBytesToU32SliceLe, swapBytesU32
} from "../../packages/bytemuck/src/index.cp";

fn main() {
    test("1. @cap/bytemuck-crate — checkCastLayout validates size multiple & alignment", () => {
        expect_eq(checkCastLayout(16, 4, 4096, 4), POD_OK);
        expect_eq(checkCastLayout(15, 4, 4096, 4), POD_ERR_SIZE_MISMATCH);
        expect_eq(checkCastLayout(16, 4, 4098, 4), POD_ERR_ALIGNMENT);
    });

    test("2. @cap/bytemuck-crate — u16 & u32 LE/BE roundtrip", () => {
        let b2: [2]u8 = [0; 2];
        expect_eq(writeU16Le(4660, b2[0..2]), true); // 0x1234
        expect_eq(b2[0], 52); // 0x34
        expect_eq(b2[1], 18); // 0x12
        expect_eq(readU16Le(b2[0..2]) ?? 0, 4660);

        let b4: [4]u8 = [0; 4];
        expect_eq(writeU32Be(305419896, b4[0..4]), true); // 0x12345678
        expect_eq(b4[0], 18);
        expect_eq(b4[3], 120);
        expect_eq(readU32Be(b4[0..4]) ?? 0, 305419896);
    });

    test("3. @cap/bytemuck-crate — u64 LE roundtrip with high bits", () => {
        let val: u64 = ((305419896 as u64) << 32) | (2596069104 as u64);
        let b8: [8]u8 = [0; 8];
        expect_eq(writeU64Le(val, b8[0..8]), true);
        expect_eq(readU64Le(b8[0..8]) ?? 0, val);
    });

    test("4. @cap/bytemuck-crate — castU32SliceToBytesLe & castBytesToU32SliceLe roundtrip", () => {
        let words: [3]u32 = [1, 65536, 305419896];
        let bytes: [12]u8 = [0; 12];
        let n_bytes = castU32SliceToBytesLe(words[0..3], bytes[0..12]) ?? 0;
        expect_eq(n_bytes, 12);

        let back: [3]u32 = [0; 3];
        let n_words = castBytesToU32SliceLe(bytes[0..12], back[0..3]) ?? 0;
        expect_eq(n_words, 3);
        expect_eq(back[0], 1);
        expect_eq(back[1], 65536);
        expect_eq(back[2], 305419896);
    });

    test("5. @cap/bytemuck-crate — rejects misaligned byte slice lengths & short buffers", () => {
        let odd: [5]u8 = [0; 5];
        let out: [2]u32 = [0; 2];
        expect_eq(castBytesToU32SliceLe(odd[0..5], out[0..2]) == null, true);
        expect_eq(readU32Le(odd[0..3]) == null, true);
        expect_eq(writeU64Le(1, odd[0..5]), false);
    });

    test("6. @cap/bytemuck-crate — swapBytesU32 reverses byte order", () => {
        expect_eq(swapBytesU32(305419896), 2018915346); // 0x12345678 -> 0x78563412
        expect_eq(swapBytesU32(swapBytesU32(305419896)), 305419896);
    });
}
