// Source: cp/cap/test/test_ogg_production_deep.cp (Production deep tests for cap:ogg)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    OGG_FLAG_CONTINUATION,
    OGG_FLAG_BOS,
    OGG_FLAG_EOS,
    OggPage,
    oggCrc32,
    isOgg,
    OggPageIter,
} from "cap:media";

fn putU32LE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putU64LE(buf: *[]u8, off: usize, v: u64) {
    putU32LE(buf, off, (v & 0xFFFFFFFF) as u32);
    putU32LE(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn writeOggPage(
    dst: *[]u8,
    off: usize,
    header_type: u8,
    granule: u64,
    serial: u32,
    seq: u32,
    payload: []const u8,
): usize {
    dst[off] = 'O';
    dst[off + 1] = 'g';
    dst[off + 2] = 'g';
    dst[off + 3] = 'S';
    dst[off + 4] = 0; // version 0
    dst[off + 5] = header_type;
    putU64LE(dst, off + 6, granule);
    putU32LE(dst, off + 14, serial);
    putU32LE(dst, off + 18, seq);
    putU32LE(dst, off + 22, 0); // checksum placeholder

    // Single lacing segment (payload.len <= 255)
    dst[off + 26] = 1;
    dst[off + 27] = payload.len as u8;

    let i: usize = 0;
    while (i < payload.len) {
        dst[off + 28 + i] = payload[i];
        i += 1;
    }

    const page_len: usize = payload.len + 28;
    const crc = oggCrc32(dst[off .. off + page_len]);
    putU32LE(dst, off + 22, crc);
    return page_len;
}

test("1. isOgg validates OggS magic and version 0 while rejecting bad version or non-Ogg data", () => {
    const ok_hdr: [5]u8 = ['O', 'g', 'g', 'S', 0];
    const bad_ver: [5]u8 = ['O', 'g', 'g', 'S', 1];
    const not_ogg: [5]u8 = ['f', 'L', 'a', 'C', 0];
    const short_buf: [4]u8 = ['O', 'g', 'g', 'S'];

    expect_eq(isOgg(ok_hdr[0..5]), true);
    expect_eq(isOgg(bad_ver[0..5]), false);
    expect_eq(isOgg(not_ogg[0..5]), false);
    expect_eq(isOgg(short_buf[0..4]), false);
});

test("2. OggPageIter parses multi-page BOS -> Continuation -> EOS stream with valid CRC-32", () => {
    let stream: [160]u8 = undefined;
    let s = stream[0..160];

    const p0_len = writeOggPage(*s, 0, OGG_FLAG_BOS, 0, 0x11223344, 0, "OpusHead");
    const p1_len = writeOggPage(*s, p0_len, OGG_FLAG_CONTINUATION, 48000, 0x11223344, 1, "OpusTags!");
    const p2_len = writeOggPage(*s, p0_len + p1_len, OGG_FLAG_EOS, 96000, 0x11223344, 2, "END");
    const total = p0_len + p1_len + p2_len;

    let it = OggPageIter.init(s[0..total]);

    const pg0 = it.next() ?? unreachable;
    expect_eq(pg0.version, 0);
    expect_eq(pg0.isBos(), true);
    expect_eq(pg0.isContinuation(), false);
    expect_eq(pg0.isEos(), false);
    expect_eq(pg0.granule_position, 0);
    expect_eq(pg0.serial_number, 0x11223344);
    expect_eq(pg0.page_sequence, 0);
    expect_eq(pg0.page_segments, 1);
    expect_eq(std.mem.eql(u8, pg0.payload, "OpusHead"), true);
    expect_eq(pg0.verifyChecksum(), true);

    const pg1 = it.next() ?? unreachable;
    expect_eq(pg1.isBos(), false);
    expect_eq(pg1.isContinuation(), true);
    expect_eq(pg1.isEos(), false);
    expect_eq(pg1.granule_position, 48000);
    expect_eq(pg1.page_sequence, 1);
    expect_eq(std.mem.eql(u8, pg1.payload, "OpusTags!"), true);
    expect_eq(pg1.verifyChecksum(), true);

    const pg2 = it.next() ?? unreachable;
    expect_eq(pg2.isBos(), false);
    expect_eq(pg2.isContinuation(), false);
    expect_eq(pg2.isEos(), true);
    expect_eq(pg2.granule_position, 96000);
    expect_eq(pg2.page_sequence, 2);
    expect_eq(std.mem.eql(u8, pg2.payload, "END"), true);
    expect_eq(pg2.verifyChecksum(), true);

    expect_eq(it.next() == null, true);
});

test("3. oggCrc32 and verifyChecksum detect single-bit payload tampering and zero out bytes 22..26", () => {
    let page_buf: [64]u8 = undefined;
    let s = page_buf[0..64];
    const len = writeOggPage(*s, 0, OGG_FLAG_BOS | OGG_FLAG_EOS, 123456, 42, 0, "hello-ogg-crc");

    let it_ok = OggPageIter.init(s[0..len]);
    const pg_ok = it_ok.next() ?? unreachable;
    expect_eq(pg_ok.verifyChecksum(), true);

    // Computing oggCrc32 directly on raw_page (with non-zero checksum field at 22..26) matches stored checksum
    expect_eq(oggCrc32(pg_ok.raw_page), pg_ok.checksum);

    // Flip 1 bit in payload -> verifyChecksum must fail
    s[28] ^= 0x01;
    let it_bad = OggPageIter.init(s[0..len]);
    const pg_bad = it_bad.next() ?? unreachable;
    expect_eq(pg_bad.verifyChecksum(), false);
});

test("4. OggPageIter sums multi-segment lacing table (e.g. 255 + 10 = 265 bytes)", () => {
    // 27 header + 2 segment entries + 265 payload = 294 bytes
    let raw: [294]u8 = undefined;
    let i: usize = 0;
    while (i < 294) {
        raw[i] = 0x5A;
        i += 1;
    }
    let s = raw[0..294];
    s[0] = 'O';
    s[1] = 'g';
    s[2] = 'g';
    s[3] = 'S';
    s[4] = 0;
    s[5] = OGG_FLAG_BOS;
    putU64LE(*s, 6, 0x0000000200000001);
    putU32LE(*s, 14, 99);
    putU32LE(*s, 18, 7);
    putU32LE(*s, 22, 0);
    s[26] = 2;   // 2 segments
    s[27] = 255; // seg 0
    s[28] = 10;  // seg 1 -> total payload = 265
    const crc = oggCrc32(s);
    putU32LE(*s, 22, crc);

    let it = OggPageIter.init(s);
    const pg = it.next() ?? unreachable;
    expect_eq(pg.page_segments, 2);
    expect_eq(pg.segment_table.len, 2);
    expect_eq(pg.payload.len, 265);
    expect_eq(pg.granule_position, 0x0000000200000001);
    expect_eq(pg.verifyChecksum(), true);
    expect_eq(it.next() == null, true);
});

test("5. OggPageIter safely rejects truncated headers, truncated segment tables, and truncated payloads", () => {
    let page_buf: [64]u8 = undefined;
    let s = page_buf[0..64];
    const len = writeOggPage(*s, 0, OGG_FLAG_BOS, 0, 1, 0, "1234567890");

    // Truncated header (< 27 bytes)
    let it1 = OggPageIter.init(s[0..20]);
    expect_eq(it1.next() == null, true);

    // Truncated before segment table ends (27 bytes while page_segments == 1)
    let it2 = OggPageIter.init(s[0..27]);
    expect_eq(it2.next() == null, true);

    // Truncated payload (missing last byte)
    let it3 = OggPageIter.init(s[0 .. len - 1]);
    expect_eq(it3.next() == null, true);
});
