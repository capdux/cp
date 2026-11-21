// Source: cp/cap/test/test_lucene_production_deep.cp (Production deep tests for cap:lucene)

import { test, expect_eq } from "cap:test";
import {
    LUCENE_CODEC_MAGIC,
    LUCENE_FOOTER_MAGIC,
    LUCENE_FOOTER_LENGTH,
    LUCENE_ID_LENGTH,
    LuceneVInt,
    LuceneVLong,
    LuceneString,
    LuceneCodecHeader,
    LuceneIndexHeader,
    LuceneFooter,
    readLuceneVInt,
    readLuceneVLong,
    luceneZigZagDecode32,
    luceneZigZagDecode64,
    readLuceneString,
    parseLuceneHeader,
    parseLuceneIndexHeader,
    parseLuceneFooter,
    isLuceneFile,
} from "cap:lucene";

fn writeU32BE(dst: []u8, off: usize, val: u32): usize {
    dst[off + 0] = ((val >> 24) & 0xFF) as u8;
    dst[off + 1] = ((val >> 16) & 0xFF) as u8;
    dst[off + 2] = ((val >> 8) & 0xFF) as u8;
    dst[off + 3] = (val & 0xFF) as u8;
    return off + 4;
}

fn writeU64BE(dst: []u8, off: usize, val: u64): usize {
    let p = writeU32BE(dst, off, ((val >> 32) & 0xFFFFFFFF) as u32);
    p = writeU32BE(dst, p, (val & 0xFFFFFFFF) as u32);
    return p;
}

fn writeBytes(dst: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
    return off + src.len;
}

fn writeLuceneStr(dst: []u8, off: usize, s: []const u8): usize {
    dst[off] = s.len as u8; // < 128 single-byte VInt
    return writeBytes(dst, off + 1, s);
}

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. parses Lucene CodecHeader and IndexHeader with 16B object_id and ASCII suffix", () => {
    let raw: [128]u8 = undefined;
    let buf = raw[0..128];

    let p: usize = 0;
    p = writeU32BE(buf, p, LUCENE_CODEC_MAGIC);
    p = writeLuceneStr(buf, p, "Lucene90SegmentInfo");
    p = writeU32BE(buf, p, 9); // version = 9
    const codec_end = p;

    // 16B object_id
    let i: usize = 0;
    while (i < 16) {
        buf[p + i] = (0xA0 + i) as u8;
        i += 1;
    }
    p += 16;

    // 1B suffix_len + suffix
    const suffix = "si";
    buf[p] = suffix.len as u8;
    p = writeBytes(buf, p + 1, suffix);
    const index_end = p;

    // Body + Footer
    p = writeBytes(buf, p, "PAYLOAD");
    p = writeU32BE(buf, p, LUCENE_FOOTER_MAGIC);
    p = writeU32BE(buf, p, 0); // algorithm_id = 0
    p = writeU64BE(buf, p, 0x0000000012345678);

    const file = buf[0..p];
    expect_eq(LUCENE_FOOTER_LENGTH, 16);
    expect_eq(LUCENE_ID_LENGTH, 16);
    expect_eq(isLuceneFile(file), true);

    if (ch := parseLuceneHeader(file)) {
        expect_eq(sliceEq(ch.codec_name, "Lucene90SegmentInfo"), true);
        expect_eq(ch.version, 9);
        expect_eq(ch.body_offset, codec_end);
    } else { expect_eq(true, false); }

    if (ih := parseLuceneIndexHeader(file)) {
        expect_eq(sliceEq(ih.codec_name, "Lucene90SegmentInfo"), true);
        expect_eq(ih.version, 9);
        expect_eq(ih.object_id[0], 0xA0);
        expect_eq(ih.object_id[15], 0xAF);
        expect_eq(sliceEq(ih.object_suffix, "si"), true);
        expect_eq(ih.body_offset, index_end);
    } else { expect_eq(true, false); }
});

test("2. parses LuceneFooter and verifies CRC32 checksum extraction", () => {
    let raw: [32]u8 = undefined;
    let buf = raw[0..32];
    let p: usize = 16;
    p = writeU32BE(buf, p, LUCENE_FOOTER_MAGIC);
    p = writeU32BE(buf, p, 0);
    p = writeU64BE(buf, p, 0x00000000CAFEBABE);

    if (ft := parseLuceneFooter(buf[0..32])) {
        expect_eq(ft.algorithm_id, 0);
        expect_eq(ft.checksum, 0x00000000CAFEBABE);
        expect_eq(ft.crc32, 0xCAFEBABE);
    } else {
        expect_eq(true, false);
    }
});

test("3. decodes Lucene VInt, VLong, String, and ZigZag 32/64 boundary values", () => {
    // VInt: 0, 127, 128, u32::MAX (5 bytes)
    const v0: [1]u8 = [0x00];
    if (r := readLuceneVInt(v0[0..1], 0)) { expect_eq(r.value, 0); } else { expect_eq(true, false); }

    const v128: [2]u8 = [0x80, 0x01];
    if (r := readLuceneVInt(v128[0..2], 0)) {
        expect_eq(r.value, 128);
        expect_eq(r.next_pos, 2);
    } else { expect_eq(true, false); }

    const vmax32: [5]u8 = [0xFF, 0xFF, 0xFF, 0xFF, 0x0F];
    if (r := readLuceneVInt(vmax32[0..5], 0)) {
        expect_eq(r.value, 0xFFFFFFFF);
        expect_eq(r.next_pos, 5);
    } else { expect_eq(true, false); }

    // VLong: max u63 (0x7FFFFFFFFFFFFFFF) in 9 bytes
    const vmax63: [9]u8 = [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F];
    if (r := readLuceneVLong(vmax63[0..9], 0)) {
        expect_eq(r.value, 0x7FFFFFFFFFFFFFFF);
        expect_eq(r.next_pos, 9);
    } else { expect_eq(true, false); }

    // Reject negative / 10-byte VLong (9th byte has MSB 0x80 set)
    const bad_vlong: [10]u8 = [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x01];
    expect_eq(readLuceneVLong(bad_vlong[0..10], 0) == null, true);

    // Reject overflowing 5th byte in VInt (0x10 > 0x0F)
    const bad_vint: [5]u8 = [0xFF, 0xFF, 0xFF, 0xFF, 0x10];
    expect_eq(readLuceneVInt(bad_vint[0..5], 0) == null, true);

    // ZigZag 32 & 64
    expect_eq(luceneZigZagDecode32(0), 0);
    expect_eq(luceneZigZagDecode32(1), -1);
    expect_eq(luceneZigZagDecode32(2), 1);
    expect_eq(luceneZigZagDecode32(0xFFFFFFFF), -2147483648);

    expect_eq(luceneZigZagDecode64(0), 0);
    expect_eq(luceneZigZagDecode64(1), -1);
    expect_eq(luceneZigZagDecode64(2), 1);
    expect_eq(luceneZigZagDecode64(0xFFFFFFFFFFFFFFFF), -9223372036854775807 - 1);

    // String
    const sbuf: [6]u8 = [5, 'c', 'o', 'd', 'e', 'c'];
    if (ls := readLuceneString(sbuf[0..6], 0)) {
        expect_eq(sliceEq(ls.text, "codec"), true);
        expect_eq(ls.next_pos, 6);
    } else { expect_eq(true, false); }
});

test("4. validates complete .cfs style minimal header+footer framing with isLuceneFile", () => {
    let raw: [64]u8 = undefined;
    let buf = raw[0..64];
    let p: usize = 0;
    p = writeU32BE(buf, p, LUCENE_CODEC_MAGIC);
    p = writeLuceneStr(buf, p, "Lucene90CompoundEntries");
    p = writeU32BE(buf, p, 0);
    // Immediately followed by 16B footer (0-byte body)
    p = writeU32BE(buf, p, LUCENE_FOOTER_MAGIC);
    p = writeU32BE(buf, p, 0);
    p = writeU64BE(buf, p, 0x00000000ABCDEF01);

    expect_eq(isLuceneFile(buf[0..p]), true);
});

test("5. rejects bad magic, nonzero algorithm_id, nonzero high-32 checksum, and truncated frames", () => {
    // Bad header magic
    let raw: [64]u8 = undefined;
    let buf = raw[0..64];
    let p = writeU32BE(buf, 0, 0x11223344);
    p = writeLuceneStr(buf, p, "Bad");
    p = writeU32BE(buf, p, 1);
    p = writeU32BE(buf, p, LUCENE_FOOTER_MAGIC);
    p = writeU32BE(buf, p, 0);
    p = writeU64BE(buf, p, 123);
    expect_eq(parseLuceneHeader(buf[0..p]) == null, true);
    expect_eq(isLuceneFile(buf[0..p]), false);

    // Nonzero algorithm_id in footer
    * = writeU32BE(buf, 0, LUCENE_CODEC_MAGIC);
    * = writeU32BE(buf, p - 12, 1); // algorithm_id = 1
    expect_eq(parseLuceneFooter(buf[0..p]) == null, true);
    expect_eq(isLuceneFile(buf[0..p]), false);

    // High 32 bits of checksum nonzero
    * = writeU32BE(buf, p - 12, 0);
    * = writeU64BE(buf, p - 8, 0x0000000100000000);
    expect_eq(parseLuceneFooter(buf[0..p]) == null, true);
    expect_eq(isLuceneFile(buf[0..p]), false);

    // Truncated buffer
    expect_eq(parseLuceneHeader(buf[0..3]) == null, true);
    expect_eq(parseLuceneFooter(buf[0..10]) == null, true);
});
