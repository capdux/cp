// Source: cp/cap/test/test_luks_production_deep.cp (Production deep tests for cap:luks)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LUKS_MAGIC,
    LUKS2_MAGIC_SECONDARY,
    LUKS_V1,
    LUKS_V2,
    LUKS1_HEADER_SIZE,
    LUKS1_NUM_KEYSLOTS,
    LUKS1_KEYSLOT_SIZE,
    LUKS1_KEY_ENABLED,
    LUKS1_KEY_DISABLED,
    LUKS2_BIN_HEADER_SIZE,
    LUKS2_MIN_HDR_SIZE,
    peekLuksVersion,
    isLuks,
    Luks1Keyslot,
    Luks1Header,
    parseLuks1Header,
    Luks2Header,
    parseLuks2Header,
    selectActiveLuks2,
} from "cap:luks";

fn putU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn putU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn putU64Be(buf: []u8, off: usize, v: u64) {
    putU32Be(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    putU32Be(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn initLuks1(buf: []u8) {
    copyBytes(buf, 0, LUKS_MAGIC[0..]);
    putU16Be(buf, 6, LUKS_V1);
    copyBytes(buf, 8, "aes");
    copyBytes(buf, 40, "xts-plain64");
    copyBytes(buf, 72, "sha256");
    putU32Be(buf, 104, 4096); // payload_offset (sectors)
    putU32Be(buf, 108, 64); // key_bytes (512-bit XTS)
    buf[112] = 0xAA;
    buf[132] = 0xBB;
    putU32Be(buf, 164, 150000); // mk_digest_iter
    copyBytes(buf, 168, "12345678-1234-5678-9abc-def012345678");

    // Initialize all 8 keyslots as disabled first
    let i: usize = 0;
    while (i < LUKS1_NUM_KEYSLOTS) {
        putU32Be(buf, 208 + i * LUKS1_KEYSLOT_SIZE, LUKS1_KEY_DISABLED);
        i += 1;
    }
    // Enable slot 0 and slot 3
    putU32Be(buf, 208 + 0 * 48, LUKS1_KEY_ENABLED);
    putU32Be(buf, 208 + 0 * 48 + 4, 500000);
    buf[208 + 0 * 48 + 8] = 0x42;
    putU32Be(buf, 208 + 0 * 48 + 40, 8);
    putU32Be(buf, 208 + 0 * 48 + 44, 4000);

    putU32Be(buf, 208 + 3 * 48, LUKS1_KEY_ENABLED);
    putU32Be(buf, 208 + 3 * 48 + 4, 600000);
    putU32Be(buf, 208 + 3 * 48 + 40, 520);
    putU32Be(buf, 208 + 3 * 48 + 44, 4000);
}

fn initLuks2(buf: []u8, secondary: bool, hdr_size: u64, seqid: u64, hdr_offset: u64) {
    if (secondary) {
        copyBytes(buf, 0, LUKS2_MAGIC_SECONDARY[0..]);
    } else {
        copyBytes(buf, 0, LUKS_MAGIC[0..]);
    }
    putU16Be(buf, 6, LUKS_V2);
    putU64Be(buf, 8, hdr_size);
    putU64Be(buf, 16, seqid);
    copyBytes(buf, 24, "ROOT_CRYPT");
    copyBytes(buf, 72, "sha256");
    buf[104] = 0x77;
    copyBytes(buf, 168, "abcdef00-1111-2222-3333-444455556666");
    copyBytes(buf, 208, "systemd-cryptsetup");
    putU64Be(buf, 256, hdr_offset);
    buf[448] = 0x99;
}

test("1. LUKS1 592B header parsing, cipher/mode/hash/UUID slices & 8-keyslot active counting", () => {
    expect_eq(LUKS1_HEADER_SIZE, 592 as usize);
    expect_eq(LUKS1_NUM_KEYSLOTS, 8 as usize);
    expect_eq(LUKS1_KEYSLOT_SIZE, 48 as usize);
    expect_eq(LUKS2_BIN_HEADER_SIZE, 4096 as usize);
    expect_eq(LUKS2_MIN_HDR_SIZE, 16384 as u64);

    let buf: [592]u8 = std.mem.zeroes([592]u8);
    initLuks1(buf[0..]);

    expect_eq(isLuks(buf[0..]), true);
    const ver = peekLuksVersion(buf[0..]) ?? unreachable;
    expect_eq(ver, LUKS_V1);

    const h = parseLuks1Header(buf[0..]) ?? unreachable;
    expect_eq(h.version, LUKS_V1);
    expect_eq(eqStr(h.cipherNameSlice(), "aes"), true);
    expect_eq(eqStr(h.cipherModeSlice(), "xts-plain64"), true);
    expect_eq(eqStr(h.hashSpecSlice(), "sha256"), true);
    expect_eq(eqStr(h.uuidSlice(), "12345678-1234-5678-9abc-def012345678"), true);
    expect_eq(h.payload_offset, 4096 as u32);
    expect_eq(h.key_bytes, 64 as u32);
    expect_eq(h.mk_digest[0], 0xAA as u8);
    expect_eq(h.mk_digest_salt[0], 0xBB as u8);
    expect_eq(h.mk_digest_iter, 150000 as u32);

    expect_eq(h.activeKeyslotCount(), 2 as usize);
    expect_eq(h.keyslots[0].isEnabled(), true);
    expect_eq(h.keyslots[0].isDisabled(), false);
    expect_eq(h.keyslots[0].iterations, 500000 as u32);
    expect_eq(h.keyslots[0].salt[0], 0x42 as u8);
    expect_eq(h.keyslots[0].key_material_offset, 8 as u32);
    expect_eq(h.keyslots[0].stripes, 4000 as u32);

    expect_eq(h.keyslots[1].isEnabled(), false);
    expect_eq(h.keyslots[1].isDisabled(), true);
    expect_eq(h.keyslots[3].isEnabled(), true);
    expect_eq(h.keyslots[3].key_material_offset, 520 as u32);
});

test("2. LUKS2 4096B primary & secondary (SKUL\\xBA\\xBE) dual parsing + selectActiveLuks2", () => {
    let pri: [4096]u8 = std.mem.zeroes([4096]u8);
    let sec: [4096]u8 = std.mem.zeroes([4096]u8);
    initLuks2(pri[0..], false, 16384, 5, 0);
    initLuks2(sec[0..], true, 16384, 7, 16384);

    expect_eq(isLuks(pri[0..]), true);
    expect_eq(isLuks(sec[0..]), true);
    expect_eq(peekLuksVersion(pri[0..]) ?? unreachable, LUKS_V2);
    expect_eq(peekLuksVersion(sec[0..]) ?? unreachable, LUKS_V2);

    const h1 = parseLuks2Header(pri[0..]) ?? unreachable;
    const h2 = parseLuks2Header(sec[0..]) ?? unreachable;

    expect_eq(h1.is_secondary, false);
    expect_eq(h1.version, LUKS_V2);
    expect_eq(h1.hdr_size, 16384 as u64);
    expect_eq(h1.seqid, 5 as u64);
    expect_eq(h1.hdr_offset, 0 as u64);
    expect_eq(h1.jsonAreaBytes(), 12288 as u64);
    expect_eq(eqStr(h1.labelSlice(), "ROOT_CRYPT"), true);
    expect_eq(eqStr(h1.csumAlgSlice(), "sha256"), true);
    expect_eq(eqStr(h1.uuidSlice(), "abcdef00-1111-2222-3333-444455556666"), true);
    expect_eq(eqStr(h1.subsystemSlice(), "systemd-cryptsetup"), true);
    expect_eq(h1.salt[0], 0x77 as u8);
    expect_eq(h1.csum[0], 0x99 as u8);

    expect_eq(h2.is_secondary, true);
    expect_eq(h2.seqid, 7 as u64);
    expect_eq(h2.hdr_offset, 16384 as u64);

    // Secondary has higher seqid (7 > 5) -> secondary wins
    const winner = selectActiveLuks2(h1, h2) ?? unreachable;
    expect_eq(winner.is_secondary, true);
    expect_eq(winner.seqid, 7 as u64);

    // One-sided & both-null cases
    const only1 = selectActiveLuks2(h1, null) ?? unreachable;
    expect_eq(only1.seqid, 5 as u64);
    const only2 = selectActiveLuks2(null, h2) ?? unreachable;
    expect_eq(only2.seqid, 7 as u64);
    expect_eq(selectActiveLuks2(null, null) == null, true);
});

test("3. LUKS2 JSON metadata area [4096..hdr_size] zero-copy extraction & NUL stripping", () => {
    let full: [16384]u8 = std.mem.zeroes([16384]u8);
    initLuks2(full[0..4096], false, 16384, 10, 0);

    const json_str = "{\"keyslots\":{\"0\":{\"type\":\"luks2\",\"kdf\":{\"type\":\"argon2id\"}}},\"segments\":{\"0\":{\"type\":\"crypt\"}}}";
    copyBytes(full[0..], 4096, json_str);

    const h = parseLuks2Header(full[0..]) ?? unreachable;
    const extracted = h.extractJson(full[0..]) ?? unreachable;
    expect_eq(extracted.len, json_str.len);
    expect_eq(eqStr(extracted, json_str), true);

    // Truncated full_hdr (< hdr_size) must return null
    expect_eq(h.extractJson(full[0..8192]) == null, true);
});

test("4. Malformed LUKS1 rejection (bad magic, version mismatch, key_bytes == 0, truncated)", () => {
    // Truncated (< 592)
    let short_buf: [512]u8 = std.mem.zeroes([512]u8);
    copyBytes(short_buf[0..], 0, LUKS_MAGIC[0..]);
    putU16Be(short_buf[0..], 6, LUKS_V1);
    expect_eq(parseLuks1Header(short_buf[0..]) == null, true);

    // Bad magic
    let bad: [592]u8 = std.mem.zeroes([592]u8);
    initLuks1(bad[0..]);
    bad[5] = 0x00;
    expect_eq(isLuks(bad[0..]), false);
    expect_eq(parseLuks1Header(bad[0..]) == null, true);

    // Secondary magic is invalid for LUKS1
    initLuks1(bad[0..]);
    copyBytes(bad[0..], 0, LUKS2_MAGIC_SECONDARY[0..]);
    expect_eq(peekLuksVersion(bad[0..]) == null, true);
    expect_eq(parseLuks1Header(bad[0..]) == null, true);

    // Version mismatch (v2 passed to parseLuks1Header)
    initLuks1(bad[0..]);
    putU16Be(bad[0..], 6, LUKS_V2);
    expect_eq(parseLuks1Header(bad[0..]) == null, true);

    // key_bytes == 0 rejected
    initLuks1(bad[0..]);
    putU32Be(bad[0..], 108, 0);
    expect_eq(parseLuks1Header(bad[0..]) == null, true);
});

test("5. Malformed LUKS2 rejection (bad magic, version != 2, hdr_size < 16384 or non-power-of-2, truncated)", () => {
    // Truncated (< 4096)
    let short_buf: [1024]u8 = std.mem.zeroes([1024]u8);
    copyBytes(short_buf[0..], 0, LUKS_MAGIC[0..]);
    putU16Be(short_buf[0..], 6, LUKS_V2);
    expect_eq(parseLuks2Header(short_buf[0..]) == null, true);

    // Bad magic
    let bad: [4096]u8 = std.mem.zeroes([4096]u8);
    initLuks2(bad[0..], false, 16384, 1, 0);
    bad[0] = 0x00;
    expect_eq(parseLuks2Header(bad[0..]) == null, true);

    // Version != 2 (e.g. 1 or 3)
    initLuks2(bad[0..], false, 16384, 1, 0);
    putU16Be(bad[0..], 6, LUKS_V1);
    expect_eq(parseLuks2Header(bad[0..]) == null, true);
    putU16Be(bad[0..], 6, 3);
    expect_eq(peekLuksVersion(bad[0..]) == null, true);
    expect_eq(parseLuks2Header(bad[0..]) == null, true);

    // hdr_size < 16384 (e.g. 8192)
    initLuks2(bad[0..], false, 8192, 1, 0);
    expect_eq(parseLuks2Header(bad[0..]) == null, true);

    // hdr_size non-power-of-2 (e.g. 24576)
    initLuks2(bad[0..], false, 24576, 1, 0);
    expect_eq(parseLuks2Header(bad[0..]) == null, true);

    // Valid larger power-of-2 hdr_size (65536 = 64 KiB) accepted
    initLuks2(bad[0..], true, 65536, 2, 65536);
    const h = parseLuks2Header(bad[0..]) ?? unreachable;
    expect_eq(h.jsonAreaBytes(), 61440 as u64);
});
