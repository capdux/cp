// Source: cp/cap/test/test_ktx_production_deep.cp (Production deep tests for cap:ktx)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    KtxVersion,
    Ktx1Header,
    Ktx2Header,
    Ktx2Level,
    isKtx1,
    isKtx2,
    detectKtxVersion,
    parseKtx1Header,
    parseKtx2Header,
    readKtx2Level,
} from "cap:gfx";

const KTX1_MAGIC: [12]u8 = [0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A];
const KTX2_MAGIC: [12]u8 = [0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A];

fn writeU32Le(buf: []u8, pos: usize, v: u32) {
    buf[pos] = u8(v & 0xFF);
    buf[pos + 1] = u8((v >> 8) & 0xFF);
    buf[pos + 2] = u8((v >> 16) & 0xFF);
    buf[pos + 3] = u8((v >> 24) & 0xFF);
}

fn writeU32Be(buf: []u8, pos: usize, v: u32) {
    buf[pos] = u8((v >> 24) & 0xFF);
    buf[pos + 1] = u8((v >> 16) & 0xFF);
    buf[pos + 2] = u8((v >> 8) & 0xFF);
    buf[pos + 3] = u8(v & 0xFF);
}

fn writeU64Le(buf: []u8, pos: usize, v: u64) {
    writeU32Le(buf, pos, u32(v & 0xFFFF_FFFF));
    writeU32Le(buf, pos + 4, u32((v >> 32) & 0xFFFF_FFFF));
}

fn copyMagic(dst: []u8, magic: [12]u8) {
    let i: usize = 0;
    while (i < 12) {
        dst[i] = magic[i];
        i += 1;
    }
}

test("1. Magic detection distinguishes KTX1, KTX2, and unknown inputs", () => {
    expect_eq(isKtx1(KTX1_MAGIC[0..]), true);
    expect_eq(isKtx2(KTX1_MAGIC[0..]), false);
    expect_eq(detectKtxVersion(KTX1_MAGIC[0..]) == KtxVersion.ktx1, true);

    expect_eq(isKtx2(KTX2_MAGIC[0..]), true);
    expect_eq(isKtx1(KTX2_MAGIC[0..]), false);
    expect_eq(detectKtxVersion(KTX2_MAGIC[0..]) == KtxVersion.ktx2, true);

    expect_eq(detectKtxVersion("DDS ") == KtxVersion.unknown, true);
    expect_eq(detectKtxVersion(KTX2_MAGIC[0..8]) == KtxVersion.unknown, true);
});

test("2. parseKtx1Header auto-adapts both little-endian and big-endian KTX 1.1 headers", () => {
    // Little-endian KTX1 (64 header + 16 KVD = 80 bytes)
    let le: [80]u8 = std.mem.zeroes([80]u8);
    copyMagic(le[0..], KTX1_MAGIC);
    writeU32Le(le[0..], 12, 0x04030201); // LE marker
    writeU32Le(le[0..], 16, 0x1401);     // GL_UNSIGNED_BYTE
    writeU32Le(le[0..], 20, 1);          // glTypeSize
    writeU32Le(le[0..], 24, 0x1908);     // GL_RGBA
    writeU32Le(le[0..], 28, 0x8058);     // GL_RGBA8
    writeU32Le(le[0..], 32, 0x1908);     // glBaseInternalFormat
    writeU32Le(le[0..], 36, 512);        // pixelWidth
    writeU32Le(le[0..], 40, 256);        // pixelHeight
    writeU32Le(le[0..], 44, 0);          // pixelDepth
    writeU32Le(le[0..], 48, 0);          // arrayElements
    writeU32Le(le[0..], 52, 6);          // faces (cubemap)
    writeU32Le(le[0..], 56, 10);         // mipmapLevels
    writeU32Le(le[0..], 60, 16);         // bytesOfKeyValueData

    const h_le = parseKtx1Header(le[0..]) ?? unreachable;
    expect_eq(h_le.is_big_endian, false);
    expect_eq(h_le.gl_type, 0x1401);
    expect_eq(h_le.gl_format, 0x1908);
    expect_eq(h_le.gl_internal_format, 0x8058);
    expect_eq(h_le.pixel_width, 512);
    expect_eq(h_le.pixel_height, 256);
    expect_eq(h_le.faces, 6);
    expect_eq(h_le.mipmap_levels, 10);
    expect_eq(h_le.dataOffset(), 80);

    // Big-endian KTX1
    let be: [64]u8 = std.mem.zeroes([64]u8);
    copyMagic(be[0..], KTX1_MAGIC);
    writeU32Be(be[0..], 12, 0x04030201); // BE marker (wire bytes 04 03 02 01)
    writeU32Be(be[0..], 36, 1024);       // pixelWidth
    writeU32Be(be[0..], 40, 1024);       // pixelHeight
    writeU32Be(be[0..], 52, 1);          // faces
    writeU32Be(be[0..], 56, 11);         // mipmapLevels

    const h_be = parseKtx1Header(be[0..]) ?? unreachable;
    expect_eq(h_be.is_big_endian, true);
    expect_eq(h_be.pixel_width, 1024);
    expect_eq(h_be.pixel_height, 1024);
    expect_eq(h_be.mipmap_levels, 11);
    expect_eq(h_be.dataOffset(), 64);
});

test("3. parseKtx2Header parses 80-byte KTX2 header including DFD/KVD/SGD indices", () => {
    // 80 header + 24 level0 + 16 DFD + 16 KVD + 16 SGD = 152 bytes
    let buf: [152]u8 = std.mem.zeroes([152]u8);
    copyMagic(buf[0..], KTX2_MAGIC);
    writeU32Le(buf[0..], 12, 37);    // VK_FORMAT_R8G8B8A8_UNORM = 37
    writeU32Le(buf[0..], 16, 1);     // typeSize
    writeU32Le(buf[0..], 20, 2048);  // pixelWidth
    writeU32Le(buf[0..], 24, 1024);  // pixelHeight
    writeU32Le(buf[0..], 28, 0);     // pixelDepth
    writeU32Le(buf[0..], 32, 0);     // layerCount
    writeU32Le(buf[0..], 36, 1);     // faceCount
    writeU32Le(buf[0..], 40, 0);     // levelCount=0 -> effectiveLevels() == 1
    writeU32Le(buf[0..], 44, 2);     // supercompressionScheme = Zstandard (2)
    writeU32Le(buf[0..], 48, 104);   // dfdByteOffset
    writeU32Le(buf[0..], 52, 16);    // dfdByteLength
    writeU32Le(buf[0..], 56, 120);   // kvdByteOffset
    writeU32Le(buf[0..], 60, 16);    // kvdByteLength
    writeU64Le(buf[0..], 64, 136);   // sgdByteOffset
    writeU64Le(buf[0..], 72, 16);    // sgdByteLength

    const h = parseKtx2Header(buf[0..]) ?? unreachable;
    expect_eq(h.vk_format, 37);
    expect_eq(h.type_size, 1);
    expect_eq(h.pixel_width, 2048);
    expect_eq(h.pixel_height, 1024);
    expect_eq(h.level_count, 0);
    expect_eq(h.effectiveLevels(), 1);
    expect_eq(h.supercompression_scheme, 2);
    expect_eq(h.dfd_byte_offset, 104);
    expect_eq(h.dfd_byte_length, 16);
    expect_eq(h.kvd_byte_offset, 120);
    expect_eq(h.sgd_byte_offset, 136);
    expect_eq(h.sgd_byte_length, 16);
});

test("4. readKtx2Level reads multi-level mipmap index entries and extracts exact payloads", () => {
    // 80 header + 2 * 24 level index (48 bytes -> 128) + 16 bytes mip0 + 8 bytes mip1 = 152 bytes
    let buf: [152]u8 = std.mem.zeroes([152]u8);
    copyMagic(buf[0..], KTX2_MAGIC);
    writeU32Le(buf[0..], 12, 145);  // VK_FORMAT_BC7_UNORM_BLOCK
    writeU32Le(buf[0..], 20, 64);
    writeU32Le(buf[0..], 24, 64);
    writeU32Le(buf[0..], 36, 1);
    writeU32Le(buf[0..], 40, 2);    // 2 mip levels

    // Level 0 at offset 128, len 16, uncompressed 32
    writeU64Le(buf[0..], 80, 128);
    writeU64Le(buf[0..], 88, 16);
    writeU64Le(buf[0..], 96, 32);
    buf[128] = 0xAA;
    buf[143] = 0xBB;

    // Level 1 at offset 144, len 8, uncompressed 16
    writeU64Le(buf[0..], 104, 144);
    writeU64Le(buf[0..], 112, 8);
    writeU64Le(buf[0..], 120, 16);
    buf[144] = 0xCC;
    buf[151] = 0xDD;

    const l0 = readKtx2Level(buf[0..], 0) ?? unreachable;
    expect_eq(l0.byte_offset, 128);
    expect_eq(l0.byte_length, 16);
    expect_eq(l0.uncompressed_byte_length, 32);
    const p0 = l0.payload(buf[0..]) ?? unreachable;
    expect_eq(p0.len, 16);
    expect_eq(p0[0], 0xAA);
    expect_eq(p0[15], 0xBB);

    const l1 = readKtx2Level(buf[0..], 1) ?? unreachable;
    expect_eq(l1.byte_offset, 144);
    expect_eq(l1.byte_length, 8);
    const p1 = l1.payload(buf[0..]) ?? unreachable;
    expect_eq(p1.len, 8);
    expect_eq(p1[0], 0xCC);
    expect_eq(p1[7], 0xDD);

    // Out-of-range level index
    expect_eq(readKtx2Level(buf[0..], 2) == null, true);
});

test("5. Malformed and out-of-bounds protection for KTX1/KTX2 headers and level indices", () => {
    // KTX1 bad endianness marker
    let bad_k1: [64]u8 = std.mem.zeroes([64]u8);
    copyMagic(bad_k1[0..], KTX1_MAGIC);
    writeU32Le(bad_k1[0..], 12, 0xDEADBEEF);
    expect_eq(parseKtx1Header(bad_k1[0..]) == null, true);

    // KTX1 KVD exceeds buffer length
    writeU32Le(bad_k1[0..], 12, 0x04030201);
    writeU32Le(bad_k1[0..], 60, 100);
    expect_eq(parseKtx1Header(bad_k1[0..]) == null, true);

    // KTX2 truncated < 80 bytes
    let short_k2: [79]u8 = std.mem.zeroes([79]u8);
    copyMagic(short_k2[0..], KTX2_MAGIC);
    expect_eq(parseKtx2Header(short_k2[0..]) == null, true);

    // KTX2 DFD out-of-bounds
    let bad_dfd: [80]u8 = std.mem.zeroes([80]u8);
    copyMagic(bad_dfd[0..], KTX2_MAGIC);
    writeU32Le(bad_dfd[0..], 48, 80);
    writeU32Le(bad_dfd[0..], 52, 16);
    expect_eq(parseKtx2Header(bad_dfd[0..]) == null, true);

    // KTX2 level index payload out-of-bounds & u64 overflow
    let bad_lvl: [104]u8 = std.mem.zeroes([104]u8);
    copyMagic(bad_lvl[0..], KTX2_MAGIC);
    writeU32Le(bad_lvl[0..], 40, 1);
    writeU64Le(bad_lvl[0..], 80, 0xFFFF_FFFF_FFFF_FFF0);
    writeU64Le(bad_lvl[0..], 88, 0x40);
    expect_eq(readKtx2Level(bad_lvl[0..], 0) == null, true);
});
