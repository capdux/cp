// Source: cp/cap/test/test_eot_production_deep.cp
// Deep Production Test Suite for cap:eot
// Covers:
//   1. EOT v1.0 full 82B header + 4 UTF-16LE names + embedded TrueType font_data extraction
//   2. EOT v2.1 with root_string_utf16 domain whitelist parsing & UTF-8 decoding
//   3. EOT v2.2 with root_string_checksum, eudc_code_page, signature & EUDC font data
//   4. MTX/XOR/Subset flags & decodeUtf16LeName (ASCII, CJK BMP, UTF-16 surrogate pair, error guards)
//   5. Bad 0x504C magic, non-zero reserved/padding, unknown version & out-of-bounds font_data_size guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    EOT_MAGIC,
    EOT_FIXED_HEADER_SIZE,
    EOT_VERSION_1,
    EOT_VERSION_2_1,
    EOT_VERSION_2_2,
    EOT_FLAG_COMPRESSED,
    EOT_FLAG_SUBSET,
    EOT_FLAG_XOR_ENCRYPTED,
    EotHeader,
    isEot,
    parseEotHeader,
    decodeUtf16LeName,
} from "cap:eot";

struct EotBuilder {
    buf: [1024]u8 = [0; 1024],
    len: usize = 0,

    fn writeU8(*_, v: u8) {
        _.buf[_.len] = v;
        _.len += 1;
    }

    fn writeU16Le(*_, v: u16) {
        _.writeU8((v & 0xFF) as u8);
        _.writeU8(((v >> 8) & 0xFF) as u8);
    }

    fn writeU32Le(*_, v: u32) {
        _.writeU8((v & 0xFF) as u8);
        _.writeU8(((v >> 8) & 0xFF) as u8);
        _.writeU8(((v >> 16) & 0xFF) as u8);
        _.writeU8(((v >> 24) & 0xFF) as u8);
    }

    fn writeBytes(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.writeU8(s[i]);
            i += 1;
        }
    }

    fn writeZeros(*_, count: usize) {
        let i: usize = 0;
        while (i < count) {
            _.writeU8(0);
            i += 1;
        }
    }

    fn writeAsciiAsUtf16Field(*_, ascii: []const u8) {
        _.writeU16Le((ascii.len * 2) as u16);
        let i: usize = 0;
        while (i < ascii.len) {
            _.writeU8(ascii[i]);
            _.writeU8(0);
            i += 1;
        }
    }

    fn writeFixedHeader(*_, version: u32, flags: u32, weight: u32, italic: u8) {
        _.writeU32Le(0); // eot_size placeholder at 0
        _.writeU32Le(0); // font_data_size placeholder at 4
        _.writeU32Le(version);
        _.writeU32Le(flags);
        _.writeZeros(10); // PANOSE (16..26)
        _.writeU8(1);     // charset (26)
        _.writeU8(italic); // italic (27)
        _.writeU32Le(weight); // weight (28..32)
        _.writeU16Le(0);  // fsType (32..34)
        _.writeU16Le(EOT_MAGIC); // magic (34..36)
        _.writeU32Le(0x00000001); // unicode_range[0]
        _.writeU32Le(0x00000002);
        _.writeU32Le(0x00000003);
        _.writeU32Le(0x00000004);
        _.writeU32Le(0x00000001); // codepage_range[0]
        _.writeU32Le(0x00000000);
        _.writeU32Le(0x12345678); // checksum_adjustment (60..64)
        _.writeZeros(16); // reserved1..4 (64..80)
        _.writeU16Le(0);  // padding1 (80..82)
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

fn testEotV1AndEmbeddedFontData() {
    let b = EotBuilder {};
    b.writeFixedHeader(EOT_VERSION_1, 0, 400, 0);
    expect_eq(b.len, EOT_FIXED_HEADER_SIZE);

    b.writeAsciiAsUtf16Field("Roboto");
    b.writeU16Le(0); // Padding2
    b.writeAsciiAsUtf16Field("Regular");
    b.writeU16Le(0); // Padding3
    b.writeAsciiAsUtf16Field("1.00");
    b.writeU16Le(0); // Padding4
    b.writeAsciiAsUtf16Field("Roboto Regular");

    const font_off = b.len;
    const ttf_sig = "\x00\x01\x00\x00TTF_BODY";
    b.writeBytes(ttf_sig);

    b.patchU32Le(0, b.len as u32);
    b.patchU32Le(4, ttf_sig.len as u32);

    const data = b.slice();
    expect_eq(isEot(data), true);

    const hdr = parseEotHeader(data).?;
    expect_eq(hdr.version, EOT_VERSION_1);
    expect_eq(hdr.weight, 400);
    expect_eq(hdr.italic, 0);
    expect_eq(hdr.magic_number, EOT_MAGIC);
    expect_eq(hdr.unicode_range[0], 1);
    expect_eq(hdr.unicode_range[3], 4);
    expect_eq(hdr.checksum_adjustment, 0x12345678);
    expect_eq(hdr.font_data_offset, font_off);
    expect_eq(std.mem.eql(u8, hdr.font_data, ttf_sig), true);

    let out: [64]u8 = [0; 64];
    const n_fam = decodeUtf16LeName(hdr.family_name_utf16, out[0..]).?;
    expect_eq(std.mem.eql(u8, out[0..n_fam], "Roboto"), true);

    const n_sty = decodeUtf16LeName(hdr.style_name_utf16, out[0..]).?;
    expect_eq(std.mem.eql(u8, out[0..n_sty], "Regular"), true);

    const n_full = decodeUtf16LeName(hdr.full_name_utf16, out[0..]).?;
    expect_eq(std.mem.eql(u8, out[0..n_full], "Roboto Regular"), true);
}

fn testEotV21RootStringWhitelist() {
    let b = EotBuilder {};
    b.writeFixedHeader(EOT_VERSION_2_1, EOT_FLAG_COMPRESSED | EOT_FLAG_SUBSET, 700, 1);
    b.writeAsciiAsUtf16Field("Inter");
    b.writeU16Le(0);
    b.writeAsciiAsUtf16Field("Bold Italic");
    b.writeU16Le(0);
    b.writeAsciiAsUtf16Field("2.1");
    b.writeU16Le(0);
    b.writeAsciiAsUtf16Field("Inter Bold Italic");
    b.writeU16Le(0); // Padding5
    b.writeAsciiAsUtf16Field("https://cp-lang.dev");

    const font_payload = "MTX_COMPRESSED_DATA";
    b.writeBytes(font_payload);
    b.patchU32Le(0, b.len as u32);
    b.patchU32Le(4, font_payload.len as u32);

    const hdr = parseEotHeader(b.slice()).?;
    expect_eq(hdr.version, EOT_VERSION_2_1);
    expect_eq(hdr.isCompressed(), true);
    expect_eq(hdr.isSubset(), true);
    expect_eq(hdr.isXorEncrypted(), false);
    expect_eq(std.mem.eql(u8, hdr.font_data, font_payload), true);

    let out: [64]u8 = [0; 64];
    const n_root = decodeUtf16LeName(hdr.root_string_utf16, out[0..]).?;
    expect_eq(std.mem.eql(u8, out[0..n_root], "https://cp-lang.dev"), true);
}

fn testEotV22SignatureAndEudc() {
    let b = EotBuilder {};
    b.writeFixedHeader(EOT_VERSION_2_2, EOT_FLAG_XOR_ENCRYPTED, 500, 0);
    b.writeAsciiAsUtf16Field("Noto");
    b.writeU16Le(0);
    b.writeAsciiAsUtf16Field("Medium");
    b.writeU16Le(0);
    b.writeAsciiAsUtf16Field("2.2");
    b.writeU16Le(0);
    b.writeAsciiAsUtf16Field("Noto Medium");
    b.writeU16Le(0); // Padding5
    b.writeAsciiAsUtf16Field("example.com");

    // v2.2 fields
    b.writeU32Le(0xAABBCCDD); // root_string_checksum
    b.writeU32Le(1252);       // eudc_code_page
    b.writeU16Le(0);          // Padding6
    b.writeU16Le(4);          // signature_size
    b.writeBytes("SIG!");     // signature
    b.writeU32Le(0x00000001); // eudc_flags
    b.writeU32Le(3);          // eudc_font_size
    b.writeBytes("EUD");      // eudc_font_data

    const font_payload = "XOR_FONT_BYTES";
    b.writeBytes(font_payload);
    b.patchU32Le(0, b.len as u32);
    b.patchU32Le(4, font_payload.len as u32);

    const hdr = parseEotHeader(b.slice()).?;
    expect_eq(hdr.version, EOT_VERSION_2_2);
    expect_eq(hdr.isXorEncrypted(), true);
    expect_eq(hdr.root_string_checksum, 0xAABBCCDD);
    expect_eq(hdr.eudc_code_page, 1252);
    expect_eq(std.mem.eql(u8, hdr.signature, "SIG!"), true);
    expect_eq(hdr.eudc_flags, 1);
    expect_eq(hdr.eudc_font_size, 3);
    expect_eq(std.mem.eql(u8, hdr.eudc_font_data, "EUD"), true);
    expect_eq(std.mem.eql(u8, hdr.font_data, font_payload), true);
}

fn testUtf16LeUnicodeAndSurrogates() {
    // 'A' (0x0041) + '中' (0x4E2D) + U+1F600 (0xD83D 0xDE00)
    const raw: [8]u8 = [0x41, 0x00, 0x2D, 0x4E, 0x3D, 0xD8, 0x00, 0xDE];
    let out: [32]u8 = [0; 32];
    const n = decodeUtf16LeName(raw[0..], out[0..]).?;
    expect_eq(n, 8); // 1 + 3 + 4 bytes UTF-8
    expect_eq(out[0], 'A');
    expect_eq(out[1], 0xE4);
    expect_eq(out[2], 0xB8);
    expect_eq(out[3], 0xAD);
    expect_eq(out[4], 0xF0);
    expect_eq(out[5], 0x9F);
    expect_eq(out[6], 0x98);
    expect_eq(out[7], 0x80);

    // Odd length rejected
    expect_eq(decodeUtf16LeName(raw[0..3], out[0..]) == null, true);
    // Unpaired high surrogate rejected
    const bad_surr: [2]u8 = [0x3D, 0xD8];
    expect_eq(decodeUtf16LeName(bad_surr[0..], out[0..]) == null, true);
    // Small output buffer rejected
    let tiny: [3]u8 = [0; 3];
    expect_eq(decodeUtf16LeName(raw[0..], tiny[0..]) == null, true);
}

fn testMalformedEotGuards() {
    // Too short / bad magic
    expect_eq(isEot(""), false);
    expect_eq(isEot("short"), false);

    // Non-zero Reserved1 (offset 64)
    let b1 = EotBuilder {};
    b1.writeFixedHeader(EOT_VERSION_1, 0, 400, 0);
    b1.patchU32Le(0, b1.len as u32);
    b1.patchU32Le(64, 1); // Reserved1 != 0
    expect_eq(isEot(b1.slice()), true);
    expect_eq(parseEotHeader(b1.slice()) == null, true);

    // Non-zero Padding1 (offset 80)
    let b2 = EotBuilder {};
    b2.writeFixedHeader(EOT_VERSION_1, 0, 400, 0);
    b2.patchU32Le(0, b2.len as u32);
    b2.buf[80] = 0xFF;
    expect_eq(parseEotHeader(b2.slice()) == null, true);

    // Out-of-bounds font_data_size
    let b3 = EotBuilder {};
    b3.writeFixedHeader(EOT_VERSION_1, 0, 400, 0);
    b3.writeAsciiAsUtf16Field("A");
    b3.writeU16Le(0);
    b3.writeAsciiAsUtf16Field("B");
    b3.writeU16Le(0);
    b3.writeAsciiAsUtf16Field("C");
    b3.writeU16Le(0);
    b3.writeAsciiAsUtf16Field("D");
    b3.patchU32Le(0, b3.len as u32);
    b3.patchU32Le(4, 9999); // font_data_size exceeds eot_size
    expect_eq(parseEotHeader(b3.slice()) == null, true);

    // Unsupported version
    let b4 = EotBuilder {};
    b4.writeFixedHeader(0x00030000, 0, 400, 0);
    b4.patchU32Le(0, b4.len as u32);
    expect_eq(isEot(b4.slice()), false);
}

test("1. EOT v1.0 Full Header, 4 UTF-16LE Names & Embedded Font Payload", () => {
    testEotV1AndEmbeddedFontData();
});

test("2. EOT v2.1 Domain Whitelist RootString & Compression/Subset Flags", () => {
    testEotV21RootStringWhitelist();
});

test("3. EOT v2.2 RootString Checksum, Signature & EUDC Fields", () => {
    testEotV22SignatureAndEudc();
});

test("4. decodeUtf16LeName BMP, Surrogate Pairs & Error Guards", () => {
    testUtf16LeUnicodeAndSurrogates();
});

test("5. Bad Magic, Non-Zero Reserved/Padding & Out-of-Bounds FontData Guards", () => {
    testMalformedEotGuards();
});
