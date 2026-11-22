// Source: cp/cap/test/test_ssz_production_deep.cp (Production deep tests for cap:ssz)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SSZ_OFFSET_BYTES,
    SSZ_CHUNK_BYTES,
    SszWriter,
    SszReader,
    sszBitlistLen,
    sszBitlistGet,
    sszBitvectorValid,
    sszBitvectorGet,
    SszOffsetListIter,
    sszChunkCount,
    sszNextPowerOfTwo,
} from "cap:binfmt";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. Basic SSZ scalar roundtrip (bool, u8, u16, u32, u64, u128, fixed bytes, zero pad) and strict bool", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let w = SszWriter.init(buf[0..]);

    const big128: u128 = ((0x01234567_89ABCDEF as u128) << 64) | (0xFEDCBA98_76543210 as u128);
    expect_eq(w.writeBool(true), true);
    expect_eq(w.writeBool(false), true);
    expect_eq(w.writeU8(0x5A), true);
    expect_eq(w.writeU16(0xBEEF), true);
    expect_eq(w.writeU32(0x11223344), true);
    expect_eq(w.writeU64(0x01020304_05060708), true);
    expect_eq(w.writeU128(big128), true);
    expect_eq(w.writeZeroPad(3), true);

    const enc = w.written();
    expect_eq(enc.len, 36);
    // Check little-endian byte order on u16 (offset 3..5)
    expect_eq(enc[3], 0xEF);
    expect_eq(enc[4], 0xBE);

    let r = SszReader.init(enc);
    expect_eq(r.readBool() ?? false, true);
    expect_eq(r.readBool() ?? true, false);
    expect_eq(r.readU8() ?? 0, 0x5A);
    expect_eq(r.readU16() ?? 0, 0xBEEF);
    expect_eq(r.readU32() ?? 0, 0x11223344);
    expect_eq(r.readU64() ?? 0, 0x01020304_05060708);
    expect_eq(r.readU128() ?? 0, big128);
    const pad = r.readFixedBytes(3) ?? unreachable;
    expect_eq(pad[0], 0);
    expect_eq(pad[2], 0);
    expect_eq(r.isEof(), true);

    // Invalid bool byte (0x02) rejected
    const bad_bool: [1]u8 = [0x02];
    let rb = SszReader.init(bad_bool[0..]);
    expect_eq(rb.readBool() == null, true);
});

test("2. SSZ Bitlist delimiter bit encoding/decoding and invalid empty or 0x00 tail rejection", () => {
    // 5-bit bitlist: [1, 0, 1, 1, 0] + delimiter at bit 5 => 0b0010_1101 = 0x2D
    let buf: [8]u8 = std.mem.zeroes([8]u8);
    let w = SszWriter.init(buf[0..]);
    const bits5: [5]bool = [true, false, true, true, false];
    expect_eq(w.writeBitlist(bits5[0..]), true);
    const enc5 = w.written();
    expect_eq(enc5.len, 1);
    expect_eq(enc5[0], 0x2D);
    expect_eq(sszBitlistLen(enc5) ?? 99, 5);
    expect_eq(sszBitlistGet(enc5, 0) ?? false, true);
    expect_eq(sszBitlistGet(enc5, 1) ?? true, false);
    expect_eq(sszBitlistGet(enc5, 2) ?? false, true);
    expect_eq(sszBitlistGet(enc5, 3) ?? false, true);
    expect_eq(sszBitlistGet(enc5, 4) ?? true, false);
    expect_eq(sszBitlistGet(enc5, 5) == null, true);

    // 8-bit bitlist spills delimiter bit into 2nd byte (0x01)
    let buf8: [8]u8 = std.mem.zeroes([8]u8);
    let w8 = SszWriter.init(buf8[0..]);
    const bits8: [8]bool = [true, true, false, false, true, false, true, true];
    expect_eq(w8.writeBitlist(bits8[0..]), true);
    const enc8 = w8.written();
    expect_eq(enc8.len, 2);
    expect_eq(enc8[1], 0x01);
    expect_eq(sszBitlistLen(enc8) ?? 0, 8);
    expect_eq(sszBitlistGet(enc8, 7) ?? false, true);

    // Empty bitlist encodes as [0x01] with bit length 0
    const empty_bl: [1]u8 = [0x01];
    expect_eq(sszBitlistLen(empty_bl[0..]) ?? 99, 0);

    // Invalid bitlists: empty slice or trailing 0x00 byte
    expect_eq(sszBitlistLen(buf[0..0]) == null, true);
    const bad_zero_tail: [2]u8 = [0xFF, 0x00];
    expect_eq(sszBitlistLen(bad_zero_tail[0..]) == null, true);
});

test("3. SSZ Bitvector packing and trailing unused high-bit zero validation", () => {
    let buf: [8]u8 = std.mem.zeroes([8]u8);
    let w = SszWriter.init(buf[0..]);
    const bv_bits: [10]bool = [true, false, false, true, false, false, false, false, true, true];
    expect_eq(w.writeBitvector(bv_bits[0..]), true);
    const enc = w.written();
    expect_eq(enc.len, 2);
    expect_eq(enc[0], 0x09); // bits 0 and 3
    expect_eq(enc[1], 0x03); // bits 8 and 9

    expect_eq(sszBitvectorValid(enc, 10), true);
    expect_eq(sszBitvectorGet(enc, 10, 0) ?? false, true);
    expect_eq(sszBitvectorGet(enc, 10, 1) ?? true, false);
    expect_eq(sszBitvectorGet(enc, 10, 9) ?? false, true);
    expect_eq(sszBitvectorGet(enc, 10, 10) == null, true);

    // Set unused bit 10 (bit 2 of byte 1) => must fail validation
    let dirty: [2]u8 = [0x09, 0x07];
    expect_eq(sszBitvectorValid(dirty[0..], 10), false);
    expect_eq(sszBitvectorGet(dirty[0..], 10, 0) == null, true);

    // Wrong byte length rejected
    expect_eq(sszBitvectorValid(enc, 8), false);
});

test("4. SszOffsetListIter zero-copy variable list extraction and malformed offset rejection", () => {
    // List[Bytes] with 3 items: "eth", "", "beacon"
    // Offset table = 3 * 4 = 12 bytes: [12, 15, 15]
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let w = SszWriter.init(buf[0..]);
    expect_eq(w.writeOffset(12), true);
    expect_eq(w.writeOffset(15), true);
    expect_eq(w.writeOffset(15), true);
    expect_eq(w.writeFixedBytes("eth"), true);
    expect_eq(w.writeFixedBytes("beacon"), true);

    let it = SszOffsetListIter.init(w.written());
    expect_eq(it.isValid(), true);
    expect_eq(it.len(), 3);

    const e0 = it.next() ?? unreachable;
    const e1 = it.next() ?? unreachable;
    const e2 = it.next() ?? unreachable;
    expect_eq(sliceEq(e0, "eth"), true);
    expect_eq(e1.len, 0);
    expect_eq(sliceEq(e2, "beacon"), true);
    expect_eq(it.next() == null, true);

    // Empty list is valid with 0 items
    let empty_it = SszOffsetListIter.init(buf[0..0]);
    expect_eq(empty_it.isValid(), true);
    expect_eq(empty_it.len(), 0);

    // Non-monotonic offsets [8, 10, 9] rejected
    let bad_order: [12]u8 = std.mem.zeroes([12]u8);
    let wb = SszWriter.init(bad_order[0..]);
    * = wb.writeOffset(12);
    * = wb.writeOffset(12);
    * = wb.writeOffset(10);
    let bit = SszOffsetListIter.init(wb.written());
    expect_eq(bit.isValid(), false);
    expect_eq(bit.next() == null, true);

    // Unaligned first offset (5 % 4 != 0) rejected
    let bad_align: [8]u8 = [5, 0, 0, 0, 0, 0, 0, 0];
    let ait = SszOffsetListIter.init(bad_align[0..]);
    expect_eq(ait.isValid(), false);

    // Out-of-bounds offset (> data.len) rejected
    let bad_oob: [8]u8 = [8, 0, 0, 0, 99, 0, 0, 0];
    let oit = SszOffsetListIter.init(bad_oob[0..]);
    expect_eq(oit.isValid(), false);
});

test("5. sszChunkCount and sszNextPowerOfTwo Merkleization helpers", () => {
    expect_eq(SSZ_OFFSET_BYTES, 4);
    expect_eq(SSZ_CHUNK_BYTES, 32);

    expect_eq(sszChunkCount(0), 0);
    expect_eq(sszChunkCount(1), 1);
    expect_eq(sszChunkCount(32), 1);
    expect_eq(sszChunkCount(33), 2);
    expect_eq(sszChunkCount(64), 2);
    expect_eq(sszChunkCount(65), 3);

    expect_eq(sszNextPowerOfTwo(0), 1);
    expect_eq(sszNextPowerOfTwo(1), 1);
    expect_eq(sszNextPowerOfTwo(2), 2);
    expect_eq(sszNextPowerOfTwo(3), 4);
    expect_eq(sszNextPowerOfTwo(5), 8);
    expect_eq(sszNextPowerOfTwo(16), 16);
    expect_eq(sszNextPowerOfTwo(17), 32);
});
