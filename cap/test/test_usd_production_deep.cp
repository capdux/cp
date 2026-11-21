// Source: cp/cap/test/test_usd_production_deep.cp
// Deep Production Test Suite for cap:usd
// Covers:
//   1. .usdc v0.8.0 88B Bootstrap + TOC 6 core sections (TOKENS/STRINGS/FIELDS/FIELDSETS/PATHS/SPECS) & findUsdcSection
//   2. .usda 1.0 stage metadata (defaultPrim, upAxis, metersPerUnit, nested customLayerData, BOM, header-only)
//   3. .usdz uncompressed ZIP primary asset detection (.usd/.usdc/.usda) & compressed/non-USD rejection
//   4. .usdc v0.10.0 empty TOC (section_count == 0) boundary handling
//   5. Bad magic, truncated header, out-of-bounds toc_offset / section_count overflow / section payload overflow & unclosed .usda guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    USDC_MAGIC,
    USDC_BOOTSTRAP_SIZE,
    USDC_SECTION_NAME_LEN,
    USDC_SECTION_ENTRY_SIZE,
    USDA_MAGIC,
    UsdFormat,
    UsdcBootstrap,
    UsdcSection,
    UsdaHeader,
    UsdcSectionIter,
    isUsdc,
    isUsda,
    isUsdz,
    detectUsdFormat,
    parseUsdcBootstrap,
    findUsdcSection,
    parseUsdaHeader,
} from "cap:usd";

fn approxEq(a: f64, b: f64): bool {
    return @abs(a - b) < 0.000001;
}

struct CrateBuilder {
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

    fn writeZeros(*_, count: usize) {
        let i: usize = 0;
        while (i < count) {
            _.buf[_.len] = 0;
            _.len += 1;
            i += 1;
        }
    }

    fn writeU16Le(*_, v: u16) {
        _.buf[_.len] = (v & 0xFF) as u8;
        _.buf[_.len + 1] = ((v >> 8) & 0xFF) as u8;
        _.len += 2;
    }

    fn writeU32Le(*_, v: u32) {
        _.buf[_.len] = (v & 0xFF) as u8;
        _.buf[_.len + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.len + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.len + 3] = ((v >> 24) & 0xFF) as u8;
        _.len += 4;
    }

    fn writeU64Le(*_, v: u64) {
        _.writeU32Le((v & 0xFFFFFFFF) as u32);
        _.writeU32Le(((v >> 32) & 0xFFFFFFFF) as u32);
    }

    fn patchU64Le(*_, off: usize, v: u64) {
        let i: usize = 0;
        while (i < 8) {
            _.buf[off + i] = ((v >> ((i * 8) as u6)) & 0xFF) as u8;
            i += 1;
        }
    }

    fn writeSectionEntry(*_, name: []const u8, start: u64, size: u64) {
        _.writeBytes(name);
        if (name.len < USDC_SECTION_NAME_LEN) {
            _.writeZeros(USDC_SECTION_NAME_LEN - name.len);
        }
        _.writeU64Le(start);
        _.writeU64Le(size);
    }

    fn slice(): []const u8 {
        return _.buf[0.._.len];
    }
}

fn testUsdcBootstrapAndSixSections() {
    let b = CrateBuilder {};
    // 88-byte Bootstrap header: PXR-USDC + v0.8.0 + toc_offset placeholder + 64B reserved
    b.writeBytes(USDC_MAGIC);
    b.buf[b.len] = 0;
    b.buf[b.len + 1] = 8;
    b.buf[b.len + 2] = 0;
    b.len += 3;
    b.writeZeros(5);
    b.writeU64Le(0); // toc_offset placeholder at offset 16
    b.writeZeros(64);
    expect_eq(b.len, USDC_BOOTSTRAP_SIZE);

    // Write 6 section payloads
    const s0_off = b.len as u64;
    b.writeBytes("TOK_DATA");
    const s1_off = b.len as u64;
    b.writeBytes("STR_DATA");
    const s2_off = b.len as u64;
    b.writeBytes("FLD_DATA");
    const s3_off = b.len as u64;
    b.writeBytes("FSET_DATA");
    const s4_off = b.len as u64;
    b.writeBytes("PATH_DATA");
    const s5_off = b.len as u64;
    b.writeBytes("SPEC_DATA");

    // Write TOC at current offset
    const toc_off = b.len as u64;
    b.patchU64Le(16, toc_off);
    b.writeU64Le(6); // section_count = 6
    b.writeSectionEntry("TOKENS", s0_off, 8);
    b.writeSectionEntry("STRINGS", s1_off, 8);
    b.writeSectionEntry("FIELDS", s2_off, 8);
    b.writeSectionEntry("FIELDSETS", s3_off, 9);
    b.writeSectionEntry("PATHS", s4_off, 9);
    b.writeSectionEntry("SPECS", s5_off, 9);

    const data = b.slice();
    expect_eq(isUsdc(data), true);
    expect_eq(detectUsdFormat(data) == UsdFormat.usdc, true);

    const boot = parseUsdcBootstrap(data).?;
    expect_eq(boot.version_major, 0);
    expect_eq(boot.version_minor, 8);
    expect_eq(boot.version_patch, 0);
    expect_eq(boot.toc_offset, toc_off);
    expect_eq(boot.section_count, 6);

    // Iterate all 6 sections
    let it = UsdcSectionIter.init(data, boot);
    let count: usize = 0;
    while (sec := it.next()) {
        expect_eq(sec.payload.len, sec.size as usize);
        count += 1;
    }
    expect_eq(count, 6);

    // Lookup specific sections
    const tok = findUsdcSection(data, boot, "TOKENS").?;
    expect_eq(std.mem.eql(u8, tok.payload, "TOK_DATA"), true);

    const fset = findUsdcSection(data, boot, "FIELDSETS").?;
    expect_eq(std.mem.eql(u8, fset.payload, "FSET_DATA"), true);

    const specs = findUsdcSection(data, boot, "SPECS").?;
    expect_eq(std.mem.eql(u8, specs.payload, "SPEC_DATA"), true);

    expect_eq(findUsdcSection(data, boot, "NONEXISTENT") == null, true);
}

fn testUsdaHeaderMetadataParsing() {
    const usda =
        "\xEF\xBB\xBF#usda 1.0\n" ++
        "(\n" ++
        "    # stage-level metadata\n" ++
        "    \"Scene description\"\n" ++
        "    customLayerData = {\n" ++
        "        string creator = \"CP OpenUSD\"\n" ++
        "    }\n" ++
        "    defaultPrim = \"World\"\n" ++
        "    metersPerUnit = 0.01\n" ++
        "    upAxis = \"Y\"\n" ++
        ")\n\n" ++
        "def Xform \"World\" {}\n";

    expect_eq(isUsda(usda), true);
    expect_eq(detectUsdFormat(usda) == UsdFormat.usda, true);

    const hdr = parseUsdaHeader(usda).?;
    expect_eq(std.mem.eql(u8, hdr.version, "1.0"), true);
    expect_eq(std.mem.eql(u8, hdr.default_prim, "World"), true);
    expect_eq(std.mem.eql(u8, hdr.up_axis, "Y"), true);
    expect_eq(hdr.has_meters_per_unit, true);
    expect_eq(approxEq(hdr.meters_per_unit, 0.01), true);

    // Header without stage metadata block
    const minimal = "#usda 1.0\ndef Scope \"Root\" {}\n";
    const mhdr = parseUsdaHeader(minimal).?;
    expect_eq(std.mem.eql(u8, mhdr.version, "1.0"), true);
    expect_eq(mhdr.default_prim.len, 0);
    expect_eq(mhdr.has_meters_per_unit, false);
}

fn testUsdzPrimaryAssetDetection() {
    // Valid USDZ: ZIP Local File Header (PK\x03\x04), compression=0 (Store), first file "scene.usdc"
    let z = CrateBuilder {};
    z.writeBytes("PK\x03\x04");
    z.writeU16Le(20); // version needed
    z.writeU16Le(0);  // flags
    z.writeU16Le(0);  // compression method = 0 (uncompressed Store)
    z.writeU16Le(0);  // mod time
    z.writeU16Le(0);  // mod date
    z.writeU32Le(0);  // crc32
    z.writeU32Le(0);  // comp size
    z.writeU32Le(0);  // uncomp size
    z.writeU16Le(10); // filename len ("scene.usdc")
    z.writeU16Le(0);  // extra len
    z.writeBytes("scene.usdc");

    expect_eq(isUsdz(z.slice()), true);
    expect_eq(detectUsdFormat(z.slice()) == UsdFormat.usdz, true);

    // Compressed ZIP (method = 8 Deflate) must NOT be detected as valid USDZ
    let z_comp = CrateBuilder {};
    z_comp.writeBytes("PK\x03\x04");
    z_comp.writeU16Le(20);
    z_comp.writeU16Le(0);
    z_comp.writeU16Le(8); // Deflate -> invalid for USDZ
    z_comp.writeZeros(16);
    z_comp.writeU16Le(9);
    z_comp.writeU16Le(0);
    z_comp.writeBytes("scene.usd");
    expect_eq(isUsdz(z_comp.slice()), false);

    // Uncompressed ZIP whose first entry is not .usd/.usdc/.usda -> not USDZ
    let z_txt = CrateBuilder {};
    z_txt.writeBytes("PK\x03\x04");
    z_txt.writeZeros(22);
    z_txt.writeU16Le(10);
    z_txt.writeU16Le(0);
    z_txt.writeBytes("readme.txt");
    expect_eq(isUsdz(z_txt.slice()), false);
}

fn testUsdcEmptyTocV010() {
    let b = CrateBuilder {};
    b.writeBytes(USDC_MAGIC);
    b.buf[b.len] = 0;
    b.buf[b.len + 1] = 10;
    b.buf[b.len + 2] = 0;
    b.len += 3;
    b.writeZeros(5);
    b.writeU64Le(88); // toc_offset immediately after bootstrap
    b.writeZeros(64);
    b.writeU64Le(0);  // section_count = 0

    const boot = parseUsdcBootstrap(b.slice()).?;
    expect_eq(boot.version_major, 0);
    expect_eq(boot.version_minor, 10);
    expect_eq(boot.section_count, 0);

    let it = UsdcSectionIter.init(b.slice(), boot);
    expect_eq(it.next() == null, true);
}

fn testMalformedUsdGuards() {
    // Unknown format / bad magic / truncated
    expect_eq(detectUsdFormat("") == UsdFormat.unknown, true);
    expect_eq(detectUsdFormat("PXR-USDC") == UsdFormat.unknown, true); // < 88 bytes
    expect_eq(parseUsdcBootstrap("NOT-USDC") == null, true);

    // toc_offset < 88 (overlaps bootstrap)
    let b1 = CrateBuilder {};
    b1.writeBytes(USDC_MAGIC);
    b1.writeZeros(8);
    b1.writeU64Le(16); // invalid toc_offset < 88
    b1.writeZeros(64);
    b1.writeU64Le(0);
    expect_eq(parseUsdcBootstrap(b1.slice()) == null, true);

    // section_count overflow (0xFFFFFFFFFFFFFFFF)
    let b2 = CrateBuilder {};
    b2.writeBytes(USDC_MAGIC);
    b2.writeZeros(8);
    b2.writeU64Le(88);
    b2.writeZeros(64);
    b2.writeU64Le(0xFFFFFFFFFFFFFFFF);
    expect_eq(parseUsdcBootstrap(b2.slice()) == null, true);

    // Section payload start + size out of bounds
    let b3 = CrateBuilder {};
    b3.writeBytes(USDC_MAGIC);
    b3.writeZeros(8);
    b3.writeU64Le(88);
    b3.writeZeros(64);
    b3.writeU64Le(1);
    b3.writeSectionEntry("TOKENS", 50, 99999); // exceeds file length
    const boot3 = parseUsdcBootstrap(b3.slice()).?;
    let it3 = UsdcSectionIter.init(b3.slice(), boot3);
    expect_eq(it3.next() == null, true);

    // Unclosed .usda stage metadata block
    expect_eq(parseUsdaHeader("#usda 1.0 (\n defaultPrim = \"A\"") == null, true);
    expect_eq(parseUsdaHeader("#usda ") == null, true);
}

test("1. .usdc v0.8.0 88B Bootstrap & 6 Core TOC Sections + findUsdcSection", () => {
    testUsdcBootstrapAndSixSections();
});

test("2. .usda 1.0 Stage Metadata Header (defaultPrim, upAxis, metersPerUnit)", () => {
    testUsdaHeaderMetadataParsing();
});

test("3. .usdz Uncompressed ZIP Primary Asset Detection & Compression Guard", () => {
    testUsdzPrimaryAssetDetection();
});

test("4. .usdc v0.10.0 Empty TOC (section_count == 0) Boundary", () => {
    testUsdcEmptyTocV010();
});

test("5. Bad Magic, Overflow toc_offset/section_count & Truncated USD Guards", () => {
    testMalformedUsdGuards();
});
