// Source: cp/cap/test/test_lzss_production_deep.cp (Production deep tests for cap:lzss)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LZSS_WINDOW_SIZE,
    LZSS_MIN_MATCH,
    LZSS_MAX_MATCH,
    LZSS_FRAME_HEADER_SIZE,
    lzssMaxCompressedSize,
    lzssCompress,
    lzssDecompress,
    lzssCompressFramed,
    lzssDecompressFramed,
} from "cap:lzss";

test("1. Empty, short, and repetitive text compression ratio and exact round-trip", () => {
    expect_eq(LZSS_WINDOW_SIZE, 4096);
    expect_eq(LZSS_MIN_MATCH, 3);
    expect_eq(LZSS_MAX_MATCH, 18);
    expect_eq(LZSS_FRAME_HEADER_SIZE, 4);
    expect_eq(lzssMaxCompressedSize(0), 0);
    expect_eq(lzssMaxCompressedSize(8), 9);
    expect_eq(lzssMaxCompressedSize(9), 11);

    // Empty round-trip
    let comp: [512]u8 = [0; 512];
    let decomp: [512]u8 = [0; 512];
    expect_eq(lzssCompress("", comp[0..]) ?? 99, 0);
    expect_eq(lzssDecompress("", decomp[0..]) ?? 99, 0);

    // Short 2-byte input (cannot form 3-byte match)
    const c_short = lzssCompress("AB", comp[0..]) ?? unreachable;
    expect_eq(c_short, 3); // 1 flag byte + 2 literals
    const d_short = lzssDecompress(comp[0..c_short], decomp[0..]) ?? unreachable;
    expect_eq(d_short, 2);
    expect_eq(std.mem.eql(u8, decomp[0..d_short], "AB"), true);

    // Structured repetitive text achieves strong compression
    const text = "cap:lzss sliding window compression! cap:lzss sliding window compression! cap:lzss sliding window compression!";
    const c_len = lzssCompress(text, comp[0..]) ?? unreachable;
    expect_eq(c_len < text.len / 2, true);
    const d_len = lzssDecompress(comp[0..c_len], decomp[0..]) ?? unreachable;
    expect_eq(d_len, text.len);
    expect_eq(std.mem.eql(u8, decomp[0..d_len], text), true);
});

test("2. Highly repetitive data triggering overlapping RLE back-references (dist < len)", () => {
    // 200 identical 'A' bytes -> dist=1, len=18 overlapping RLE runs
    let runs: [200]u8 = [0x41; 200];
    let comp: [256]u8 = [0; 256];
    let decomp: [256]u8 = [0; 256];

    const c_len = lzssCompress(runs[0..], comp[0..]) ?? unreachable;
    expect_eq(c_len < 36, true); // 200 bytes compressed to ~27 bytes
    const d_len = lzssDecompress(comp[0..c_len], decomp[0..]) ?? unreachable;
    expect_eq(d_len, 200);
    expect_eq(std.mem.eql(u8, decomp[0..d_len], runs[0..]), true);

    // Repeating 3-byte pattern "XYZ" * 40 = 120 bytes -> dist=3, len=18 overlapping RLE
    let pat: [120]u8 = [0; 120];
    let i: usize = 0;
    while (i < 120) {
        pat[i] = (0x58 + (i % 3)) as u8; // 'X', 'Y', 'Z'
        i += 1;
    }
    const c_pat = lzssCompress(pat[0..], comp[0..]) ?? unreachable;
    expect_eq(c_pat < 30, true);
    const d_pat = lzssDecompress(comp[0..c_pat], decomp[0..]) ?? unreachable;
    expect_eq(d_pat, 120);
    expect_eq(std.mem.eql(u8, decomp[0..d_pat], pat[0..]), true);
});

test("3. Incompressible pseudo-random data worst-case bound and exact round-trip", () => {
    let raw: [256]u8 = [0; 256];
    let state: u32 = 0x13579BDF;
    let i: usize = 0;
    while (i < 256) {
        state = state * 1664525 + 1013904223;
        raw[i] = ((state >> 24) & 0xFF) as u8;
        i += 1;
    }

    const max_sz = lzssMaxCompressedSize(256);
    expect_eq(max_sz, 288);

    let comp: [300]u8 = [0; 300];
    let decomp: [256]u8 = [0; 256];

    const c_len = lzssCompress(raw[0..], comp[0..]) ?? unreachable;
    expect_eq(c_len <= max_sz, true);
    const d_len = lzssDecompress(comp[0..c_len], decomp[0..]) ?? unreachable;
    expect_eq(d_len, 256);
    expect_eq(std.mem.eql(u8, decomp[0..d_len], raw[0..]), true);
});

test("4. Framed 4-byte LE header round-trip and length mismatch rejection", () => {
    const payload = "framed-lzss-payload-framed-lzss-payload-0123456789";
    let comp: [128]u8 = [0; 128];
    let decomp: [128]u8 = [0; 128];

    const c_len = lzssCompressFramed(payload, comp[0..]) ?? unreachable;
    expect_eq(c_len > 4, true);
    const d_len = lzssDecompressFramed(comp[0..c_len], decomp[0..]) ?? unreachable;
    expect_eq(d_len, payload.len);
    expect_eq(std.mem.eql(u8, decomp[0..d_len], payload), true);

    // Empty framed round-trip
    const c_empty = lzssCompressFramed("", comp[0..]) ?? unreachable;
    expect_eq(c_empty, 4);
    expect_eq(lzssDecompressFramed(comp[0..4], decomp[0..]) ?? 99, 0);

    // Corrupted header length (declares larger than actual decompressed payload) -> rejected
    comp[0] = (payload.len + 5) as u8;
    expect_eq(lzssDecompressFramed(comp[0..c_len], decomp[0..]) == null, true);

    // Destination buffer smaller than declared header length -> rejected immediately
    let tiny_dst: [8]u8 = [0; 8];
    expect_eq(lzssDecompressFramed(comp[0..c_len], tiny_dst[0..]) == null, true);

    // Truncated frame (< 4 bytes) -> rejected
    expect_eq(lzssDecompressFramed(comp[0..3], decomp[0..]) == null, true);
});

test("5. Malformed stream & overflow rejection: dist > out_pos, truncated 2B token, dangling flag, dst overflow", () => {
    let dst: [32]u8 = [0; 32];

    // Illegal back-reference distance (dist=1 when out_pos=0): flags=0x00, token=(dist_m1=0, len_m3=0)
    const bad_dist: [3]u8 = [0x00, 0x00, 0x00];
    expect_eq(lzssDecompress(bad_dist[0..], dst[0..]) == null, true);

    // Illegal back-reference distance (dist=5 when out_pos=1): flags=0x01 (bit0=lit, bit1=match), 'A', (dist_m1=4, len_m3=0)
    const bad_dist2: [4]u8 = [0x01, 0x41, 0x04, 0x00];
    expect_eq(lzssDecompress(bad_dist2[0..], dst[0..]) == null, true);

    // Truncated 2-byte match token (only 1 byte after flags=0x00)
    const trunc_tok: [2]u8 = [0x00, 0x00];
    expect_eq(lzssDecompress(trunc_tok[0..], dst[0..]) == null, true);

    // Dangling flag byte with no tokens
    const dangling: [1]u8 = [0xFF];
    expect_eq(lzssDecompress(dangling[0..], dst[0..]) == null, true);

    // Output buffer overflow during decompression
    let small_dst: [2]u8 = [0; 2];
    const lit3: [4]u8 = [0x07, 0x41, 0x42, 0x43];
    expect_eq(lzssDecompress(lit3[0..], small_dst[0..]) == null, true);

    // Output buffer overflow during compression
    expect_eq(lzssCompress("ABCDEF", small_dst[0..]) == null, true);
});
