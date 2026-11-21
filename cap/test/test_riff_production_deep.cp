// Source: cp/cap/test/test_riff_production_deep.cp (Production deep tests for cap:riff)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    RIFF_HEADER_SIZE,
    RIFF_CHUNK_HEADER_SIZE,
    RiffHeader,
    RiffChunk,
    fourccEquals,
    parseRiffHeader,
    parseListType,
    RiffChunkIter,
    findRiffChunk,
} from "cap:riff";

fn writeTag(buf: []u8, off: usize, tag: []const u8) {
    buf[off] = tag[0];
    buf[off + 1] = tag[1];
    buf[off + 2] = tag[2];
    buf[off + 3] = tag[3];
}

fn writeU32LE(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU32BE(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn appendChunkLE(buf: []u8, pos: usize, id: []const u8, payload: []const u8, add_pad: bool): usize {
    let p = pos;
    writeTag(buf, p, id);
    p += 4;
    writeU32LE(buf, p, payload.len as u32);
    p += 4;
    let i: usize = 0;
    while (i < payload.len) {
        buf[p] = payload[i];
        p += 1;
        i += 1;
    }
    if (add_pad && (payload.len & 1) == 1) {
        buf[p] = 0;
        p += 1;
    }
    return p;
}

test("1. parseRiffHeader and fourccEquals validate RIFF/RIFX headers and reject malformed containers", () => {
    let buf: [32]u8 = std.mem.zeroes([32]u8);
    writeTag(buf[0..], 0, "RIFF");
    writeU32LE(buf[0..], 4, 24); // total = 32
    writeTag(buf[0..], 8, "WAVE");

    const h1 = parseRiffHeader(buf[0..]) ?? unreachable;
    expect_eq(h1.is_big_endian, false);
    expect_eq(h1.file_size, 24);
    expect_eq(h1.isForm("WAVE"), true);
    expect_eq(fourccEquals(h1.form_type, "WAVE"), true);
    expect_eq(fourccEquals(h1.form_type, "WAV"), false);

    // Big-endian RIFX
    writeTag(buf[0..], 0, "RIFX");
    writeU32BE(buf[0..], 4, 20);
    writeTag(buf[0..], 8, "WEBP");
    const h2 = parseRiffHeader(buf[0..]) ?? unreachable;
    expect_eq(h2.is_big_endian, true);
    expect_eq(h2.file_size, 20);
    expect_eq(h2.isForm("WEBP"), true);

    // Reject too short, bad magic, file_size < 4, and file_size > buffer / overflow
    expect_eq(parseRiffHeader(buf[0..11]) == null, true);

    writeTag(buf[0..], 0, "FORM");
    expect_eq(parseRiffHeader(buf[0..]) == null, true);

    writeTag(buf[0..], 0, "RIFF");
    writeU32LE(buf[0..], 4, 3);
    expect_eq(parseRiffHeader(buf[0..]) == null, true);

    writeU32LE(buf[0..], 4, 0xFFFF_FFFC);
    expect_eq(parseRiffHeader(buf[0..]) == null, true);
});

test("2. RiffChunkIter handles odd-sized chunk padding and zero-copy payload slicing", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let pos: usize = RIFF_HEADER_SIZE;

    // Odd-sized chunk (3 bytes + 1 pad byte)
    pos = appendChunkLE(buf[0..], pos, "JUNK", "abc", true);
    // Even-sized chunk (4 bytes)
    pos = appendChunkLE(buf[0..], pos, "fmt ", "PCM1", true);
    // Trailing odd-sized chunk without trailing pad byte (tolerant EOF handling)
    pos = appendChunkLE(buf[0..], pos, "data", "12345", false);

    writeTag(buf[0..], 0, "RIFF");
    writeU32LE(buf[0..], 4, (pos - 8) as u32);
    writeTag(buf[0..], 8, "WAVE");

    let it = RiffChunkIter.init(buf[0..pos]);

    const c0 = it.next() ?? unreachable;
    expect_eq(fourccEquals(c0.id, "JUNK"), true);
    expect_eq(c0.size, 3);
    expect_eq(std.mem.eql(u8, c0.data, "abc"), true);

    const c1 = it.next() ?? unreachable;
    expect_eq(fourccEquals(c1.id, "fmt "), true);
    expect_eq(c1.size, 4);
    expect_eq(std.mem.eql(u8, c1.data, "PCM1"), true);

    const c2 = it.next() ?? unreachable;
    expect_eq(fourccEquals(c2.id, "data"), true);
    expect_eq(c2.size, 5);
    expect_eq(std.mem.eql(u8, c2.data, "12345"), true);

    expect_eq(it.next() == null, true);
});

test("3. RiffChunkIter.initSubchunks and parseListType decode nested LIST/INFO subchunks", () => {
    let list_body: [64]u8 = std.mem.zeroes([64]u8);
    writeTag(list_body[0..], 0, "INFO");
    let lp: usize = 4;
    lp = appendChunkLE(list_body[0..], lp, "INAM", "Track 1", true); // 7 bytes + 1 pad
    lp = appendChunkLE(list_body[0..], lp, "IART", "CP", true); // 2 bytes

    let wav: [128]u8 = std.mem.zeroes([128]u8);
    let pos: usize = RIFF_HEADER_SIZE;
    pos = appendChunkLE(wav[0..], pos, "LIST", list_body[0..lp], true);
    writeTag(wav[0..], 0, "RIFF");
    writeU32LE(wav[0..], 4, (pos - 8) as u32);
    writeTag(wav[0..], 8, "WAVE");

    const list_chunk = findRiffChunk(wav[0..pos], "LIST") ?? unreachable;
    const ltype = parseListType(list_chunk.data) ?? unreachable;
    expect_eq(fourccEquals(ltype, "INFO"), true);

    let sub = RiffChunkIter.initSubchunks(list_chunk.data, false);
    const s0 = sub.next() ?? unreachable;
    expect_eq(fourccEquals(s0.id, "INAM"), true);
    expect_eq(std.mem.eql(u8, s0.data, "Track 1"), true);

    const s1 = sub.next() ?? unreachable;
    expect_eq(fourccEquals(s1.id, "IART"), true);
    expect_eq(std.mem.eql(u8, s1.data, "CP"), true);

    expect_eq(sub.next() == null, true);
});

test("4. Big-endian RIFX iteration and findRiffChunk FourCC lookup", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let pos: usize = RIFF_HEADER_SIZE;

    // Chunk 0: "VP8X", size=4 (BE), payload="ABCD"
    writeTag(buf[0..], pos, "VP8X"); pos += 4;
    writeU32BE(buf[0..], pos, 4); pos += 4;
    writeTag(buf[0..], pos, "ABCD"); pos += 4;

    // Chunk 1: "VP8 ", size=3 (BE), payload="xyz", pad=0
    writeTag(buf[0..], pos, "VP8 "); pos += 4;
    writeU32BE(buf[0..], pos, 3); pos += 4;
    buf[pos] = 'x'; buf[pos + 1] = 'y'; buf[pos + 2] = 'z'; buf[pos + 3] = 0;
    pos += 4;

    writeTag(buf[0..], 0, "RIFX");
    writeU32BE(buf[0..], 4, (pos - 8) as u32);
    writeTag(buf[0..], 8, "WEBP");

    const vp8 = findRiffChunk(buf[0..pos], "VP8 ") ?? unreachable;
    expect_eq(vp8.size, 3);
    expect_eq(std.mem.eql(u8, vp8.data, "xyz"), true);

    expect_eq(findRiffChunk(buf[0..pos], "ALPH") == null, true);
    expect_eq(findRiffChunk(buf[0..pos], "BAD") == null, true);
});

test("5. Overflowing chunk sizes and truncated payloads terminate safely without out-of-bounds reads", () => {
    let buf: [32]u8 = std.mem.zeroes([32]u8);
    writeTag(buf[0..], 0, "RIFF");
    writeU32LE(buf[0..], 4, 24);
    writeTag(buf[0..], 8, "AVI ");

    // Chunk at offset 12 claims size 0xFFFF_FFF8 (would wrap u32 if added without u64)
    writeTag(buf[0..], 12, "EVIL");
    writeU32LE(buf[0..], 16, 0xFFFF_FFF8);

    let it1 = RiffChunkIter.init(buf[0..]);
    expect_eq(it1.next() == null, true);

    // Chunk claims 100 bytes in a 32-byte buffer
    writeU32LE(buf[0..], 16, 100);
    let it2 = RiffChunkIter.init(buf[0..]);
    expect_eq(it2.next() == null, true);

    // Short list payload (<4 bytes) yields null cleanly
    expect_eq(parseListType(buf[0..3]) == null, true);
    let sub_empty = RiffChunkIter.initSubchunks(buf[0..3], false);
    expect_eq(sub_empty.next() == null, true);
});
