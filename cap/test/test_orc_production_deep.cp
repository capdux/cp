// Source: cp/cap/test/test_orc_production_deep.cp (Production deep tests for cap:orc)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ORC_MAGIC,
    OrcCompression,
    OrcPostScript,
    OrcLayout,
    OrcCompressionBlock,
    VarintResult,
    orcCompressionFromU64,
    readVarint,
    skipField,
    parseOrcPostScript,
    isOrc,
    parseOrcLayout,
    OrcBlockIter,
} from "cap:binfmt";

fn putVarint(dst: []u8, off: usize, value: u64): usize {
    let v = value;
    let p = off;
    while (true) {
        let byte: u8 = (v & 0x7F) as u8;
        v = v >> 7;
        if (v != 0) {
            dst[p] = byte | 0x80;
            p += 1;
        } else {
            dst[p] = byte;
            p += 1;
            break;
        }
    }
    return p - off;
}

fn copyBytes(dst: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
    return src.len;
}

fn buildSampleOrc(
    dst: []u8,
    stripe_payload: []const u8,
    meta_payload: []const u8,
    footer_payload: []const u8,
    compression_id: u64,
    magic_str: []const u8,
): usize {
    let p: usize = 0;
    p += copyBytes(dst, p, "ORC");
    p += copyBytes(dst, p, stripe_payload);
    p += copyBytes(dst, p, meta_payload);
    p += copyBytes(dst, p, footer_payload);

    const ps_start = p;
    // field 1: footerLength (tag = (1 << 3) | 0 = 8)
    p += putVarint(dst, p, 8);
    p += putVarint(dst, p, footer_payload.len as u64);
    // field 2: compression (tag = 16)
    p += putVarint(dst, p, 16);
    p += putVarint(dst, p, compression_id);
    // field 3: compressionBlockSize = 262144 (tag = 24)
    p += putVarint(dst, p, 24);
    p += putVarint(dst, p, 262144);
    // field 4: packed repeated version = [0, 12] (tag = (4 << 3) | 2 = 34)
    p += putVarint(dst, p, 34);
    p += putVarint(dst, p, 2); // 2 bytes packed payload
    p += putVarint(dst, p, 0);
    p += putVarint(dst, p, 12);
    // field 5: metadataLength (tag = 40)
    p += putVarint(dst, p, 40);
    p += putVarint(dst, p, meta_payload.len as u64);
    // field 6: writerVersion = 6 (tag = 48)
    p += putVarint(dst, p, 48);
    p += putVarint(dst, p, 6);
    // field 8000: magic (tag = (8000 << 3) | 2 = 64002)
    p += putVarint(dst, p, 64002);
    p += putVarint(dst, p, magic_str.len as u64);
    p += copyBytes(dst, p, magic_str);

    const ps_len = p - ps_start;
    dst[p] = ps_len as u8;
    return p + 1;
}

test("1. parseOrcPostScript decodes protobuf fields, packed version [0,12], compression, and magic", () => {
    let buf: [256]u8 = undefined;
    const total = buildSampleOrc(buf[0..256], "STRIPE_DATA", "META", "FOOTER_BYTES", 5, ORC_MAGIC);

    expect_eq(isOrc(buf[0..total]), true);
    const ps = parseOrcPostScript(buf[0..total]) ?? unreachable;
    expect_eq(ps.footer_length, 12);
    expect_eq(ps.compression == OrcCompression.zstd, true);
    expect_eq(ps.compression_block_size, 262144);
    expect_eq(ps.version_major, 0);
    expect_eq(ps.version_minor, 12);
    expect_eq(ps.metadata_length, 4);
    expect_eq(ps.writer_version, 6);
    expect_eq(std.mem.eql(u8, ps.magic, "ORC"), true);
});

test("2. parseOrcLayout computes exact zero-copy slices for stripes, metadata, and footer", () => {
    let buf: [256]u8 = undefined;
    const total = buildSampleOrc(buf[0..256], "STRIPE_01", "META_02", "FOOTER_03", 1, ORC_MAGIC);

    const layout = parseOrcLayout(buf[0..total]) ?? unreachable;
    expect_eq(layout.postscript.compression == OrcCompression.zlib, true);
    expect_eq(std.mem.eql(u8, layout.stripes_slice, "STRIPE_01"), true);
    expect_eq(std.mem.eql(u8, layout.metadata_slice, "META_02"), true);
    expect_eq(std.mem.eql(u8, layout.footer_slice, "FOOTER_03"), true);
    expect_eq(layout.metadata_offset, 3 + 9);
    expect_eq(layout.footer_offset, 3 + 9 + 7);
});

test("3. OrcBlockIter decodes 3-byte little-endian headers for original and compressed chunks", () => {
    // Block 0: original=true, length=5 -> hdr = (5 << 1) | 1 = 11 (0x0B, 0x00, 0x00)
    // Block 1: original=false, length=4 -> hdr = (4 << 1) | 0 = 8 (0x08, 0x00, 0x00)
    const stream: [15]u8 = [
        0x0B, 0x00, 0x00, 'h', 'e', 'l', 'l', 'o',
        0x08, 0x00, 0x00, 0x78, 0x9C, 0x01, 0x02,
    ];

    let it = OrcBlockIter.init(stream[0..15]);
    const b0 = it.next() ?? unreachable;
    expect_eq(b0.is_original, true);
    expect_eq(b0.length, 5);
    expect_eq(std.mem.eql(u8, b0.payload, "hello"), true);

    const b1 = it.next() ?? unreachable;
    expect_eq(b1.is_original, false);
    expect_eq(b1.length, 4);
    expect_eq(b1.payload[0], 0x78);
    expect_eq(b1.payload[3], 0x02);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);

    // Truncated block payload sets had_error
    let it_bad = OrcBlockIter.init(stream[0..13]);
    expect_eq(it_bad.next() != null, true);
    expect_eq(it_bad.next() == null, true);
    expect_eq(it_bad.had_error, true);
});

test("4. orcCompressionFromU64 maps all 6 ORC compression codecs and unknown fallback", () => {
    expect_eq(orcCompressionFromU64(0) == OrcCompression.none, true);
    expect_eq(orcCompressionFromU64(1) == OrcCompression.zlib, true);
    expect_eq(orcCompressionFromU64(2) == OrcCompression.snappy, true);
    expect_eq(orcCompressionFromU64(3) == OrcCompression.lzo, true);
    expect_eq(orcCompressionFromU64(4) == OrcCompression.lz4, true);
    expect_eq(orcCompressionFromU64(5) == OrcCompression.zstd, true);
    expect_eq(orcCompressionFromU64(99) == OrcCompression.unknown, true);
});

test("5. parseOrcPostScript and parseOrcLayout guard against wrong magic, truncation, and underflowing footer/metadata lengths", () => {
    // Wrong magic in PostScript
    let bad_magic: [256]u8 = undefined;
    const m_len = buildSampleOrc(bad_magic[0..256], "S", "M", "F", 0, "BAD");
    expect_eq(isOrc(bad_magic[0..m_len]), false);
    expect_eq(parseOrcPostScript(bad_magic[0..m_len]) == null, true);

    // Build a valid PostScript declaring footer_length = 99999 exceeding file size
    let oob: [64]u8 = undefined;
    let p: usize = 0;
    p += copyBytes(oob[0..64], p, "ORC");
    const ps_start = p;
    p += putVarint(oob[0..64], p, 8);
    p += putVarint(oob[0..64], p, 99999);
    p += putVarint(oob[0..64], p, 64002);
    p += putVarint(oob[0..64], p, 3);
    p += copyBytes(oob[0..64], p, "ORC");
    oob[p] = (p - ps_start) as u8;
    const oob_len = p + 1;

    expect_eq(parseOrcPostScript(oob[0..oob_len]) != null, true);
    expect_eq(parseOrcLayout(oob[0..oob_len]) == null, true);

    // Truncated buffer
    expect_eq(parseOrcPostScript("ORC") == null, true);
});
