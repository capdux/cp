// Source: cp/cap/test/test_uf2_production_deep.cp (Production deep tests for cap:uf2)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    UF2_BLOCK_SIZE,
    UF2_MAX_PAYLOAD,
    UF2_MAGIC_START0,
    UF2_MAGIC_START1,
    UF2_MAGIC_END,
    UF2_FLAG_NOT_MAIN_FLASH,
    UF2_FLAG_FILE_CONTAINER,
    UF2_FLAG_FAMILY_ID_PRESENT,
    UF2_FAMILY_RP2040,
    UF2_FAMILY_ESP32S3,
    Uf2Block,
    Uf2Summary,
    isUf2,
    parseUf2Block,
    Uf2Iter,
    encodeUf2Block,
    summarizeUf2,
} from "cap:uf2";

fn writeU32LE(buf: []u8, offset: usize, val: u32) {
    buf[offset] = (val & 0xFF) as u8;
    buf[offset + 1] = ((val >> 8) & 0xFF) as u8;
    buf[offset + 2] = ((val >> 16) & 0xFF) as u8;
    buf[offset + 3] = ((val >> 24) & 0xFF) as u8;
}

test("1. RP2040 multi-block 256B flash page encode, iterate round-trip, and zero-padding check", () => {
    let image: [1536]u8 = std.mem.zeroes([1536]u8); // 3 blocks * 512B
    let page0: [256]u8 = std.mem.zeroes([256]u8);
    let page1: [256]u8 = std.mem.zeroes([256]u8);
    let page2: [256]u8 = std.mem.zeroes([256]u8);
    page0[0] = 0xAA;
    page0[255] = 0x11;
    page1[0] = 0xBB;
    page1[255] = 0x22;
    page2[0] = 0xCC;
    page2[255] = 0x33;

    // Pre-fill garbage in padding area of block 0 to confirm encodeUf2Block zeroes it
    image[300] = 0xFF;
    image[500] = 0xEE;

    const base_addr: u32 = 0x10000000;
    expect_eq(encodeUf2Block(UF2_FLAG_FAMILY_ID_PRESENT, base_addr, 0, 3, UF2_FAMILY_RP2040, page0[0..], image[0..512]), true);
    expect_eq(encodeUf2Block(UF2_FLAG_FAMILY_ID_PRESENT, base_addr + 256, 1, 3, UF2_FAMILY_RP2040, page1[0..], image[512..1024]), true);
    expect_eq(encodeUf2Block(UF2_FLAG_FAMILY_ID_PRESENT, base_addr + 512, 2, 3, UF2_FAMILY_RP2040, page2[0..], image[1024..1536]), true);

    // Padding bytes cleared
    expect_eq(image[300], 0);
    expect_eq(image[500], 0);
    expect_eq(isUf2(image[0..]), true);

    let it = Uf2Iter.init(image[0..]);
    const b0 = it.next() ?? unreachable;
    expect_eq(b0.target_addr, base_addr);
    expect_eq(b0.block_no, 0);
    expect_eq(b0.num_blocks, 3);
    expect_eq(b0.payload_size, 256);
    expect_eq(b0.payload[0], 0xAA);
    expect_eq(b0.payload[255], 0x11);
    expect_eq(b0.hasFamilyId(), true);
    expect_eq(b0.familyId() ?? 0, UF2_FAMILY_RP2040);

    const b1 = it.next() ?? unreachable;
    expect_eq(b1.target_addr, base_addr + 256);
    expect_eq(b1.payload[0], 0xBB);

    const b2 = it.next() ?? unreachable;
    expect_eq(b2.target_addr, base_addr + 512);
    expect_eq(b2.payload[0], 0xCC);

    expect_eq(it.next() == null, true);
    expect_eq(it.malformed, false);
});

test("2. summarizeUf2 verifies contiguous vs non-contiguous flash address ranges", () => {
    let contig: [1024]u8 = std.mem.zeroes([1024]u8);
    const p = [0x42; 256];
    expect_eq(encodeUf2Block(UF2_FLAG_FAMILY_ID_PRESENT, 0x10000000, 0, 2, UF2_FAMILY_RP2040, p[0..], contig[0..512]), true);
    expect_eq(encodeUf2Block(UF2_FLAG_FAMILY_ID_PRESENT, 0x10000100, 1, 2, UF2_FAMILY_RP2040, p[0..], contig[512..1024]), true);

    const s1 = summarizeUf2(contig[0..]) ?? unreachable;
    expect_eq(s1.block_count, 2);
    expect_eq(s1.total_payload_bytes, 512);
    expect_eq(s1.min_addr, 0x10000000);
    expect_eq(s1.max_addr, 0x10000200);
    expect_eq(s1.family_id ?? 0, UF2_FAMILY_RP2040);
    expect_eq(s1.is_contiguous, true);

    // Non-contiguous gap (0x10000000..0x10000100 then jump to 0x10002000)
    let sparse: [1024]u8 = std.mem.zeroes([1024]u8);
    expect_eq(encodeUf2Block(UF2_FLAG_FAMILY_ID_PRESENT, 0x10000000, 0, 2, UF2_FAMILY_ESP32S3, p[0..], sparse[0..512]), true);
    expect_eq(encodeUf2Block(UF2_FLAG_FAMILY_ID_PRESENT, 0x10002000, 1, 2, UF2_FAMILY_ESP32S3, p[0..], sparse[512..1024]), true);

    const s2 = summarizeUf2(sparse[0..]) ?? unreachable;
    expect_eq(s2.block_count, 2);
    expect_eq(s2.min_addr, 0x10000000);
    expect_eq(s2.max_addr, 0x10002100);
    expect_eq(s2.family_id ?? 0, UF2_FAMILY_ESP32S3);
    expect_eq(s2.is_contiguous, false);
});

test("3. Flag helpers for Family ID, NotMainFlash, and FileContainer modes", () => {
    let blk_buf: [512]u8 = std.mem.zeroes([512]u8);
    const payload = "README.TXT\x00hello";
    const flags = UF2_FLAG_NOT_MAIN_FLASH | UF2_FLAG_FILE_CONTAINER;
    expect_eq(encodeUf2Block(flags, 0, 0, 1, 1024, payload, blk_buf[0..]), true);

    const blk = parseUf2Block(blk_buf[0..]) ?? unreachable;
    expect_eq(blk.hasFamilyId(), false);
    expect_eq(blk.familyId() == null, true);
    expect_eq(blk.isNotMainFlash(), true);
    expect_eq(blk.isFileContainer(), true);
    expect_eq(blk.family_or_file_size, 1024);
    expect_eq(std.mem.eql(u8, blk.payload, payload), true);

    const summary = summarizeUf2(blk_buf[0..]) ?? unreachable;
    expect_eq(summary.family_id == null, true);
    expect_eq(summary.block_count, 1);
    expect_eq(summary.total_payload_bytes, payload.len);
});

test("4. Encode boundary validation for max payload 476B, oversized payload, and invalid block indices", () => {
    let out: [512]u8 = std.mem.zeroes([512]u8);
    const max_p = [0x55; 476];
    expect_eq(encodeUf2Block(0, 0x08000000, 0, 1, 0, max_p[0..], out[0..]), true);
    const parsed = parseUf2Block(out[0..]) ?? unreachable;
    expect_eq(parsed.payload_size, UF2_MAX_PAYLOAD as u32);
    expect_eq(parsed.payload[475], 0x55);

    // Payload > 476 -> reject
    const too_big = [0x11; 477];
    expect_eq(encodeUf2Block(0, 0, 0, 1, 0, too_big[0..], out[0..]), false);

    // Output buffer < 512 -> reject
    expect_eq(encodeUf2Block(0, 0, 0, 1, 0, "ok", out[0..511]), false);

    // num_blocks == 0 or block_no >= num_blocks -> reject
    expect_eq(encodeUf2Block(0, 0, 0, 0, 0, "ok", out[0..]), false);
    expect_eq(encodeUf2Block(0, 0, 2, 2, 0, "ok", out[0..]), false);
});

test("5. Malformed UF2 inputs (bad magic, non-512 multiple, payload_size > 476, block_no >= num_blocks) fail safely", () => {
    let valid: [512]u8 = std.mem.zeroes([512]u8);
    expect_eq(encodeUf2Block(UF2_FLAG_FAMILY_ID_PRESENT, 0x10000000, 0, 1, UF2_FAMILY_RP2040, "abc", valid[0..]), true);

    // Non-512 multiple / empty
    expect_eq(isUf2(""), false);
    expect_eq(isUf2(valid[0..511]), false);
    expect_eq(summarizeUf2(valid[0..511]) == null, true);

    // Bad magic start0 / start1 / end
    let bad_m0: [512]u8 = valid;
    writeU32LE(bad_m0[0..], 0, 0xDEADBEEF);
    expect_eq(isUf2(bad_m0[0..]), false);
    expect_eq(parseUf2Block(bad_m0[0..]) == null, true);

    let bad_m1: [512]u8 = valid;
    writeU32LE(bad_m1[0..], 4, 0x00000000);
    expect_eq(isUf2(bad_m1[0..]), false);

    let bad_end: [512]u8 = valid;
    writeU32LE(bad_end[0..], 508, 0x12345678);
    expect_eq(isUf2(bad_end[0..]), false);
    expect_eq(parseUf2Block(bad_end[0..]) == null, true);

    // payload_size > 476
    let bad_psize: [512]u8 = valid;
    writeU32LE(bad_psize[0..], 16, 477);
    expect_eq(parseUf2Block(bad_psize[0..]) == null, true);
    expect_eq(summarizeUf2(bad_psize[0..]) == null, true);

    // block_no >= num_blocks
    let bad_bno: [512]u8 = valid;
    writeU32LE(bad_bno[0..], 20, 1);
    writeU32LE(bad_bno[0..], 24, 1);
    expect_eq(parseUf2Block(bad_bno[0..]) == null, true);
    expect_eq(summarizeUf2(bad_bno[0..]) == null, true);
});
