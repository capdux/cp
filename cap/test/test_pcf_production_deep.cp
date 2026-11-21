// Source: cp/cap/test/test_pcf_production_deep.cp
// Deep Production Test Suite for cap:pcf
// Covers:
//   1. Standard PCF TOC multi-table iteration & findPcfTable lookup
//   2. Compressed 5B (0x80 bias) & uncompressed 12B LE/BE PcfMetrics decoding
//   3. PCF_BDF_ENCODINGS 1D (ASCII) & 2D (multi-byte) O(1) glyph lookup + 0xFFFF missing sentinel
//   4. PCF_PROPERTIES string & integer property iteration with 4-byte padding & Big-Endian table
//   5. Bad "\x01fcp" magic, out-of-bounds TOC offset/size & truncated table payload guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    PCF_MAGIC,
    PCF_TOC_ENTRY_SIZE,
    PCF_PROPERTIES,
    PCF_ACCELERATORS,
    PCF_METRICS,
    PCF_BITMAPS,
    PCF_INK_METRICS,
    PCF_BDF_ENCODINGS,
    PCF_SWIDTHS,
    PCF_GLYPH_NAMES,
    PCF_BDF_ACCELERATORS,
    PCF_DEFAULT_FORMAT,
    PCF_COMPRESSED_METRICS,
    PCF_BYTE_MASK,
    PCF_BIT_MASK,
    PCF_GLYPH_PAD_MASK,
    PcfTocEntry,
    PcfMetrics,
    PcfEncodingsHeader,
    PcfProperty,
    PcfTocIter,
    PcfPropIter,
    isPcf,
    findPcfTable,
    parsePcfMetricsCount,
    readPcfMetric,
    parsePcfEncodings,
    lookupPcfGlyphIndex,
} from "cap:pcf";

struct PcfBuilder {
    buf: [1024]u8 = [0; 1024],
    len: usize = 0,

    fn writeBytes(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.len] = s[i];
            _.len += 1;
            i += 1;
        }
    }

    fn writeU8(*_, v: u8) {
        _.buf[_.len] = v;
        _.len += 1;
    }

    fn writeU16Le(*_, v: u16) {
        _.writeU8((v & 0xFF) as u8);
        _.writeU8(((v >> 8) & 0xFF) as u8);
    }

    fn writeU16Be(*_, v: u16) {
        _.writeU8(((v >> 8) & 0xFF) as u8);
        _.writeU8((v & 0xFF) as u8);
    }

    fn writeU32Le(*_, v: u32) {
        _.writeU8((v & 0xFF) as u8);
        _.writeU8(((v >> 8) & 0xFF) as u8);
        _.writeU8(((v >> 16) & 0xFF) as u8);
        _.writeU8(((v >> 24) & 0xFF) as u8);
    }

    fn writeU32Be(*_, v: u32) {
        _.writeU8(((v >> 24) & 0xFF) as u8);
        _.writeU8(((v >> 16) & 0xFF) as u8);
        _.writeU8(((v >> 8) & 0xFF) as u8);
        _.writeU8((v & 0xFF) as u8);
    }

    fn patchU32Le(*_, off: usize, v: u32) {
        _.buf[off] = (v & 0xFF) as u8;
        _.buf[off + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[off + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[off + 3] = ((v >> 24) & 0xFF) as u8;
    }

    fn slice(): []const u8 {
        return _.buf[0.._.len];
    }
}

fn testTocMultiTableAndLookup() {
    let b = PcfBuilder {};
    b.writeBytes(PCF_MAGIC);
    b.writeU32Le(2); // 2 tables: PCF_METRICS + PCF_BDF_ENCODINGS

    // TOC entry 0: PCF_METRICS (compressed)
    const e0_off = b.len;
    b.writeU32Le(PCF_METRICS);
    b.writeU32Le(PCF_COMPRESSED_METRICS);
    b.writeU32Le(0); // size placeholder
    b.writeU32Le(0); // offset placeholder

    // TOC entry 1: PCF_BDF_ENCODINGS
    const e1_off = b.len;
    b.writeU32Le(PCF_BDF_ENCODINGS);
    b.writeU32Le(PCF_DEFAULT_FORMAT);
    b.writeU32Le(0);
    b.writeU32Le(0);

    // Table 0 payload: compressed metrics (1 glyph)
    const t0_start = b.len;
    b.writeU32Le(PCF_COMPRESSED_METRICS);
    b.writeU16Le(1);
    b.writeU8(128 + 1); // lsb = +1
    b.writeU8(128 + 7); // rsb = +7
    b.writeU8(128 + 8); // width = +8
    b.writeU8(128 + 10); // ascent = +10
    b.writeU8(128 - 2); // descent = -2
    const t0_size = b.len - t0_start;
    b.patchU32Le(e0_off + 8, t0_size as u32);
    b.patchU32Le(e0_off + 12, t0_start as u32);

    // Table 1 payload: encodings [65..66]
    const t1_start = b.len;
    b.writeU32Le(PCF_DEFAULT_FORMAT);
    b.writeU16Le(65); // min_char
    b.writeU16Le(66); // max_char
    b.writeU16Le(0);  // min_byte1
    b.writeU16Le(0);  // max_byte1
    b.writeU16Le(65); // default_char
    b.writeU16Le(0);  // 'A' -> glyph 0
    b.writeU16Le(0xFFFF); // 'B' -> missing
    const t1_size = b.len - t1_start;
    b.patchU32Le(e1_off + 8, t1_size as u32);
    b.patchU32Le(e1_off + 12, t1_start as u32);

    const data = b.slice();
    expect_eq(isPcf(data), true);

    let it = PcfTocIter.init(data);
    const te0 = it.next().?;
    expect_eq(te0.table_type, PCF_METRICS);
    expect_eq(te0.payload.len, t0_size);

    const te1 = it.next().?;
    expect_eq(te1.table_type, PCF_BDF_ENCODINGS);
    expect_eq(te1.payload.len, t1_size);
    expect_eq(it.next() == null, true);

    // Lookup via findPcfTable
    const met = findPcfTable(data, PCF_METRICS).?;
    expect_eq(parsePcfMetricsCount(met.payload).?, 1);
    expect_eq(findPcfTable(data, PCF_BITMAPS) == null, true);
}

fn testCompressedAndUncompressedMetricsLeBe() {
    // 1. Compressed 5B metrics (Big-Endian count)
    let c = PcfBuilder {};
    c.writeU32Le(PCF_COMPRESSED_METRICS | PCF_BYTE_MASK);
    c.writeU16Be(2);
    // Glyph 0: lsb=-3, rsb=9, width=8, ascent=12, descent=3
    c.writeU8(125);
    c.writeU8(137);
    c.writeU8(136);
    c.writeU8(140);
    c.writeU8(131);
    // Glyph 1: lsb=0, rsb=5, width=6, ascent=7, descent=0
    c.writeU8(128);
    c.writeU8(133);
    c.writeU8(134);
    c.writeU8(135);
    c.writeU8(128);

    expect_eq(parsePcfMetricsCount(c.slice()).?, 2);
    const m0 = readPcfMetric(c.slice(), 0).?;
    expect_eq(m0.left_side_bearing, -3);
    expect_eq(m0.right_side_bearing, 9);
    expect_eq(m0.character_width, 8);
    expect_eq(m0.character_ascent, 12);
    expect_eq(m0.character_descent, 3);
    expect_eq(m0.character_attributes, 0);

    const m1 = readPcfMetric(c.slice(), 1).?;
    expect_eq(m1.character_width, 6);
    expect_eq(readPcfMetric(c.slice(), 2) == null, true);

    // 2. Uncompressed 12B metrics (Big-Endian)
    let u = PcfBuilder {};
    u.writeU32Le(PCF_DEFAULT_FORMAT | PCF_BYTE_MASK);
    u.writeU32Be(1);
    const neg_lsb: i16 = -10;
    const neg_bits: u16 = @bitCast(neg_lsb);
    u.writeU16Be(neg_bits); // lsb = -10
    u.writeU16Be(20);       // rsb = 20
    u.writeU16Be(16);       // width = 16
    u.writeU16Be(14);       // ascent = 14
    u.writeU16Be(4);        // descent = 4
    u.writeU16Be(0x0042);   // attributes = 0x42

    const um = readPcfMetric(u.slice(), 0).?;
    expect_eq(um.left_side_bearing, -10);
    expect_eq(um.right_side_bearing, 20);
    expect_eq(um.character_width, 16);
    expect_eq(um.character_ascent, 14);
    expect_eq(um.character_descent, 4);
    expect_eq(um.character_attributes, 0x0042);
}

fn testEncodings1DAnd2DMatrixLookup() {
    // 2D matrix: byte1 in [0x30..0x31], byte2 in [0x41..0x42] -> 2x2 = 4 entries (Big-Endian)
    let e = PcfBuilder {};
    e.writeU32Le(PCF_DEFAULT_FORMAT | PCF_BYTE_MASK);
    e.writeU16Be(0x41);   // min_char_or_byte2
    e.writeU16Be(0x42);   // max_char_or_byte2
    e.writeU16Be(0x30);   // min_byte1
    e.writeU16Be(0x31);   // max_byte1
    e.writeU16Be(0x3041); // default_char
    // Row 0 (0x30): 0x3041 -> 10, 0x3042 -> 0xFFFF (missing)
    e.writeU16Be(10);
    e.writeU16Be(0xFFFF);
    // Row 1 (0x31): 0x3141 -> 20, 0x3142 -> 21
    e.writeU16Be(20);
    e.writeU16Be(21);

    const hdr = parsePcfEncodings(e.slice()).?;
    expect_eq(hdr.min_byte1, 0x30);
    expect_eq(hdr.max_byte1, 0x31);
    expect_eq(hdr.default_char, 0x3041);

    expect_eq(lookupPcfGlyphIndex(e.slice(), 0x3041).?, 10);
    expect_eq(lookupPcfGlyphIndex(e.slice(), 0x3042) == null, true); // 0xFFFF missing
    expect_eq(lookupPcfGlyphIndex(e.slice(), 0x3141).?, 20);
    expect_eq(lookupPcfGlyphIndex(e.slice(), 0x3142).?, 21);
    expect_eq(lookupPcfGlyphIndex(e.slice(), 0x2041) == null, true); // byte1 OOB
    expect_eq(lookupPcfGlyphIndex(e.slice(), 0x3040) == null, true); // byte2 OOB
    expect_eq(lookupPcfGlyphIndex(e.slice(), 0x10000) == null, true); // > 16-bit
}

fn testPropertiesTableStringAndInt() {
    // 2 properties in Big-Endian format:
    //   Prop 0: "FAMILY_NAME" -> string "Fixed"
    //   Prop 1: "PIXEL_SIZE"  -> int 14
    // String pool: "FAMILY_NAME\0Fixed\0PIXEL_SIZE\0"
    //   off 0:  "FAMILY_NAME" (len 11 + NUL = 12)
    //   off 12: "Fixed"       (len 5 + NUL = 6)
    //   off 18: "PIXEL_SIZE"  (len 10 + NUL = 11)
    //   total string_size = 29
    // Props section size = 8 + 2 * 9 = 26 -> pad to 4B = 2 bytes -> str_size at offset 28
    let p = PcfBuilder {};
    p.writeU32Le(PCF_DEFAULT_FORMAT | PCF_BYTE_MASK);
    p.writeU32Be(2); // nprops = 2

    // Prop 0 (9 bytes)
    p.writeU32Be(0);  // name_off = 0 ("FAMILY_NAME")
    p.writeU8(1);     // is_string = true
    p.writeU32Be(12); // val_off = 12 ("Fixed")

    // Prop 1 (9 bytes)
    p.writeU32Be(18); // name_off = 18 ("PIXEL_SIZE")
    p.writeU8(0);     // is_string = false
    p.writeU32Be(14); // int_value = 14

    // 2 bytes padding to align 26 -> 28
    p.writeU8(0);
    p.writeU8(0);

    // string_size + string pool
    const pool = "FAMILY_NAME\x00Fixed\x00PIXEL_SIZE\x00";
    p.writeU32Be(pool.len as u32);
    p.writeBytes(pool);

    let it = PcfPropIter.init(p.slice());
    const pr0 = it.next().?;
    expect_eq(std.mem.eql(u8, pr0.name, "FAMILY_NAME"), true);
    expect_eq(pr0.is_string, true);
    expect_eq(std.mem.eql(u8, pr0.str_value, "Fixed"), true);

    const pr1 = it.next().?;
    expect_eq(std.mem.eql(u8, pr1.name, "PIXEL_SIZE"), true);
    expect_eq(pr1.is_string, false);
    expect_eq(pr1.int_value, 14);

    expect_eq(it.next() == null, true);
}

fn testMalformedPcfGuards() {
    // Bad magic & truncated header
    expect_eq(isPcf(""), false);
    expect_eq(isPcf("\x01fcp"), false);
    expect_eq(isPcf("NOPE\x00\x00\x00\x00"), false);

    // TOC claims 100 tables in 8-byte buffer
    let bad_cnt = PcfBuilder {};
    bad_cnt.writeBytes(PCF_MAGIC);
    bad_cnt.writeU32Le(100);
    expect_eq(isPcf(bad_cnt.slice()), false);

    // TOC entry with out-of-bounds offset/size
    let bad_off = PcfBuilder {};
    bad_off.writeBytes(PCF_MAGIC);
    bad_off.writeU32Le(1);
    bad_off.writeU32Le(PCF_METRICS);
    bad_off.writeU32Le(0);
    bad_off.writeU32Le(9999); // size OOB
    bad_off.writeU32Le(24);
    expect_eq(isPcf(bad_off.slice()), true);
    let oob_it = PcfTocIter.init(bad_off.slice());
    expect_eq(oob_it.next() == null, true);

    // Truncated metrics, encodings, and properties payloads
    expect_eq(parsePcfMetricsCount("\x00\x00\x00") == null, true);
    expect_eq(parsePcfEncodings("\x00\x00\x00\x00") == null, true);
    let bad_pit = PcfPropIter.init("\x00\x00\x00\x00\x05\x00\x00\x00");
    expect_eq(bad_pit.next() == null, true);
}

test("1. Standard PCF TOC Multi-Table Iteration & findPcfTable Lookup", () => {
    testTocMultiTableAndLookup();
});

test("2. Compressed 5B (0x80 Bias) & Uncompressed 12B LE/BE PcfMetrics Decoding", () => {
    testCompressedAndUncompressedMetricsLeBe();
});

test("3. PCF_BDF_ENCODINGS 1D/2D O(1) Glyph Lookup & 0xFFFF Missing Sentinel", () => {
    testEncodings1DAnd2DMatrixLookup();
});

test("4. PCF_PROPERTIES String/Int Iteration with 4B Padding & Big-Endian Format", () => {
    testPropertiesTableStringAndInt();
});

test("5. Bad Magic, Out-of-Bounds TOC Offsets & Truncated Table Guards", () => {
    testMalformedPcfGuards();
});
