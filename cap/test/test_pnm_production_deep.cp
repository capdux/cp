// Source: cp/cap/test/test_pnm_production_deep.cp (Production deep tests for cap:pnm)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PnmFormat,
    PnmHeader,
    parsePnmHeader,
    pnmRawPixels,
    encodePpmP6,
    encodePgmP5,
} from "cap:pnm";

test("1. parsePnmHeader parses P1..P6 with # comments, 16-bit maxval, and computes expectedRawBytes", () => {
    const p1 = parsePnmHeader("P1\n# ascii bitmap\n4 2\n0 1 0 1\n1 0 1 0\n") ?? unreachable;
    expect_eq(p1.format == PnmFormat.p1_pbm_ascii, true);
    expect_eq(p1.width, 4);
    expect_eq(p1.height, 2);
    expect_eq(p1.maxval, 1);
    expect_eq(p1.channels(), 1);
    expect_eq(p1.isBinary(), false);
    expect_eq(p1.expectedRawBytes() == null, true);

    const p2 = parsePnmHeader("P2 # gray\n3 2\n# max\n255\n0 128 255 1 2 3") ?? unreachable;
    expect_eq(p2.format == PnmFormat.p2_pgm_ascii, true);
    expect_eq(p2.channels(), 1);
    expect_eq(p2.isBinary(), false);

    const p3 = parsePnmHeader("P3\n2 2\n65535\n0 0 0") ?? unreachable;
    expect_eq(p3.format == PnmFormat.p3_ppm_ascii, true);
    expect_eq(p3.maxval, 65535);
    expect_eq(p3.channels(), 3);
    expect_eq(p3.isBinary(), false);

    // P4 raw bitmap: width=10 -> ceil(10/8)=2 bytes/row * 3 rows = 6 bytes
    const p4 = parsePnmHeader("P4\n10 3\n123456") ?? unreachable;
    expect_eq(p4.format == PnmFormat.p4_pbm_raw, true);
    expect_eq(p4.maxval, 1);
    expect_eq(p4.isBinary(), true);
    expect_eq(p4.expectedRawBytes() ?? 0, 6);

    // P5 16-bit raw gray: 4*3 * 1ch * 2bytes = 24 bytes
    const p5 = parsePnmHeader("P5\n4 3\n65535\n") ?? unreachable;
    expect_eq(p5.format == PnmFormat.p5_pgm_raw, true);
    expect_eq(p5.maxval, 65535);
    expect_eq(p5.expectedRawBytes() ?? 0, 24);

    // P6 8-bit raw rgb: 4*3 * 3ch * 1byte = 36 bytes
    const p6 = parsePnmHeader("P6\n4 3\n255\n") ?? unreachable;
    expect_eq(p6.format == PnmFormat.p6_ppm_raw, true);
    expect_eq(p6.channels(), 3);
    expect_eq(p6.expectedRawBytes() ?? 0, 36);
});

test("2. encodePpmP6 -> parsePnmHeader -> pnmRawPixels lossless roundtrip preserves leading whitespace pixel bytes", () => {
    // First pixel bytes are literally ' ' (0x20), '\n' (0x0A), '\t' (0x09)!
    const rgb: [12]u8 = [
        0x20, 0x0A, 0x09,
        0xFF, 0x00, 0x80,
        0x11, 0x22, 0x33,
        0xAA, 0xBB, 0xCC,
    ];
    let out: [64]u8 = std.mem.zeroes([64]u8);
    const encoded = encodePpmP6(rgb[0..], 2, 2, out[0..]) ?? unreachable;

    // Header must be "P6\n2 2\n255\n" (11 bytes) + 12 bytes payload = 23 bytes
    expect_eq(encoded.len, 23);
    expect_eq(std.mem.eql(u8, encoded[0..11], "P6\n2 2\n255\n"), true);

    const hdr = parsePnmHeader(encoded) ?? unreachable;
    expect_eq(hdr.format == PnmFormat.p6_ppm_raw, true);
    expect_eq(hdr.width, 2);
    expect_eq(hdr.height, 2);
    expect_eq(hdr.data_offset, 11);

    const pixels = pnmRawPixels(encoded) ?? unreachable;
    expect_eq(pixels.len, 12);
    expect_eq(std.mem.eql(u8, pixels, rgb[0..]), true);
});

test("3. encodePgmP5 roundtrip and exact buffer sizing bounds", () => {
    const gray: [6]u8 = [0x0A, 0x20, 0x7F, 0x80, 0xFE, 0xFF];
    let out: [32]u8 = std.mem.zeroes([32]u8);
    const encoded = encodePgmP5(gray[0..], 3, 2, out[0..]) ?? unreachable;

    // "P5\n3 2\n255\n" (11 bytes) + 6 bytes = 17 bytes
    expect_eq(encoded.len, 17);
    expect_eq(std.mem.eql(u8, pnmRawPixels(encoded) ?? "", gray[0..]), true);

    // Exact 17-byte buffer succeeds
    let exact: [17]u8 = std.mem.zeroes([17]u8);
    const ok = encodePgmP5(gray[0..], 3, 2, exact[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, ok, encoded), true);

    // 1 byte short -> null
    expect_eq(encodePgmP5(gray[0..], 3, 2, exact[0..16]) == null, true);
});

test("4. pnmRawPixels rejects ASCII/P4/truncated payloads and trims trailing extra bytes", () => {
    // Reject ASCII P3 and raw P4
    expect_eq(pnmRawPixels("P3\n1 1\n255\n10 20 30\n") == null, true);
    expect_eq(pnmRawPixels("P4\n8 1\n\xFF") == null, true);

    // Truncated P6 (needs 6 bytes, only 4 provided)
    expect_eq(pnmRawPixels("P6\n2 1\n255\nABCD") == null, true);

    // P5 with extra trailing bytes -> returns exact expected slice (3 bytes)
    const p5_extra: []const u8 = "P5\n3 1\n255\nXYZ_TRAILING";
    const px = pnmRawPixels(p5_extra) ?? unreachable;
    expect_eq(std.mem.eql(u8, px, "XYZ"), true);
});

test("5. Malformed header and invalid encode parameter protection", () => {
    expect_eq(parsePnmHeader("") == null, true);
    expect_eq(parsePnmHeader("P0\n1 1\n") == null, true);
    expect_eq(parsePnmHeader("P7\n1 1\n") == null, true);
    expect_eq(parsePnmHeader("P6\n0 2\n255\n") == null, true);
    expect_eq(parsePnmHeader("P6\n2 0\n255\n") == null, true);
    expect_eq(parsePnmHeader("P6\n2 2\n0\n") == null, true);
    expect_eq(parsePnmHeader("P6\n2 2\n65536\n") == null, true);
    expect_eq(parsePnmHeader("P6\n2 2\n255") == null, true); // missing trailing single whitespace

    // Mismatched pixel length or zero dimension on encode
    const dummy: [6]u8 = [1, 2, 3, 4, 5, 6];
    let out: [32]u8 = std.mem.zeroes([32]u8);
    expect_eq(encodePpmP6(dummy[0..], 2, 2, out[0..]) == null, true);
    expect_eq(encodePgmP5(dummy[0..], 0, 2, out[0..]) == null, true);
});
