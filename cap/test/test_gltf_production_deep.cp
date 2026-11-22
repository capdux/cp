// Source: cp/cap/test/test_gltf_production_deep.cp (Production deep tests for cap:gltf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    GLB_MAGIC,
    GLB_VERSION_2,
    CHUNK_TYPE_JSON,
    CHUNK_TYPE_BIN,
    COMPONENT_BYTE,
    COMPONENT_UNSIGNED_BYTE,
    COMPONENT_SHORT,
    COMPONENT_UNSIGNED_SHORT,
    COMPONENT_UNSIGNED_INT,
    COMPONENT_FLOAT,
    ChunkType,
    AccessorType,
    GlbHeader,
    GlbChunk,
    GlbDocument,
    isGlb,
    parseGlbHeader,
    GlbChunkIter,
    extractJsonChunk,
    extractBinChunk,
    parseGlb,
    componentByteSize,
    accessorComponentCount,
    parseAccessorType,
    accessorElementStride,
} from "cap:gfx";

fn putU32LE(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn writeChunk(dst: []u8, off: usize, raw_type: u32, payload: []const u8): usize {
    putU32LE(dst, off, payload.len as u32);
    putU32LE(dst, off + 4, raw_type);
    copyBytes(dst, off + 8, payload);
    return payload.len + 8;
}

test("1. parseGlb, extractJsonChunk, and extractBinChunk decode complete GLB 2.0 JSON+BIN document", () => {
    // JSON payload padded to 4-byte boundary (24 bytes)
    const json_str = "{\"asset\":{\"version\":\"2\"}}   ";
    const json_aligned = json_str[0..28];
    const bin_data: [12]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];

    // 12 header + (8 + 28) JSON + (8 + 12) BIN = 68 bytes
    let buf: [68]u8 = undefined;
    const s = buf[0..68];
    putU32LE(s, 0, GLB_MAGIC);
    putU32LE(s, 4, GLB_VERSION_2);
    putU32LE(s, 8, 68);
    const c0 = writeChunk(s, 12, CHUNK_TYPE_JSON, json_aligned);
    const c1 = writeChunk(s, 12 + c0, CHUNK_TYPE_BIN, bin_data[0..12]);
    expect_eq(12 + c0 + c1, 68);

    expect_eq(isGlb(s), true);
    const doc = parseGlb(s) ?? unreachable;
    expect_eq(doc.header.magic, GLB_MAGIC);
    expect_eq(doc.header.version, 2);
    expect_eq(doc.header.length, 68);
    expect_eq(doc.chunk_count, 2);
    expect_eq(std.mem.eql(u8, doc.json, json_aligned), true);

    const b = doc.bin ?? unreachable;
    expect_eq(b.len, 12);
    expect_eq(b[0], 1);
    expect_eq(b[11], 12);

    const j_direct = extractJsonChunk(s) ?? unreachable;
    expect_eq(std.mem.eql(u8, j_direct, json_aligned), true);
    const b_direct = extractBinChunk(s) ?? unreachable;
    expect_eq(b_direct.len, 12);
});

test("2. GlbChunkIter and parseGlb safely skip unknown extension chunks between JSON and BIN", () => {
    const json_4 = "{}  "; // 4 bytes
    const ext_8 = "EXTDATA!"; // 8 bytes
    const bin_4: [4]u8 = [0xAA, 0xBB, 0xCC, 0xDD];

    // 12 + (8+4) + (8+8) + (8+4) = 52 bytes
    let buf: [52]u8 = undefined;
    const s = buf[0..52];
    putU32LE(s, 0, GLB_MAGIC);
    putU32LE(s, 4, GLB_VERSION_2);
    putU32LE(s, 8, 52);

    let off: usize = 12;
    off += writeChunk(s, off, CHUNK_TYPE_JSON, json_4);
    off += writeChunk(s, off, 0x54584558, ext_8); // "XEXT" unknown chunk
    off += writeChunk(s, off, CHUNK_TYPE_BIN, bin_4[0..4]);
    expect_eq(off, 52);

    let it = GlbChunkIter.init(s);
    const ch0 = it.next() ?? unreachable;
    expect_eq(ch0.chunk_type == ChunkType.json, true);

    const ch1 = it.next() ?? unreachable;
    expect_eq(ch1.chunk_type == ChunkType.unknown, true);
    expect_eq(ch1.raw_type, 0x54584558);
    expect_eq(std.mem.eql(u8, ch1.data, ext_8), true);

    const ch2 = it.next() ?? unreachable;
    expect_eq(ch2.chunk_type == ChunkType.bin, true);
    expect_eq(ch2.data[0], 0xAA);
    expect_eq(ch2.data[3], 0xDD);
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);

    const doc = parseGlb(s) ?? unreachable;
    expect_eq(doc.chunk_count, 3);
    expect_eq((doc.bin ?? unreachable).len, 4);
});

test("3. parseGlbHeader rejects wrong magic, version != 2, unaligned length, and truncated header", () => {
    let hdr: [12]u8 = undefined;
    const s = hdr[0..12];

    // Wrong magic
    putU32LE(s, 0, 0x12345678);
    putU32LE(s, 4, 2);
    putU32LE(s, 8, 12);
    expect_eq(parseGlbHeader(s) == null, true);

    // Wrong version (glTF 1.0)
    putU32LE(s, 0, GLB_MAGIC);
    putU32LE(s, 4, 1);
    expect_eq(parseGlbHeader(s) == null, true);

    // Total length exceeds buffer
    putU32LE(s, 4, 2);
    putU32LE(s, 8, 100);
    expect_eq(parseGlbHeader(s) == null, true);

    // Total length not 4-byte aligned
    let buf16: [16]u8 = undefined;
    const s16 = buf16[0..16];
    putU32LE(s16, 0, GLB_MAGIC);
    putU32LE(s16, 4, 2);
    putU32LE(s16, 8, 14);
    expect_eq(parseGlbHeader(s16) == null, true);

    // Truncated (< 12 bytes)
    expect_eq(parseGlbHeader(s[0..8]) == null, true);
});

test("4. parseGlb and GlbChunkIter reject unaligned chunk length, out-of-bounds chunk, and non-JSON first chunk", () => {
    // Case A: Chunk length not 4-byte aligned (3 bytes)
    let bad_align: [24]u8 = undefined;
    const s1 = bad_align[0..24];
    putU32LE(s1, 0, GLB_MAGIC);
    putU32LE(s1, 4, 2);
    putU32LE(s1, 8, 24);
    putU32LE(s1, 12, 3); // unaligned chunk length
    putU32LE(s1, 16, CHUNK_TYPE_JSON);
    expect_eq(parseGlb(s1) == null, true);

    // Case B: Chunk length exceeds total length
    putU32LE(s1, 12, 64);
    expect_eq(parseGlb(s1) == null, true);

    // Case C: First chunk is BIN instead of JSON
    putU32LE(s1, 12, 4);
    putU32LE(s1, 16, CHUNK_TYPE_BIN);
    expect_eq(parseGlb(s1) == null, true);
});

test("5. componentByteSize, accessorComponentCount, parseAccessorType, and accessorElementStride compute exact strides", () => {
    expect_eq(componentByteSize(COMPONENT_BYTE) ?? 0, 1);
    expect_eq(componentByteSize(COMPONENT_UNSIGNED_BYTE) ?? 0, 1);
    expect_eq(componentByteSize(COMPONENT_SHORT) ?? 0, 2);
    expect_eq(componentByteSize(COMPONENT_UNSIGNED_SHORT) ?? 0, 2);
    expect_eq(componentByteSize(COMPONENT_UNSIGNED_INT) ?? 0, 4);
    expect_eq(componentByteSize(COMPONENT_FLOAT) ?? 0, 4);
    expect_eq(componentByteSize(5124) == null, true); // GL_INT not in glTF 2.0

    expect_eq(accessorComponentCount(AccessorType.scalar), 1);
    expect_eq(accessorComponentCount(AccessorType.vec2), 2);
    expect_eq(accessorComponentCount(AccessorType.vec3), 3);
    expect_eq(accessorComponentCount(AccessorType.vec4), 4);
    expect_eq(accessorComponentCount(AccessorType.mat2), 4);
    expect_eq(accessorComponentCount(AccessorType.mat3), 9);
    expect_eq(accessorComponentCount(AccessorType.mat4), 16);

    expect_eq((parseAccessorType("VEC3") ?? unreachable) == AccessorType.vec3, true);
    expect_eq((parseAccessorType("MAT4") ?? unreachable) == AccessorType.mat4, true);
    expect_eq(parseAccessorType("VEC5") == null, true);

    expect_eq(accessorElementStride(COMPONENT_FLOAT, AccessorType.vec3) ?? 0, 12);
    expect_eq(accessorElementStride(COMPONENT_FLOAT, AccessorType.mat4) ?? 0, 64);
    expect_eq(accessorElementStride(COMPONENT_UNSIGNED_SHORT, AccessorType.vec4) ?? 0, 8);
    expect_eq(accessorElementStride(9999, AccessorType.scalar) == null, true);
});
