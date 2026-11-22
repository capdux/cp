// Source: cp/cap/test/test_dmg_production_deep.cp (Production deep tests for cap:dmg)

import { test, expect_eq } from "cap:test";
import {
    DMG_KOLY_MAGIC,
    DMG_KOLY_SIZE,
    DMG_MISH_MAGIC,
    DMG_MISH_HEADER_SIZE,
    DMG_STRIPE_SIZE,
    BLKX_ZERO,
    BLKX_RAW,
    BLKX_IGNORE,
    BLKX_ADC,
    BLKX_ZLIB,
    BLKX_BZ2,
    BLKX_LZFSE,
    BLKX_LZMA,
    BLKX_COMMENT,
    BLKX_LAST,
    DmgKolyHeader,
    DmgMishHeader,
    DmgBlkStripe,
    isDmg,
    parseDmgKoly,
    parseDmgMish,
    DmgStripeIter,
} from "cap:devops";

fn zeroBytes(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
}

fn writeBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn writeU32BE(dst: []u8, off: usize, val: u32) {
    dst[off + 0] = ((val >> 24) & 0xFF) as u8;
    dst[off + 1] = ((val >> 16) & 0xFF) as u8;
    dst[off + 2] = ((val >> 8) & 0xFF) as u8;
    dst[off + 3] = (val & 0xFF) as u8;
}

fn writeU64BE(dst: []u8, off: usize, val: u64) {
    writeU32BE(dst, off, ((val >> 32) & 0xFFFFFFFF) as u32);
    writeU32BE(dst, off + 4, (val & 0xFFFFFFFF) as u32);
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

fn writeKolyTrailer(
    dst: []u8,
    base: usize,
    data_off: u64,
    data_len: u64,
    xml_off: u64,
    xml_len: u64,
    sector_count: u64,
) {
    writeBytes(dst, base, DMG_KOLY_MAGIC);
    writeU32BE(dst, base + 4, 4); // version
    writeU32BE(dst, base + 8, 512); // header_size
    writeU32BE(dst, base + 12, 1); // flags
    writeU64BE(dst, base + 16, 0); // running_data_fork_offset
    writeU64BE(dst, base + 24, data_off);
    writeU64BE(dst, base + 32, data_len);
    writeU64BE(dst, base + 40, 0); // rsrc_fork_offset
    writeU64BE(dst, base + 48, 0); // rsrc_fork_length
    writeU32BE(dst, base + 56, 1); // segment_number
    writeU32BE(dst, base + 60, 1); // segment_count
    writeU32BE(dst, base + 80, 2); // data_checksum_type (CRC32)
    writeU32BE(dst, base + 84, 32); // data_checksum_size
    writeU64BE(dst, base + 216, xml_off);
    writeU64BE(dst, base + 224, xml_len);
    writeU32BE(dst, base + 352, 2); // master_checksum_type
    writeU32BE(dst, base + 488, 1); // image_variant (UDIF)
    writeU64BE(dst, base + 492, sector_count);
}

fn writeStripe(
    dst: []u8,
    off: usize,
    entry_type: u32,
    sector_start: u64,
    sector_count: u64,
    comp_offset: u64,
    comp_length: u64,
): usize {
    writeU32BE(dst, off, entry_type);
    writeU32BE(dst, off + 4, 0);
    writeU64BE(dst, off + 8, sector_start);
    writeU64BE(dst, off + 16, sector_count);
    writeU64BE(dst, off + 24, comp_offset);
    writeU64BE(dst, off + 32, comp_length);
    return off + DMG_STRIPE_SIZE;
}

test("1. parses 512B koly trailer and extracts zero-copy dataForkSlice and xmlSlice", () => {
    const data_payload = "RAW-COMPRESSED-FORK-BYTES";
    const xml_payload = "<plist version=\"1.0\"><dict><key>blkx</key></dict></plist>";
    const total_size: usize = data_payload.len + xml_payload.len + DMG_KOLY_SIZE;

    let raw: [768]u8 = undefined;
    let buf = raw[0..total_size];
    zeroBytes(buf);

    writeBytes(buf, 0, data_payload);
    writeBytes(buf, data_payload.len, xml_payload);
    writeKolyTrailer(
        buf,
        data_payload.len + xml_payload.len,
        0,
        data_payload.len as u64,
        data_payload.len as u64,
        xml_payload.len as u64,
        2048,
    );

    expect_eq(DMG_KOLY_SIZE, 512);
    expect_eq(isDmg(buf), true);

    if (koly := parseDmgKoly(buf)) {
        expect_eq(koly.version, 4);
        expect_eq(koly.header_size, 512);
        expect_eq(koly.flags, 1);
        expect_eq(koly.segment_number, 1);
        expect_eq(koly.segment_count, 1);
        expect_eq(koly.data_checksum_type, 2);
        expect_eq(koly.data_checksum_size, 32);
        expect_eq(koly.master_checksum_type, 2);
        expect_eq(koly.image_variant, 1);
        expect_eq(koly.sector_count, 2048);
        expect_eq(koly.uncompressedBytes(), 1048576);

        if (fork := koly.dataForkSlice(buf)) {
            expect_eq(sliceEq(fork, data_payload), true);
        } else {
            expect_eq(true, false);
        }

        if (xml := koly.xmlSlice(buf)) {
            expect_eq(sliceEq(xml, xml_payload), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("2. parses 204B mish header and iterates 40B BLKX stripes (RAW/ZLIB/LZFSE/LAST)", () => {
    const total_len: usize = DMG_MISH_HEADER_SIZE + (4 as usize) * DMG_STRIPE_SIZE;
    let raw: [400]u8 = undefined;
    let buf = raw[0..total_len];
    zeroBytes(buf);

    writeBytes(buf, 0, DMG_MISH_MAGIC);
    writeU32BE(buf, 4, 1); // version
    writeU64BE(buf, 8, 64); // sector_start
    writeU64BE(buf, 16, 4096); // sector_count
    writeU64BE(buf, 24, 1024); // data_offset
    writeU32BE(buf, 32, 520); // buffers_needed
    writeU32BE(buf, 36, 3); // block_descriptors
    writeU32BE(buf, 200, 4); // stripe_count

    let p: usize = DMG_MISH_HEADER_SIZE;
    p = writeStripe(buf, p, BLKX_RAW, 0, 16, 0, 8192);
    p = writeStripe(buf, p, BLKX_ZLIB, 16, 128, 8192, 12345);
    p = writeStripe(buf, p, BLKX_LZFSE, 144, 256, 20537, 9988);
    p = writeStripe(buf, p, BLKX_LAST, 400, 0, 30525, 0);
    expect_eq(p, total_len);

    expect_eq(DMG_MISH_HEADER_SIZE, 204);
    expect_eq(DMG_STRIPE_SIZE, 40);

    if (mish := parseDmgMish(buf)) {
        expect_eq(mish.version, 1);
        expect_eq(mish.sector_start, 64);
        expect_eq(mish.sector_count, 4096);
        expect_eq(mish.data_offset, 1024);
        expect_eq(mish.buffers_needed, 520);
        expect_eq(mish.block_descriptors, 3);
        expect_eq(mish.stripe_count, 4);

        let it = DmgStripeIter.init(buf);
        if (s0 := it.next()) {
            expect_eq(s0.entry_type, BLKX_RAW);
            expect_eq(s0.sector_start, 0);
            expect_eq(s0.sector_count, 16);
            expect_eq(s0.comp_offset, 0);
            expect_eq(s0.comp_length, 8192);
        } else {
            expect_eq(true, false);
        }
        if (s1 := it.next()) {
            expect_eq(s1.entry_type, BLKX_ZLIB);
            expect_eq(s1.sector_start, 16);
            expect_eq(s1.comp_length, 12345);
        } else {
            expect_eq(true, false);
        }
        if (s2 := it.next()) {
            expect_eq(s2.entry_type, BLKX_LZFSE);
            expect_eq(s2.sector_count, 256);
            expect_eq(s2.comp_offset, 20537);
        } else {
            expect_eq(true, false);
        }
        if (s3 := it.next()) {
            expect_eq(s3.entry_type, BLKX_LAST);
            expect_eq(s3.sector_count, 0);
        } else {
            expect_eq(true, false);
        }
        expect_eq(it.next() == null, true);
        expect_eq(it.had_error, false);
    } else {
        expect_eq(true, false);
    }
});

test("3. uncompressedBytes saturates cleanly on extreme sector_count without overflow", () => {
    let raw: [512]u8 = undefined;
    let buf = raw[0..512];
    zeroBytes(buf);
    writeKolyTrailer(buf, 0, 0, 0, 0, 0, 0xFFFFFFFFFFFFFFFF);

    if (koly := parseDmgKoly(buf)) {
        expect_eq(koly.uncompressedBytes(), 0xFFFFFFFFFFFFFFFF);
    } else {
        expect_eq(true, false);
    }

    // Exact maximum non-overflow value: 0x007FFFFFFFFFFFFF * 512
    writeKolyTrailer(buf, 0, 0, 0, 0, 0, 36028797018963967);
    if (koly2 := parseDmgKoly(buf)) {
        expect_eq(koly2.uncompressedBytes(), 36028797018963967 * 512);
    } else {
        expect_eq(true, false);
    }
});

test("4. verifies all BLKX block type constants match Apple UDIF specification", () => {
    expect_eq(BLKX_ZERO, 0x00000000);
    expect_eq(BLKX_RAW, 0x00000001);
    expect_eq(BLKX_IGNORE, 0x00000002);
    expect_eq(BLKX_ADC, 0x80000004);
    expect_eq(BLKX_ZLIB, 0x80000005);
    expect_eq(BLKX_BZ2, 0x80000006);
    expect_eq(BLKX_LZFSE, 0x80000007);
    expect_eq(BLKX_LZMA, 0x80000008);
    expect_eq(BLKX_COMMENT, 0x7FFFFFFE);
    expect_eq(BLKX_LAST, 0xFFFFFFFF);
});

test("5. rejects truncated image, wrong koly magic, bad version, non-512 header_size, and out-of-bounds xml/data slices", () => {
    // 5a. Truncated (< 512 bytes)
    let small: [200]u8 = undefined;
    zeroBytes(small[0..200]);
    expect_eq(isDmg(small[0..200]), false);
    expect_eq(parseDmgKoly(small[0..200]) == null, true);

    // 5b. Valid baseline
    let raw: [512]u8 = undefined;
    let buf = raw[0..512];
    zeroBytes(buf);
    writeKolyTrailer(buf, 0, 0, 0, 0, 0, 10);
    expect_eq(isDmg(buf), true);

    // Wrong magic
    buf[0] = 'x';
    expect_eq(isDmg(buf), false);
    expect_eq(parseDmgKoly(buf) == null, true);
    buf[0] = 'k';

    // Wrong version (!= 4)
    writeU32BE(buf, 4, 3);
    expect_eq(isDmg(buf), false);
    writeU32BE(buf, 4, 4);

    // Non-512 header_size
    writeU32BE(buf, 8, 256);
    expect_eq(isDmg(buf), false);
    writeU32BE(buf, 8, 512);

    // Out-of-bounds xml_offset & overflow xml_offset + xml_length
    writeU64BE(buf, 216, 400);
    writeU64BE(buf, 224, 300); // 400 + 300 > 512
    writeU64BE(buf, 24, 0xFFFFFFFFFFFFFFFF); // overflow data_fork_offset
    writeU64BE(buf, 32, 16);
    if (koly := parseDmgKoly(buf)) {
        expect_eq(koly.xmlSlice(buf) == null, true);
        expect_eq(koly.dataForkSlice(buf) == null, true);
    } else {
        expect_eq(true, false);
    }
});

test("6. mish defensive bounds: truncated mish header, bad magic, and truncated stripe array", () => {
    // Truncated mish (< 204 bytes)
    let tiny: [100]u8 = undefined;
    zeroBytes(tiny[0..100]);
    expect_eq(parseDmgMish(tiny[0..100]) == null, true);

    let it_bad = DmgStripeIter.init(tiny[0..100]);
    expect_eq(it_bad.next() == null, true);
    expect_eq(it_bad.had_error, true);

    // Valid 204B mish claiming 3 stripes, but only 1 stripe (40B) is present
    let raw: [244]u8 = undefined;
    let buf = raw[0..244];
    zeroBytes(buf);
    writeBytes(buf, 0, DMG_MISH_MAGIC);
    writeU32BE(buf, 4, 1);
    writeU32BE(buf, 200, 3); // claims 3 stripes
    * = writeStripe(buf, DMG_MISH_HEADER_SIZE, BLKX_ZERO, 0, 8, 0, 0);

    let it = DmgStripeIter.init(buf);
    if (s0 := it.next()) {
        expect_eq(s0.entry_type, BLKX_ZERO);
    } else {
        expect_eq(true, false);
    }
    // Second stripe is truncated -> returns null and sets had_error
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, true);
});
