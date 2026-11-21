// Source: cp/cap/test/test_apk_production_deep.cp (Production deep tests for cap:apk)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ZIP_EOCD_SIG,
    ZIP_EOCD_MIN_SIZE,
    APK_SIG_BLOCK_MAGIC,
    APK_SIG_V2_BLOCK_ID,
    APK_SIG_V3_BLOCK_ID,
    APK_SIG_V3_1_BLOCK_ID,
    APK_VERITY_PADDING_BLOCK_ID,
    APK_DEPENDENCY_INFO_BLOCK_ID,
    ApkZipEocd,
    ApkSigningBlock,
    ApkSigPair,
    ApkSigSummary,
    findZipEocd,
    findApkSigningBlock,
    ApkSigPairIter,
    findApkSigPair,
    summarizeApkSigning,
} from "cap:apk";

fn writeU16LE(buf: []u8, offset: usize, val: u16) {
    buf[offset] = (val & 0xFF) as u8;
    buf[offset + 1] = ((val >> 8) & 0xFF) as u8;
}

fn writeU32LE(buf: []u8, offset: usize, val: u32) {
    buf[offset] = (val & 0xFF) as u8;
    buf[offset + 1] = ((val >> 8) & 0xFF) as u8;
    buf[offset + 2] = ((val >> 16) & 0xFF) as u8;
    buf[offset + 3] = ((val >> 24) & 0xFF) as u8;
}

fn writeU64LE(buf: []u8, offset: usize, val: u64) {
    writeU32LE(buf, offset, (val & 0xFFFFFFFF) as u32);
    writeU32LE(buf, offset + 4, ((val >> 32) & 0xFFFFFFFF) as u32);
}

fn copyBytes(dst: []u8, offset: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[offset + i] = src[i];
        i += 1;
    }
}

fn appendPair(buf: []u8, pos: *usize, id: u32, payload: []const u8) {
    const pair_len = u64(4 + payload.len);
    writeU64LE(buf, pos.*, pair_len);
    pos.* += 8;
    writeU32LE(buf, pos.*, id);
    pos.* += 4;
    copyBytes(buf, pos.*, payload);
    pos.* += payload.len;
}

/// 构造完整合成 APK：Local File + APK Signing Block (v2 + v3 + Verity Padding) + Central Directory + EOCD (含注释)
struct BuiltApk {
    total_len: usize,
    sig_block_offset: usize,
    cd_offset: usize,
    eocd_offset: usize,
}

fn buildFullSignedApk(buf: []u8, comment: []const u8): BuiltApk {
    let pos: usize = 0;

    // 1. Minimal ZIP Local File Entry (30B header + 11B "classes.dex" + 4B payload = 45B)
    writeU32LE(buf, pos, 0x04034B50);
    writeU16LE(buf, pos + 26, 11);
    copyBytes(buf, pos + 30, "classes.dex");
    copyBytes(buf, pos + 41, "DEX!");
    pos += 45;

    // 2. APK Signing Block
    const sig_block_offset = pos;
    pos += 8; // reserve top size_of_block

    const pairs_start = pos;
    appendPair(buf, &pos, APK_SIG_V2_BLOCK_ID, "V2_SIG_DATA");
    appendPair(buf, &pos, APK_SIG_V3_BLOCK_ID, "V3_SIG_PAYLOAD_XYZ");
    appendPair(buf, &pos, APK_VERITY_PADDING_BLOCK_ID, "\x00\x00\x00\x00");
    const pairs_len = pos - pairs_start;

    const size_of_block = u64(pairs_len + 8 + 16);
    writeU64LE(buf, sig_block_offset, size_of_block);
    writeU64LE(buf, pos, size_of_block);
    pos += 8;
    copyBytes(buf, pos, APK_SIG_BLOCK_MAGIC);
    pos += 16;

    // 3. Minimal Central Directory Header (46B + 11B filename = 57B)
    const cd_offset = pos;
    writeU32LE(buf, pos, 0x02014B50);
    writeU16LE(buf, pos + 28, 11);
    copyBytes(buf, pos + 46, "classes.dex");
    const cd_size: u32 = 57;
    pos += usize(cd_size);

    // 4. ZIP EOCD (22B + comment)
    const eocd_offset = pos;
    writeU32LE(buf, pos, ZIP_EOCD_SIG);
    writeU16LE(buf, pos + 8, 1);
    writeU16LE(buf, pos + 10, 1);
    writeU32LE(buf, pos + 12, cd_size);
    writeU32LE(buf, pos + 16, cd_offset as u32);
    writeU16LE(buf, pos + 20, comment.len as u16);
    pos += ZIP_EOCD_MIN_SIZE;
    copyBytes(buf, pos, comment);
    pos += comment.len;

    return BuiltApk {
        total_len: pos,
        sig_block_offset: sig_block_offset,
        cd_offset: cd_offset,
        eocd_offset: eocd_offset,
    };
}

test("1. Full APK EOCD & Signing Block parsing with ZIP comment and v2/v3/Verity Padding pairs", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    const meta = buildFullSignedApk(buf[0..], "Signed by CP APK Builder");
    const apk = buf[0..meta.total_len];

    const eocd = findZipEocd(apk) ?? unreachable;
    expect_eq(eocd.eocd_offset, meta.eocd_offset);
    expect_eq(eocd.cd_entries, 1);
    expect_eq(eocd.cd_size, 57);
    expect_eq(eocd.cd_offset, meta.cd_offset as u32);
    expect_eq(eocd.comment_len, 24);

    const block = findApkSigningBlock(apk) ?? unreachable;
    expect_eq(block.block_offset, meta.sig_block_offset);
    expect_eq(block.cd_offset, meta.cd_offset);

    let it = ApkSigPairIter.init(block);
    const p1 = it.next() ?? unreachable;
    expect_eq(p1.id, APK_SIG_V2_BLOCK_ID);
    expect_eq(std.mem.eql(u8, p1.value, "V2_SIG_DATA"), true);

    const p2 = it.next() ?? unreachable;
    expect_eq(p2.id, APK_SIG_V3_BLOCK_ID);
    expect_eq(std.mem.eql(u8, p2.value, "V3_SIG_PAYLOAD_XYZ"), true);

    const p3 = it.next() ?? unreachable;
    expect_eq(p3.id, APK_VERITY_PADDING_BLOCK_ID);
    expect_eq(p3.value.len, 4);

    expect_eq(it.next() == null, true);
    expect_eq(it.malformed, false);
});

test("2. findApkSigPair and summarizeApkSigning accurately report v2/v3/v3.1/padding flags", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    const meta = buildFullSignedApk(buf[0..], "");
    const apk = buf[0..meta.total_len];

    const block = findApkSigningBlock(apk) ?? unreachable;
    const v2 = findApkSigPair(block, APK_SIG_V2_BLOCK_ID) ?? unreachable;
    expect_eq(std.mem.eql(u8, v2, "V2_SIG_DATA"), true);

    const v3 = findApkSigPair(block, APK_SIG_V3_BLOCK_ID) ?? unreachable;
    expect_eq(std.mem.eql(u8, v3, "V3_SIG_PAYLOAD_XYZ"), true);

    expect_eq(findApkSigPair(block, APK_SIG_V3_1_BLOCK_ID) == null, true);
    expect_eq(findApkSigPair(block, APK_DEPENDENCY_INFO_BLOCK_ID) == null, true);

    const summary = summarizeApkSigning(apk) ?? unreachable;
    expect_eq(summary.has_v2, true);
    expect_eq(summary.has_v3, true);
    expect_eq(summary.has_v3_1, false);
    expect_eq(summary.has_verity_padding, true);
    expect_eq(summary.pair_count, 3);
});

test("3. Mismatched top/bottom size_of_block and corrupted magic are strictly rejected", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    const meta = buildFullSignedApk(buf[0..], "");

    // Tamper top size_of_block -> mismatch with bottom
    let bad_top: [512]u8 = buf;
    bad_top[meta.sig_block_offset] = bad_top[meta.sig_block_offset] ^ 0x01;
    expect_eq(findApkSigningBlock(bad_top[0..meta.total_len]) == null, true);
    expect_eq(summarizeApkSigning(bad_top[0..meta.total_len]) == null, true);

    // Tamper "APK Sig Block 42" magic
    let bad_magic: [512]u8 = buf;
    bad_magic[meta.cd_offset - 1] = '3';
    expect_eq(findApkSigningBlock(bad_magic[0..meta.total_len]) == null, true);

    // Underflow boundary check: bottom size > cd_offset - 8
    let huge_size: [512]u8 = buf;
    writeU64LE(huge_size[0..], meta.cd_offset - 24, 0xFFFFFFFFFFFF);
    expect_eq(findApkSigningBlock(huge_size[0..meta.total_len]) == null, true);
});

test("4. Plain unsigned ZIP without APK Signing Block returns EOCD but null signing block", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let pos: usize = 0;

    // Local file (30 + 5 = 35B)
    writeU32LE(buf[0..], pos, 0x04034B50);
    writeU16LE(buf[0..], pos + 26, 5);
    copyBytes(buf[0..], pos + 30, "a.txt");
    pos += 35;

    // Central Directory (46 + 5 = 51B)
    const cd_offset = pos;
    writeU32LE(buf[0..], pos, 0x02014B50);
    writeU16LE(buf[0..], pos + 28, 5);
    copyBytes(buf[0..], pos + 46, "a.txt");
    pos += 51;

    // EOCD
    writeU32LE(buf[0..], pos, ZIP_EOCD_SIG);
    writeU16LE(buf[0..], pos + 10, 1);
    writeU32LE(buf[0..], pos + 12, 51);
    writeU32LE(buf[0..], pos + 16, cd_offset as u32);
    writeU16LE(buf[0..], pos + 20, 0);
    pos += ZIP_EOCD_MIN_SIZE;

    const eocd = findZipEocd(buf[0..pos]) ?? unreachable;
    expect_eq(eocd.cd_offset, cd_offset as u32);
    expect_eq(findApkSigningBlock(buf[0..pos]) == null, true);
    expect_eq(summarizeApkSigning(buf[0..pos]) == null, true);
});

test("5. Out-of-bounds pair length and truncated/non-ZIP inputs fail safely", () => {
    // Non-ZIP / empty / too short
    expect_eq(findZipEocd("") == null, true);
    expect_eq(findZipEocd("not a zip archive at all!") == null, true);
    expect_eq(findApkSigningBlock("short") == null, true);

    // Build valid APK then corrupt first pair_len to exceed pairs_slice
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    const meta = buildFullSignedApk(buf[0..], "");
    const first_pair_offset = meta.sig_block_offset + 8;

    // Case A: pair_len overflow past pairs_slice
    let oob: [512]u8 = buf;
    writeU64LE(oob[0..], first_pair_offset, 999999);
    const blk_oob = findApkSigningBlock(oob[0..meta.total_len]) ?? unreachable;
    let it_oob = ApkSigPairIter.init(blk_oob);
    expect_eq(it_oob.next() == null, true);
    expect_eq(it_oob.malformed, true);
    expect_eq(summarizeApkSigning(oob[0..meta.total_len]) == null, true);

    // Case B: pair_len < 4 (invalid, cannot hold 4B id)
    let too_small: [512]u8 = buf;
    writeU64LE(too_small[0..], first_pair_offset, 2);
    expect_eq(summarizeApkSigning(too_small[0..meta.total_len]) == null, true);
});
