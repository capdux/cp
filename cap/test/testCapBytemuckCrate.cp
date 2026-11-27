import { test, expect_eq } from "cap:test";
import {
    POD_OK, POD_ERR_SIZE_MISMATCH, POD_ERR_ALIGNMENT, POD_ERR_OUTPUT_SLICE_TOO_SMALL,
    checkCastLayout, isAligned, fillZeroed,
    zeroedU8, zeroedU16, zeroedU32, zeroedU64, zeroedF32, zeroedF64,
    writeU16Le, readU16Le, writeU16Be, readU16Be,
    writeU32Le, readU32Le, writeU32Be, readU32Be,
    writeU64Le, readU64Le, writeU64Be, readU64Be,
    writeF32Le, readF32Le, writeF64Le, readF64Le,
    bytesOfU16, bytesOfU32, bytesOfU64, bytesOfF32, bytesOfF64,
    fromBytesU16, fromBytesU32, fromBytesU64, fromBytesF32, fromBytesF64,
    podReadUnalignedU16, podReadUnalignedU32, podReadUnalignedU64, podReadUnalignedI32,
    castU16SliceToBytesLe, castBytesToU16SliceLe,
    castU32SliceToBytesLe, castBytesToU32SliceLe,
    castU64SliceToBytesLe, castBytesToU64SliceLe,
    tryCastBytesToU16Slice, tryCastBytesToU32Slice, tryCastBytesToU64Slice,
    castU32SliceAsBytes, tryCastSliceBytesToU32,
    swapBytesU16, swapBytesU32, swapBytesU64,
} from "../../packages/bytemuck/src/index.cp";

fn main() {
    test("1. @cap/bytemuck-crate — checkCastLayout validates size multiple & alignment", () => {
        expect_eq(checkCastLayout(16, 4, 4096, 4), POD_OK);
        expect_eq(checkCastLayout(15, 4, 4096, 4), POD_ERR_SIZE_MISMATCH);
        expect_eq(checkCastLayout(16, 4, 4098, 4), POD_ERR_ALIGNMENT);
        expect_eq(isAligned(4096, 8), true);
        expect_eq(isAligned(4098, 4), false);
    });

    test("2. @cap/bytemuck-crate — u16 & u32 LE/BE roundtrip", () => {
        let b2: [2]u8 = [0; 2];
        expect_eq(writeU16Le(4660, b2[0..2]), true); // 0x1234
        expect_eq(b2[0], 52); // 0x34
        expect_eq(b2[1], 18); // 0x12
        expect_eq(readU16Le(b2[0..2]) ?? 0, 4660);

        expect_eq(writeU16Be(4660, b2[0..2]), true);
        expect_eq(b2[0], 18);
        expect_eq(readU16Be(b2[0..2]) ?? 0, 4660);

        let b4: [4]u8 = [0; 4];
        expect_eq(writeU32Be(305419896, b4[0..4]), true); // 0x12345678
        expect_eq(b4[0], 18);
        expect_eq(b4[3], 120);
        expect_eq(readU32Be(b4[0..4]) ?? 0, 305419896);
    });

    test("3. @cap/bytemuck-crate — u64 LE/BE & f32/f64 roundtrip", () => {
        let val: u64 = ((305419896 as u64) << 32) | (2596069104 as u64);
        let b8: [8]u8 = [0; 8];
        expect_eq(writeU64Le(val, b8[0..8]), true);
        expect_eq(readU64Le(b8[0..8]) ?? 0, val);
        expect_eq(writeU64Be(val, b8[0..8]), true);
        expect_eq(readU64Be(b8[0..8]) ?? 0, val);

        let f4: [4]u8 = [0; 4];
        expect_eq(writeF32Le(3.5, f4[0..4]), true);
        expect_eq(readF32Le(f4[0..4]) ?? 0.0, 3.5 as f32);

        expect_eq(writeF64Le(123.25, b8[0..8]), true);
        expect_eq(readF64Le(b8[0..8]) ?? 0.0, 123.25 as f64);
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

    test("6. @cap/bytemuck-crate — swapBytesU16/U32/U64", () => {
        expect_eq(swapBytesU16(0x1234), 0x3412 as u16);
        expect_eq(swapBytesU32(305419896), 2018915346); // 0x12345678 -> 0x78563412
        expect_eq(swapBytesU32(swapBytesU32(305419896)), 305419896);
        const v64: u64 = 0x0102030405060708 as u64;
        expect_eq(swapBytesU64(swapBytesU64(v64)), v64);
    });

    test("7. @cap/bytemuck-crate — bytesOf, fromBytes, podReadUnaligned & zeroed", () => {
        expect_eq(zeroedU8(), 0 as u8);
        expect_eq(zeroedU16(), 0 as u16);
        expect_eq(zeroedU32(), 0 as u32);
        expect_eq(zeroedU64(), 0 as u64);
        expect_eq(zeroedF32(), 0.0 as f32);
        expect_eq(zeroedF64(), 0.0 as f64);

        let zbuf: [4]u8 = [1, 2, 3, 4];
        fillZeroed(zbuf[0..4]);
        expect_eq(zbuf[0], 0 as u8);
        expect_eq(zbuf[3], 0 as u8);

        const b16 = bytesOfU16(0xBEEF);
        expect_eq(fromBytesU16(b16[0..2]) ?? 0, 0xBEEF as u16);
        expect_eq(fromBytesU16(b16[0..1]) == null, true);

        const b32 = bytesOfU32(0xDEADBEEF);
        expect_eq(fromBytesU32(b32[0..4]) ?? 0, 0xDEADBEEF as u32);
        expect_eq(fromBytesU32(b32[0..3]) == null, true);

        const b64 = bytesOfU64(0x1122334455667788);
        expect_eq(fromBytesU64(b64[0..8]) ?? 0, 0x1122334455667788 as u64);

        const bf32 = bytesOfF32(2.25);
        expect_eq(fromBytesF32(bf32[0..4]) ?? 0.0, 2.25 as f32);
        const bf64 = bytesOfF64(9.75);
        expect_eq(fromBytesF64(bf64[0..8]) ?? 0.0, 9.75 as f64);

        // Unaligned read starting at odd offset 1
        let unaligned: [9]u8 = [0xAA, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00, 0x00, 0x00];
        expect_eq(podReadUnalignedU32(unaligned[1..5]) ?? 0, 0x12345678 as u32);
        expect_eq(podReadUnalignedU16(unaligned[1..3]) ?? 0, 0x5678 as u16);
        expect_eq(podReadUnalignedI32(unaligned[1..5]) ?? 0, 0x12345678 as i32);
        expect_eq(podReadUnalignedU64(unaligned[1..9]) ?? 0, 0x0000000012345678 as u64);
    });

    test("8. @cap/bytemuck-crate — tryCastBytesTo*Slice & zero-copy castU32SliceAsBytes / tryCastSliceBytesToU32", () => {
        let u16s: [2]u16 = [0x1122, 0x3344];
        let b4: [4]u8 = [0; 4];
        expect_eq(castU16SliceToBytesLe(u16s[0..2], b4[0..4]) ?? 0, 4 as usize);
        let u16_out: [2]u16 = [0; 2];
        expect_eq(castBytesToU16SliceLe(b4[0..4], u16_out[0..2]) ?? 0, 2 as usize);
        expect_eq(u16_out[1], 0x3344 as u16);

        const r_ok = tryCastBytesToU16Slice(b4[0..4], u16_out[0..2]);
        expect_eq(r_ok.isOk(), true);
        expect_eq(r_ok.count, 2 as usize);

        const r_small = tryCastBytesToU16Slice(b4[0..4], u16_out[0..1]);
        expect_eq(r_small.err_code, POD_ERR_OUTPUT_SLICE_TOO_SMALL);

        let u64s: [2]u64 = [100, 200];
        let b16: [16]u8 = [0; 16];
        expect_eq(castU64SliceToBytesLe(u64s[0..2], b16[0..16]) ?? 0, 16 as usize);
        let u64_out: [2]u64 = [0; 2];
        const r64 = tryCastBytesToU64Slice(b16[0..16], u64_out[0..2]);
        expect_eq(r64.isOk(), true);
        expect_eq(u64_out[1], 200 as u64);

        let words: [2]u32 = [0x11223344, 0x55667788];
        const raw_view = castU32SliceAsBytes(words[0..2]);
        expect_eq(raw_view.len, 8 as usize);
        const casted = tryCastSliceBytesToU32(raw_view);
        expect_eq(casted != null, true);
        if (s := casted) {
            expect_eq(s.len, 2 as usize);
            expect_eq(s[0], 0x11223344 as u32);
            expect_eq(s[1], 0x55667788 as u32);
        }
        // Misaligned sub-slice [1..5] must return null
        expect_eq(tryCastSliceBytesToU32(raw_view[1..5]) == null, true);

        const r32_bad = tryCastBytesToU32Slice(raw_view[0..5], words[0..2]);
        expect_eq(r32_bad.err_code, POD_ERR_SIZE_MISMATCH);
    });
}
