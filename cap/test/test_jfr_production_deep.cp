// Source: cp/cap/test/test_jfr_production_deep.cp (Production deep tests for cap:jfr)

import { test, expect_eq } from "cap:test";
import {
    JFR_MAGIC,
    JFR_CHUNK_HEADER_SIZE,
    JFR_EVENT_METADATA,
    JFR_EVENT_CHECKPOINT,
    JFR_FLAG_COMPRESSED_INTS,
    JFR_FLAG_FINAL_CHUNK,
    JfrChunkHeader,
    JfrChunk,
    JfrVarint,
    JfrEventFrame,
    JfrStats,
    parseJfrChunkHeader,
    isJfr,
    readJfrUleb128,
    JfrChunkIter,
    JfrEventIter,
    summarizeJfr,
} from "cap:binfmt";

fn zeroBytes(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
}

fn writeU16BE(dst: []u8, off: usize, val: u16) {
    dst[off + 0] = ((val >> 8) & 0xFF) as u8;
    dst[off + 1] = (val & 0xFF) as u8;
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

fn writeChunkHeader(
    dst: []u8,
    off: usize,
    chunk_size: u64,
    cp_off: u64,
    meta_off: u64,
    duration_ns: u64,
    features: u32,
) {
    dst[off + 0] = 'F';
    dst[off + 1] = 'L';
    dst[off + 2] = 'R';
    dst[off + 3] = 0;
    writeU16BE(dst, off + 4, 2); // major
    writeU16BE(dst, off + 6, 0); // minor
    writeU64BE(dst, off + 8, chunk_size);
    writeU64BE(dst, off + 16, cp_off);
    writeU64BE(dst, off + 24, meta_off);
    writeU64BE(dst, off + 32, 1700000000000000000); // start_time_nanos
    writeU64BE(dst, off + 40, duration_ns);
    writeU64BE(dst, off + 48, 1000000); // start_ticks
    writeU64BE(dst, off + 56, 1000000000); // ticks_per_second
    writeU32BE(dst, off + 64, features);
}

fn writeSimpleEvent(dst: []u8, off: usize, type_id: u8, payload: []const u8): usize {
    // 1-byte size (< 128) + 1-byte type_id (< 128) + payload
    const total: usize = (2 as usize) + payload.len;
    dst[off] = total as u8;
    dst[off + 1] = type_id;
    let i: usize = 0;
    while (i < payload.len) {
        dst[off + 2 + i] = payload[i];
        i += 1;
    }
    return off + total;
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

test("1. parses 68B JFR v2.0 chunk header with feature flags and durationMs", () => {
    let raw: [68]u8 = undefined;
    let buf = raw[0..68];
    zeroBytes(buf);

    const flags = JFR_FLAG_COMPRESSED_INTS | JFR_FLAG_FINAL_CHUNK;
    writeChunkHeader(buf, 0, 68, 68, 68, 250000000, flags); // 250 ms

    expect_eq(JFR_CHUNK_HEADER_SIZE, 68);
    expect_eq(isJfr(buf), true);

    if (hdr := parseJfrChunkHeader(buf, 0)) {
        expect_eq(hdr.major_version, 2);
        expect_eq(hdr.minor_version, 0);
        expect_eq(hdr.chunk_size, 68);
        expect_eq(hdr.start_time_nanos, 1700000000000000000);
        expect_eq(hdr.duration_nanos, 250000000);
        expect_eq(hdr.ticks_per_second, 1000000000);
        expect_eq(hdr.usesCompressedInts(), true);
        expect_eq(hdr.isFinalChunk(), true);
        expect_eq(hdr.durationMs() > 249.9 && hdr.durationMs() < 250.1, true);
    } else {
        expect_eq(true, false);
    }
});

test("2. iterates multi-chunk concatenated .jfr and events (Metadata, Checkpoint, Custom) + summarizeJfr", () => {
    let raw: [256]u8 = undefined;
    let buf = raw[0..256];
    zeroBytes(buf);

    // Chunk 1: 68B header + 3 events (Metadata=0, Checkpoint=1, Custom=42)
    let p: usize = JFR_CHUNK_HEADER_SIZE;
    const meta_off = p as u64;
    p = writeSimpleEvent(buf, p, 0, "META1");
    const cp_off = p as u64;
    p = writeSimpleEvent(buf, p, 1, "CP1");
    p = writeSimpleEvent(buf, p, 42, "SAMPLE");
    const chunk1_size = p;
    writeChunkHeader(buf, 0, chunk1_size as u64, cp_off, meta_off, 100000000, JFR_FLAG_COMPRESSED_INTS);

    // Chunk 2: starts at chunk1_size, 68B header + 2 events (Checkpoint=1, Custom=99)
    const c2_start = chunk1_size;
    let p2: usize = c2_start + JFR_CHUNK_HEADER_SIZE;
    p2 = writeSimpleEvent(buf, p2, 1, "CP2");
    p2 = writeSimpleEvent(buf, p2, 99, "GC");
    const chunk2_size = p2 - c2_start;
    writeChunkHeader(buf, c2_start, chunk2_size as u64, 68, 0, 150000000, JFR_FLAG_COMPRESSED_INTS | JFR_FLAG_FINAL_CHUNK);

    const recording = buf[0..p2];

    let cit = JfrChunkIter.init(recording);
    let chunk_idx: usize = 0;
    while (ch := cit.next()) {
        if (chunk_idx == 0) {
            expect_eq(ch.offset, 0);
            expect_eq(ch.header.isFinalChunk(), false);
            let eit = JfrEventIter.init(ch.slice);
            if (e0 := eit.next()) {
                expect_eq(e0.event_type_id, JFR_EVENT_METADATA);
                expect_eq(sliceEq(e0.payload, "META1"), true);
            } else { expect_eq(true, false); }
            if (e1 := eit.next()) {
                expect_eq(e1.event_type_id, JFR_EVENT_CHECKPOINT);
                expect_eq(sliceEq(e1.payload, "CP1"), true);
            } else { expect_eq(true, false); }
            if (e2 := eit.next()) {
                expect_eq(e2.event_type_id, 42);
                expect_eq(sliceEq(e2.payload, "SAMPLE"), true);
            } else { expect_eq(true, false); }
            expect_eq(eit.next() == null, true);
            expect_eq(eit.had_error, false);
        } else if (chunk_idx == 1) {
            expect_eq(ch.offset, c2_start);
            expect_eq(ch.header.isFinalChunk(), true);
        }
        chunk_idx += 1;
    }
    expect_eq(chunk_idx, 2);
    expect_eq(cit.had_error, false);

    if (stats := summarizeJfr(recording)) {
        expect_eq(stats.chunk_count, 2);
        expect_eq(stats.event_count, 5);
        expect_eq(stats.metadata_count, 1);
        expect_eq(stats.checkpoint_count, 2);
        expect_eq(stats.total_duration_ms > 249.9 && stats.total_duration_ms < 250.1, true);
    } else {
        expect_eq(true, false);
    }
});

test("3. readJfrUleb128 handles 1-byte, 2-byte, and JFR 9-byte full u64 max extreme values", () => {
    // 1-byte: 0x7F => 127
    const v1: [1]u8 = [0x7F];
    if (r := readJfrUleb128(v1[0..1], 0)) {
        expect_eq(r.value, 127);
        expect_eq(r.next_pos, 1);
    } else { expect_eq(true, false); }

    // 2-byte: [0x80, 0x01] => 128
    const v2: [2]u8 = [0x80, 0x01];
    if (r := readJfrUleb128(v2[0..2], 0)) {
        expect_eq(r.value, 128);
        expect_eq(r.next_pos, 2);
    } else { expect_eq(true, false); }

    // 9-byte JFR max u64 (0xFFFFFFFFFFFFFFFF): 8 bytes of 0xFF + 9th byte 0xFF (all 8 bits valid!)
    const v9max: [9]u8 = [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF];
    if (r := readJfrUleb128(v9max[0..9], 0)) {
        expect_eq(r.value, 0xFFFFFFFFFFFFFFFF);
        expect_eq(r.next_pos, 9);
    } else { expect_eq(true, false); }

    // 9-byte high bit only: [0x80*8, 0x80] => 1 << 63
    const v9hi: [9]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80];
    if (r := readJfrUleb128(v9hi[0..9], 0)) {
        expect_eq(r.value, 0x8000000000000000);
        expect_eq(r.next_pos, 9);
    } else { expect_eq(true, false); }
});

test("4. rejects truncated headers, wrong FLR magic, major=0, and out-of-bounds chunk_size", () => {
    let tiny: [32]u8 = undefined;
    zeroBytes(tiny[0..32]);
    expect_eq(isJfr(tiny[0..32]), false);
    expect_eq(parseJfrChunkHeader(tiny[0..32], 0) == null, true);

    let raw: [68]u8 = undefined;
    let buf = raw[0..68];
    zeroBytes(buf);
    writeChunkHeader(buf, 0, 68, 0, 0, 0, 0);

    // Bad magic
    buf[2] = 'X';
    expect_eq(isJfr(buf), false);
    buf[2] = 'R';

    // Major version 0
    writeU16BE(buf, 4, 0);
    expect_eq(isJfr(buf), false);
    writeU16BE(buf, 4, 2);

    // chunk_size < 68
    writeU64BE(buf, 8, 40);
    expect_eq(isJfr(buf), false);

    // chunk_size > buffer length
    writeU64BE(buf, 8, 500);
    expect_eq(isJfr(buf), false);
});

test("5. defensive bounds for malformed event frames (zero size, out-of-bounds size, truncated varint)", () => {
    let raw: [80]u8 = undefined;
    let buf = raw[0..80];
    zeroBytes(buf);

    // Case A: event size == 0
    writeChunkHeader(buf, 0, 72, 0, 0, 0, JFR_FLAG_COMPRESSED_INTS);
    buf[68] = 0;
    let eit0 = JfrEventIter.init(buf[0..72]);
    expect_eq(eit0.next() == null, true);
    expect_eq(eit0.had_error, true);
    expect_eq(summarizeJfr(buf[0..72]) == null, true);

    // Case B: event size exceeds chunk slice (claims 50 bytes, only 4 left)
    buf[68] = 50;
    buf[69] = 1;
    let eit1 = JfrEventIter.init(buf[0..72]);
    expect_eq(eit1.next() == null, true);
    expect_eq(eit1.had_error, true);

    // Case C: truncated ULEB128
    const trunc_var: [2]u8 = [0x80, 0x80];
    expect_eq(readJfrUleb128(trunc_var[0..2], 0) == null, true);
});
