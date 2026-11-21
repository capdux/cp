// Source: cp/cap/test/test_s3m_production_deep.cp (Production deep tests for cap:s3m)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    S3M_MAGIC,
    S3M_HEADER_SIZE,
    S3M_SUB_BYTE,
    S3M_TYPE_MODULE,
    S3M_INST_SIZE,
    S3M_ORDER_SKIP,
    S3M_ORDER_END,
    S3mHeader,
    S3mInstrument,
    S3mPattern,
    isS3m,
    parseS3mHeader,
    parseS3mInstrument,
    parseS3mPattern,
} from "cap:s3m";

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

fn copyBytes(dst: []u8, offset: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[offset + i] = src[i];
        i += 1;
    }
}

/// 构造完整合成 S3M 模块（512B）：
/// - 0..96: 96B Header (4 orders, 2 instruments, 1 pattern, 8 active channels)
/// - 96..100: Orders [0, 254, 0, 255]
/// - 100..104: Instrument Parapointers [pp=8 (0x80), pp=13 (0xD0)]
/// - 104..106: Pattern Parapointer [pp=20 (0x140)]
/// - 0x80..0xD0: Instrument 0 (80B PCM with 24-bit memseg -> 0x180 & "SCRS")
/// - 0xD0..0x120: Instrument 1 (80B AdLib type=2)
/// - 0x140..0x150: Pattern 0 (packed_len = 10, 8B payload)
fn buildSampleS3m(buf: []u8) {
    copyBytes(buf, 0, "Second Reality");
    buf[28] = S3M_SUB_BYTE;
    buf[29] = S3M_TYPE_MODULE;
    writeU16LE(buf, 32, 4); // order_count
    writeU16LE(buf, 34, 2); // instrument_count
    writeU16LE(buf, 36, 1); // pattern_count
    writeU16LE(buf, 38, 0x0040); // flags
    writeU16LE(buf, 40, 0x1320); // ST3 3.20
    writeU16LE(buf, 42, 2); // unsigned samples
    copyBytes(buf, 44, S3M_MAGIC);
    buf[48] = 64; // global_volume
    buf[49] = 6; // initial_speed
    buf[50] = 125; // initial_tempo
    buf[51] = 0x80 | 48; // stereo + master_vol 48

    // Enable first 8 channels, disable remaining 24 (0xFF)
    let c: usize = 0;
    while (c < 32) {
        buf[64 + c] = if (c < 8) (c as u8) else 0xFF;
        c += 1;
    }

    // Orders at 96..100
    buf[96] = 0;
    buf[97] = S3M_ORDER_SKIP;
    buf[98] = 0;
    buf[99] = S3M_ORDER_END;

    // Instrument parapointers at 100..104: 0x80 / 16 = 8, 0xD0 / 16 = 13
    writeU16LE(buf, 100, 8);
    writeU16LE(buf, 102, 13);

    // Pattern parapointer at 104..106: 0x140 / 16 = 20
    writeU16LE(buf, 104, 20);

    // Instrument 0 at 0x80 (PCM)
    const inst0_off: usize = 0x80;
    buf[inst0_off] = 1; // PCM sample
    copyBytes(buf, inst0_off + 1, "BASS.SMP");
    // 24-bit memseg: hi=0x01, lo=0x0018 => 0x010018 * 16 = 0x100180
    buf[inst0_off + 13] = 0x01;
    writeU16LE(buf, inst0_off + 14, 0x0018);
    writeU32LE(buf, inst0_off + 16, 4096); // length
    writeU32LE(buf, inst0_off + 20, 512); // loop_begin
    writeU32LE(buf, inst0_off + 24, 2048); // loop_end
    buf[inst0_off + 28] = 60; // volume
    buf[inst0_off + 31] = 0x01 | 0x04; // loop + 16-bit
    writeU32LE(buf, inst0_off + 32, 8363); // c2_speed
    copyBytes(buf, inst0_off + 48, "Synth Bass 16bit");
    copyBytes(buf, inst0_off + 76, "SCRS");

    // Instrument 1 at 0xD0 (AdLib)
    const inst1_off: usize = 0xD0;
    buf[inst1_off] = 2;
    copyBytes(buf, inst1_off + 1, "OPL.SBI");
    copyBytes(buf, inst1_off + 48, "AdLib Lead");
    copyBytes(buf, inst1_off + 76, "SCRI");

    // Pattern 0 at 0x140
    const p0: usize = 0x140;
    writeU16LE(buf, p0, 10); // 2B len + 8B payload
    copyBytes(buf, p0 + 2, "\x20\x44\x01\x00\x00\x00\x00\x00");
}

test("1. isS3m & parseS3mHeader decode 96B header, stereo/signed flags, and active channel count", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    buildSampleS3m(buf[0..]);

    expect_eq(isS3m(buf[0..]), true);
    const hdr = parseS3mHeader(buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, hdr.song_name, "Second Reality"), true);
    expect_eq(hdr.order_count, 4);
    expect_eq(hdr.instrument_count, 2);
    expect_eq(hdr.pattern_count, 1);
    expect_eq(hdr.global_volume, 64);
    expect_eq(hdr.initial_speed, 6);
    expect_eq(hdr.initial_tempo, 125);
    expect_eq(hdr.isStereo(), true);
    expect_eq(hdr.masterVolLevel(), 48);
    expect_eq(hdr.isSignedSamples(), false);
    expect_eq(hdr.activeChannelCount(), 8);
});

test("2. Order slice and Parapointer (pp * 16) addressing for instruments and patterns", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    buildSampleS3m(buf[0..]);

    const hdr = parseS3mHeader(buf[0..]) ?? unreachable;
    const orders = hdr.ordersSlice(buf[0..]);
    expect_eq(orders.len, 4);
    expect_eq(orders[0], 0);
    expect_eq(orders[1], S3M_ORDER_SKIP);
    expect_eq(orders[3], S3M_ORDER_END);

    expect_eq(hdr.instrumentOffset(buf[0..], 0) ?? 0, 0x80);
    expect_eq(hdr.instrumentOffset(buf[0..], 1) ?? 0, 0xD0);
    expect_eq(hdr.instrumentOffset(buf[0..], 2) == null, true);

    expect_eq(hdr.patternOffset(buf[0..], 0) ?? 0, 0x140);
    expect_eq(hdr.patternOffset(buf[0..], 1) == null, true);
});

test("3. parseS3mInstrument decodes 80B PCM 24-bit memseg, flags, SCRS signature, and AdLib entry", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    buildSampleS3m(buf[0..]);

    const hdr = parseS3mHeader(buf[0..]) ?? unreachable;
    const inst0 = parseS3mInstrument(buf[0..], hdr, 0) ?? unreachable;
    expect_eq(inst0.inst_type, 1);
    expect_eq(std.mem.eql(u8, inst0.dos_filename, "BASS.SMP"), true);
    expect_eq(std.mem.eql(u8, inst0.title, "Synth Bass 16bit"), true);
    expect_eq(inst0.sample_data_offset, 0x100180);
    expect_eq(inst0.length, 4096);
    expect_eq(inst0.loop_begin, 512);
    expect_eq(inst0.loop_end, 2048);
    expect_eq(inst0.volume, 60);
    expect_eq(inst0.c2_speed, 8363);
    expect_eq(inst0.hasLoop(), true);
    expect_eq(inst0.isStereoSample(), false);
    expect_eq(inst0.is16BitSample(), true);
    expect_eq(inst0.has_scrs_sig, true);

    const inst1 = parseS3mInstrument(buf[0..], hdr, 1) ?? unreachable;
    expect_eq(inst1.inst_type, 2);
    expect_eq(std.mem.eql(u8, inst1.title, "AdLib Lead"), true);
    expect_eq(inst1.has_scrs_sig, false);
});

test("4. parseS3mPattern extracts zero-copy packed pattern payload slice", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    buildSampleS3m(buf[0..]);

    const hdr = parseS3mHeader(buf[0..]) ?? unreachable;
    const pat = parseS3mPattern(buf[0..], hdr, 0) ?? unreachable;
    expect_eq(pat.offset, 0x140);
    expect_eq(pat.packed_length, 10);
    expect_eq(pat.data.len, 8);
    expect_eq(pat.data[0], 0x20);
    expect_eq(pat.data[1], 0x44);
    expect_eq(pat.data[2], 0x01);
});

test("5. Malformed S3M headers (bad SCRM/0x1A/type, truncated tables, out-of-bounds parapointer) fail safely", () => {
    expect_eq(isS3m(""), false);
    expect_eq(parseS3mHeader("short") == null, true);

    let buf: [512]u8 = std.mem.zeroes([512]u8);
    buildSampleS3m(buf[0..]);

    // Wrong 0x1A byte
    let bad_sub: [512]u8 = buf;
    bad_sub[28] = 0x00;
    expect_eq(isS3m(bad_sub[0..]), false);

    // Wrong SCRM magic
    let bad_magic: [512]u8 = buf;
    bad_magic[44] = 'X';
    expect_eq(parseS3mHeader(bad_magic[0..]) == null, true);

    // Truncated before tables end (needs 106B)
    expect_eq(parseS3mHeader(buf[0..105]) == null, true);

    // Out-of-bounds instrument parapointer (pp = 0x0100 => 4096 > 512)
    let oob_pp: [512]u8 = buf;
    writeU16LE(oob_pp[0..], 100, 0x0100);
    const hdr = parseS3mHeader(oob_pp[0..]) ?? unreachable;
    expect_eq(parseS3mInstrument(oob_pp[0..], hdr, 0) == null, true);

    // Out-of-bounds pattern packed length
    let oob_pat: [512]u8 = buf;
    writeU16LE(oob_pat[0..], 0x140, 1000);
    const hdr2 = parseS3mHeader(oob_pat[0..]) ?? unreachable;
    expect_eq(parseS3mPattern(oob_pat[0..], hdr2, 0) == null, true);
});
