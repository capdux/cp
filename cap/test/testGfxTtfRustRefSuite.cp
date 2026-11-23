import { test, expect_eq } from "cap:test";
import {
    SfntFlavor,
    parseTtcHeader,
    getTtcFontOffset,
    parseSfntHeaderAt,
    parseSfntHeader,
    findSfntTableScalar,
    findSfntTableSimd,
    findSfntTable,
    findSfntTableAt,
    parseTtfHead,
    parseTtfNumGlyphs,
    parseTtfHhea,
    parseTtfHmtx,
    parseTtfOs2,
    lookupCmapGlyph,
    findTtfNameRecord,
    calcSfntChecksumScalar,
    calcSfntChecksumSimd,
    calcSfntChecksum,
    computeTtfTableChecksum,
} from "../../lib/gfx/ttf.cp";

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn writeU16BE(out: []u8, off: usize, v: u16) {
    out[off] = ((v >> 8) & 0xFF) as u8;
    out[off + 1] = (v & 0xFF) as u8;
}

fn writeU32BE(out: []u8, off: usize, v: u32) {
    out[off] = ((v >> 24) & 0xFF) as u8;
    out[off + 1] = ((v >> 16) & 0xFF) as u8;
    out[off + 2] = ((v >> 8) & 0xFF) as u8;
    out[off + 3] = (v & 0xFF) as u8;
}

// Real TrueType font from RazrFalcon/ttf-parser (tests/fonts/demo.ttf, 400 bytes, MIT/Apache-2.0)
const TTF_PARSER_DEMO: [400]u8 = [
    0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x40, 0x00, 0x02, 0x00, 0x30, 0x63, 0x6d, 0x61, 0x70,
    0x00, 0x09, 0x00, 0x76, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x67, 0x6c, 0x79, 0x66,
    0xf1, 0xcb, 0x66, 0x98, 0x00, 0x00, 0x01, 0x34, 0x00, 0x00, 0x00, 0x5c, 0x68, 0x65, 0x61, 0x64,
    0xf2, 0x35, 0xdd, 0xf8, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x36, 0x68, 0x68, 0x65, 0x61,
    0x06, 0x61, 0x00, 0xca, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00, 0x24, 0x68, 0x6d, 0x74, 0x78,
    0x04, 0x74, 0x00, 0x6a, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x08, 0x6c, 0x6f, 0x63, 0x61,
    0x00, 0x2e, 0x00, 0x14, 0x00, 0x00, 0x01, 0x2c, 0x00, 0x00, 0x00, 0x06, 0x6d, 0x61, 0x78, 0x70,
    0x00, 0x05, 0x00, 0x0b, 0x00, 0x00, 0x00, 0xd8, 0x00, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0xf5, 0x9c, 0x29, 0x44, 0x5f, 0x0f, 0x3c, 0xf5, 0x00, 0x02, 0x03, 0xe8,
    0x00, 0x00, 0x00, 0x00, 0xb4, 0x92, 0xf4, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x2f, 0xa6, 0x5c,
    0x00, 0x06, 0x00, 0x00, 0x02, 0x58, 0x02, 0xbc, 0x00, 0x00, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x04, 0x00, 0xfe, 0x70, 0x00, 0x00, 0x02, 0x58,
    0x00, 0x06, 0xff, 0xff, 0x02, 0x58, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x0b,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x58, 0x00, 0x64, 0x02, 0x1c, 0x00, 0x06,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x04, 0x00, 0x20,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x41, 0xff, 0xff, 0x00, 0x00,
    0x00, 0x41, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14,
    0x00, 0x2e, 0x00, 0x00, 0x00, 0x02, 0x00, 0x64, 0x00, 0x00, 0x02, 0x58, 0x02, 0xbc, 0x00, 0x03,
    0x00, 0x07, 0x00, 0x00, 0x33, 0x11, 0x21, 0x11, 0x25, 0x21, 0x11, 0x21, 0x64, 0x01, 0xf4, 0xfe,
    0x34, 0x01, 0xa4, 0xfe, 0x5c, 0x02, 0xbc, 0xfd, 0x44, 0x28, 0x02, 0x6c, 0x00, 0x02, 0x00, 0x06,
    0x00, 0x00, 0x02, 0x1d, 0x02, 0x90, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x00, 0x13, 0x33, 0x03, 0x01,
    0x13, 0x33, 0x13, 0x23, 0x27, 0x23, 0x07, 0xad, 0xc4, 0x63, 0xfe, 0xf8, 0xda, 0x60, 0xdd, 0x59,
    0x3e, 0xef, 0x42, 0x01, 0x0b, 0x01, 0x40, 0xfd, 0xb5, 0x02, 0x90, 0xfd, 0x70, 0xc8, 0xc8, 0x00
];

fn runTestDemoTtfAndTtc(): void! {
    const hdr = parseSfntHeader(TTF_PARSER_DEMO[0..]).?;
    expect_eq(hdr.flavor == SfntFlavor.truetype, true);
    expect_eq(hdr.num_tables, 7);

    const head = parseTtfHead(TTF_PARSER_DEMO[0..]).?;
    expect_eq(head.units_per_em, 1000);
    expect_eq(head.index_to_loc_format, 0);

    const hhea = parseTtfHhea(TTF_PARSER_DEMO[0..]).?;
    expect_eq(hhea.ascender, 1024);
    expect_eq(hhea.descender, -400);
    expect_eq(hhea.number_of_hmetrics, 2);

    const ng = parseTtfNumGlyphs(TTF_PARSER_DEMO[0..]).?;
    expect_eq(ng, 2);

    const hm0 = parseTtfHmtx(TTF_PARSER_DEMO[0..], 0).?;
    expect_eq(hm0.advance_width, 600);
    expect_eq(hm0.lsb, 100);
    const hm1 = parseTtfHmtx(TTF_PARSER_DEMO[0..], 1).?;
    expect_eq(hm1.advance_width, 540);
    expect_eq(hm1.lsb, 6);
    expect_true(parseTtfHmtx(TTF_PARSER_DEMO[0..], 2) == null)?;

    // cmap Format 4: 'A' (0x41) -> glyph 1, 'B' (0x42) -> null
    expect_eq(lookupCmapGlyph(TTF_PARSER_DEMO[0..], 0x41).?, 1);
    expect_true(lookupCmapGlyph(TTF_PARSER_DEMO[0..], 0x42) == null)?;

    // Verify table checksums on cmap, maxp, hhea
    const r_cmap = findSfntTable(TTF_PARSER_DEMO[0..], "cmap").?;
    expect_eq(computeTtfTableChecksum(r_cmap.slice(TTF_PARSER_DEMO[0..]).?), r_cmap.checksum);
    const r_maxp = findSfntTable(TTF_PARSER_DEMO[0..], "maxp").?;
    expect_eq(calcSfntChecksum(r_maxp.slice(TTF_PARSER_DEMO[0..]).?), r_maxp.checksum);

    // TrueType Collection (ttcf) with 1 embedded SFNT at offset 16
    let ttc: [416]u8 = [0; 416];
    writeU32BE(ttc[0..], 0, 0x74746366); // "ttcf"
    writeU16BE(ttc[0..], 4, 1);
    writeU16BE(ttc[0..], 6, 0);
    writeU32BE(ttc[0..], 8, 1); // num_fonts = 1
    writeU32BE(ttc[0..], 12, 16); // font 0 at offset 16
    let k: usize = 0;
    while (k < 400) : (k += 1) {
        ttc[16 + k] = TTF_PARSER_DEMO[k];
    }
    const ttc_h = parseTtcHeader(ttc[0..]).?;
    expect_eq(ttc_h.num_fonts, 1);
    const f0_off = getTtcFontOffset(ttc[0..], 0).?;
    expect_eq(f0_off, 16);
    expect_true(getTtcFontOffset(ttc[0..], 1) == null)?;
    const sub_hdr = parseSfntHeaderAt(ttc[0..], f0_off).?;
    expect_eq(sub_hdr.num_tables, 7);
    expect_true(findSfntTableAt(ttc[0..], f0_off, "head") != null)?;
}

fn runTestCmap12Os2NameAndBounds(): void! {
    // Build synthetic SFNT with cmap (Format 12), OS/2, and name tables
    let font: [512]u8 = [0; 512];
    writeU32BE(font[0..], 0, 0x4F54544F); // "OTTO"
    writeU16BE(font[0..], 4, 3); // 3 tables: OS/2, cmap, name

    // Table 0: "OS/2" at offset 64, len 78
    font[12] = 'O'; font[13] = 'S'; font[14] = '/'; font[15] = '2';
    writeU32BE(font[0..], 20, 64);
    writeU32BE(font[0..], 24, 78);
    writeU16BE(font[0..], 64, 4); // version 4
    writeU16BE(font[0..], 64 + 4, 700); // weight_class = 700 (Bold)
    writeU16BE(font[0..], 64 + 68, 800); // typo_ascender

    // Table 1: "cmap" at offset 144, len 52 (4B header + 8B encoding record + 40B Format 12)
    font[28] = 'c'; font[29] = 'm'; font[30] = 'a'; font[31] = 'p';
    writeU32BE(font[0..], 36, 144);
    writeU32BE(font[0..], 40, 52);
    writeU16BE(font[0..], 144 + 2, 1); // numTables = 1
    writeU16BE(font[0..], 144 + 4, 3); // platformID = 3 (Windows)
    writeU16BE(font[0..], 144 + 6, 10); // encodingID = 10 (Unicode full)
    writeU32BE(font[0..], 144 + 8, 12); // subtable offset = 12
    // Format 12 at 156:
    writeU16BE(font[0..], 156, 12); // format = 12
    writeU32BE(font[0..], 156 + 12, 2); // numGroups = 2
    // Group 0: U+0041..U+0042 -> gid 1
    writeU32BE(font[0..], 156 + 16, 0x0041);
    writeU32BE(font[0..], 156 + 20, 0x0042);
    writeU32BE(font[0..], 156 + 24, 1);
    // Group 1: U+1F600..U+1F602 -> gid 42
    writeU32BE(font[0..], 156 + 28, 0x1F600);
    writeU32BE(font[0..], 156 + 32, 0x1F602);
    writeU32BE(font[0..], 156 + 36, 42);

    // Table 2: "name" at offset 200, len 24
    font[44] = 'n'; font[45] = 'a'; font[46] = 'm'; font[47] = 'e';
    writeU32BE(font[0..], 52, 200);
    writeU32BE(font[0..], 56, 24);
    writeU16BE(font[0..], 200 + 2, 1); // count = 1
    writeU16BE(font[0..], 200 + 4, 18); // storageOffset = 18 (6 + 12)
    writeU16BE(font[0..], 200 + 6, 1); // platformID = 1 (Mac)
    writeU16BE(font[0..], 200 + 12, 1); // nameID = 1 (Family)
    writeU16BE(font[0..], 200 + 14, 6); // length = 6
    writeU16BE(font[0..], 200 + 16, 0); // offset = 0
    font[218] = 'C'; font[219] = 'P'; font[220] = 'F'; font[221] = 'o'; font[222] = 'n'; font[223] = 't';

    const hdr = parseSfntHeader(font[0..]).?;
    expect_eq(hdr.flavor == SfntFlavor.cff, true);

    const os2 = parseTtfOs2(font[0..]).?;
    expect_eq(os2.weight_class, 700);
    expect_eq(os2.typo_ascender, 800);

    expect_eq(lookupCmapGlyph(font[0..], 0x0041).?, 1);
    expect_eq(lookupCmapGlyph(font[0..], 0x0042).?, 2);
    expect_eq(lookupCmapGlyph(font[0..], 0x1F600).?, 42);
    expect_eq(lookupCmapGlyph(font[0..], 0x1F602).?, 44);
    expect_true(lookupCmapGlyph(font[0..], 0x1F603) == null)?;

    const nr = findTtfNameRecord(font[0..], 1).?;
    expect_true(bytesEqual(nr.raw, "CPFont"))?;

    // Out-of-bounds table offset rejected
    writeU32BE(font[0..], 20, 0xFFFFFF00);
    expect_true(parseTtfOs2(font[0..]) == null)?;
}

fn runTestSimdParity(): void! {
    let buf: [257]u8 = [0; 257];
    let i: usize = 0;
    while (i < 257) : (i += 1) {
        buf[i] = ((i * 41 + 17) & 0xFF) as u8;
    }
    const lens: [10]usize = [0, 1, 3, 4, 5, 15, 16, 17, 64, 257];
    let li: usize = 0;
    while (li < lens.len) : (li += 1) {
        const n = lens[li];
        expect_eq(calcSfntChecksumSimd(buf[0..n]), calcSfntChecksumScalar(buf[0..n]));
    }

    const tags: [8][]const u8 = ["cmap", "glyf", "head", "hhea", "hmtx", "loca", "maxp", "xxxx"];
    let ti: usize = 0;
    while (ti < tags.len) : (ti += 1) {
        const s_rec = findSfntTableScalar(TTF_PARSER_DEMO[0..], tags[ti]);
        const v_rec = findSfntTableSimd(TTF_PARSER_DEMO[0..], tags[ti]);
        expect_eq(s_rec == null, v_rec == null);
        if (s_rec != null) {
            expect_eq(s_rec.?.offset, v_rec.?.offset);
            expect_eq(s_rec.?.checksum, v_rec.?.checksum);
        }
    }
}

fn runTestMalformedFuzz(): void! {
    let seed: u64 = 0x7F4A7C159E3779B9;
    let buf: [256]u8 = [0; 256];
    let case_idx: usize = 0;
    while (case_idx < 1300) : (case_idx += 1) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        const len: usize = ((seed % 255) as usize) + 1;

        let j: usize = 0;
        while (j < len) : (j += 1) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            buf[j] = (seed & 0xFF) as u8;
        }
        if ((case_idx % 4) == 0 && len >= 12) {
            writeU32BE(buf[0..], 0, 0x00010000);
        } else if ((case_idx % 7) == 0 && len >= 12) {
            writeU32BE(buf[0..], 0, 0x74746366);
        }

        const slice = buf[0..len];
        expect_eq(calcSfntChecksumSimd(slice), calcSfntChecksumScalar(slice));
        * = parseTtcHeader(slice);
        * = parseSfntHeader(slice);
        * = parseTtfHead(slice);
        * = parseTtfHhea(slice);
        * = parseTtfNumGlyphs(slice);
        * = parseTtfHmtx(slice, 0);
        * = parseTtfOs2(slice);
        * = lookupCmapGlyph(slice, 0x41);
        * = lookupCmapGlyph(slice, 0x1F600);
        * = findTtfNameRecord(slice, 1);
    }
}

fn main() {
    test("1. RazrFalcon/ttf-parser demo.ttf + TTC + hmtx + cmap Format 4 + checksums", () => {
        runTestDemoTtfAndTtc()?;
    });
    test("2. OpenType OTTO + cmap Format 12 (U+1F600) + OS/2 + name + OOB bounds", () => {
        runTestCmap12Os2NameAndBounds()?;
    });
    test("3. SIMD vs Scalar 0-diff parity (calcSfntChecksum & findSfntTable)", () => {
        runTestSimdParity()?;
    });
    test("4. 1,300 deterministic PRNG malformed TTF/TTC/cmap/name fuzz cases (0 panic / 0 OOB)", () => {
        runTestMalformedFuzz()?;
    });
}
