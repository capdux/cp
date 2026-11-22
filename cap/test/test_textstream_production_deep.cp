// Source: cp/cap/test/test_textstream_production_deep.cp (Production deep tests for cap:textstream)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    UTF8_REPLACEMENT,
    isLeadSurrogate,
    isTrailSurrogate,
    combineSurrogatePair,
    encodeCodePointUtf8,
    utf8ByteLengthFromLatin1,
    utf8ByteLengthFromUtf16,
    TextEncoderStream,
} from "cap:textstream";

test("1. Cross-chunk UTF-16 surrogate pair reassembly (😀 U+1F600 split across chunks)", () => {
    let enc = TextEncoderStream.init();
    let buf: [32]u8 = std.mem.zeroes([32]u8);
    let out: []u8 = buf[0..];

    // Chunk 1 ends with high surrogate 0xD83D (first half of U+1F600 = F0 9F 98 80)
    const c1: [3]u16 = ['H', 'i', 0xD83D];
    expect_eq(utf8ByteLengthFromUtf16(c1[0..], false), 2);
    const n1 = enc.encodeUtf16(c1[0..], &out) ?? unreachable;
    expect_eq(n1, 2);
    expect_eq(std.mem.eql(u8, buf[0..2], "Hi"), true);
    expect_eq(enc.hasPendingSurrogate(), true);

    // Chunk 2 starts with low surrogate 0xDE00 -> emits 4-byte UTF-8 emoji + '!'
    const c2: [2]u16 = [0xDE00, '!'];
    expect_eq(utf8ByteLengthFromUtf16(c2[0..], true), 5);
    const n2 = enc.encodeUtf16(c2[0..], &out) ?? unreachable;
    expect_eq(n2, 5);
    expect_eq(enc.hasPendingSurrogate(), false);
    expect_eq(buf[0], 0xF0);
    expect_eq(buf[1], 0x9F);
    expect_eq(buf[2], 0x98);
    expect_eq(buf[3], 0x80);
    expect_eq(buf[4], '!');

    // Flush with no pending surrogate emits 0 bytes
    expect_eq(enc.flush(&out) ?? unreachable, 0);
});

test("2. Lone surrogates (intra-chunk, cross-chunk non-trail, and EOF flush) emit U+FFFD", () => {
    let enc = TextEncoderStream.init();
    let buf: [32]u8 = std.mem.zeroes([32]u8);
    let out: []u8 = buf[0..];

    // 2a. Lone lead followed by ASCII 'a', and lone trail followed by trailing lead
    const c1: [4]u16 = [0xD800, 'a', 0xDC00, 0xDBFF];
    const n1 = enc.encodeUtf16(c1[0..], &out) ?? unreachable;
    // U+FFFD (3) + 'a' (1) + U+FFFD (3) = 7 bytes; 0xDBFF held pending
    expect_eq(n1, 7);
    expect_eq(std.mem.eql(u8, buf[0..3], UTF8_REPLACEMENT[0..]), true);
    expect_eq(buf[3], 'a');
    expect_eq(std.mem.eql(u8, buf[4..7], UTF8_REPLACEMENT[0..]), true);
    expect_eq(enc.hasPendingSurrogate(), true);

    // 2b. EOF flush emits U+FFFD for trailing pending lead and clears state
    const nf = enc.flush(&out) ?? unreachable;
    expect_eq(nf, 3);
    expect_eq(std.mem.eql(u8, buf[0..3], UTF8_REPLACEMENT[0..]), true);
    expect_eq(enc.hasPendingSurrogate(), false);
    expect_eq(enc.flush(&out) ?? unreachable, 0);

    // 2c. Pending lead followed by non-trail in next UTF-16 chunk
    const lead_only: [1]u16 = [0xD83D];
    expect_eq(enc.encodeUtf16(lead_only[0..], &out) ?? unreachable, 0);
    expect_eq(enc.hasPendingSurrogate(), true);
    const next_ascii: [1]u16 = ['Z'];
    const n_after = enc.encodeUtf16(next_ascii[0..], &out) ?? unreachable;
    expect_eq(n_after, 4);
    expect_eq(std.mem.eql(u8, buf[0..3], UTF8_REPLACEMENT[0..]), true);
    expect_eq(buf[3], 'Z');
});

test("3. Latin-1 streaming conversion (ASCII + 0x80..0xFF) & pending surrogate handoff", () => {
    let enc = TextEncoderStream.init();
    let buf: [32]u8 = std.mem.zeroes([32]u8);
    let out: []u8 = buf[0..];

    // Hold a pending lead surrogate, then feed Latin-1 [0x41 ('A'), 0xE9 ('é' -> C3 A9), 0xFF ('ÿ' -> C3 BF)]
    const lead: [1]u16 = [0xD800];
    expect_eq(enc.encodeUtf16(lead[0..], &out) ?? unreachable, 0);
    expect_eq(enc.hasPendingSurrogate(), true);

    const lat: [3]u8 = [0x41, 0xE9, 0xFF];
    expect_eq(utf8ByteLengthFromLatin1(lat[0..]), 5);
    const n = enc.encodeLatin1(lat[0..], &out) ?? unreachable;
    // 3 (U+FFFD) + 1 ('A') + 2 ('é') + 2 ('ÿ') = 8 bytes
    expect_eq(n, 8);
    expect_eq(enc.hasPendingSurrogate(), false);
    expect_eq(std.mem.eql(u8, buf[0..3], UTF8_REPLACEMENT[0..]), true);
    expect_eq(buf[3], 0x41);
    expect_eq(buf[4], 0xC3);
    expect_eq(buf[5], 0xA9);
    expect_eq(buf[6], 0xC3);
    expect_eq(buf[7], 0xBF);
});

test("4. BMP multi-byte UTF-16 (2-byte Greek + 3-byte CJK) & inline surrogate pair", () => {
    let enc = TextEncoderStream.init();
    let buf: [32]u8 = std.mem.zeroes([32]u8);
    let out: []u8 = buf[0..];

    // U+03A9 (Ω -> CE A9), U+4E2D (中 -> E4 B8 AD), U+1F680 (🚀 = D83D DE80 -> F0 9F 9A 80)
    const seq: [4]u16 = [0x03A9, 0x4E2D, 0xD83D, 0xDE80];
    const n = enc.encodeUtf16(seq[0..], &out) ?? unreachable;
    expect_eq(n, 9);
    expect_eq(buf[0], 0xCE);
    expect_eq(buf[1], 0xA9);
    expect_eq(buf[2], 0xE4);
    expect_eq(buf[3], 0xB8);
    expect_eq(buf[4], 0xAD);
    expect_eq(buf[5], 0xF0);
    expect_eq(buf[6], 0x9F);
    expect_eq(buf[7], 0x9A);
    expect_eq(buf[8], 0x80);
});

test("5. Defensive guards: invalid code points, non-surrogate pairs, and atomic small-buffer rejection", () => {
    expect_eq(isLeadSurrogate(0xD800), true);
    expect_eq(isLeadSurrogate(0xDC00), false);
    expect_eq(isTrailSurrogate(0xDFFF), true);
    expect_eq(isTrailSurrogate(0xD800), false);
    expect_eq(combineSurrogatePair(0x0041, 0xDC00) == null, true);
    expect_eq(combineSurrogatePair(0xD83D, 0xDE00) ?? unreachable, 0x1F600);

    let tiny: [2]u8 = std.mem.zeroes([2]u8);
    let tiny_s: []u8 = tiny[0..];
    expect_eq(encodeCodePointUtf8(0xD800, &tiny_s) == null, true);
    expect_eq(encodeCodePointUtf8(0x110000, &tiny_s) == null, true);
    expect_eq(encodeCodePointUtf8(0x4E2D, &tiny_s) == null, true);

    // Atomic rejection on small output buffer preserves pending_lead_surrogate
    let enc = TextEncoderStream.init();
    const lead: [1]u16 = [0xD83D];
    expect_eq(enc.encodeUtf16(lead[0..], &tiny_s) ?? unreachable, 0);
    expect_eq(enc.hasPendingSurrogate(), true);
    expect_eq(enc.flush(&tiny_s) == null, true);
    expect_eq(enc.hasPendingSurrogate(), true);

    enc.reset();
    expect_eq(enc.hasPendingSurrogate(), false);
});
