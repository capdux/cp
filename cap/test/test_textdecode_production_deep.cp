// Source: cp/cap/test/test_textdecode_production_deep.cp (Production deep tests for cap:textdecode)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    TextEncoding,
    sniffBom,
    TextDecoder,
} from "cap:textdecode";

test("1. UTF-8 streaming decode across arbitrary byte splits with BOM stripping & preservation", () => {
    // UTF-8 BOM (EF BB BF) + "中" (E4 B8 AD) + "😀" (F0 9F 98 80)
    const input: [10]u8 = [0xEF, 0xBB, 0xBF, 0xE4, 0xB8, 0xAD, 0xF0, 0x9F, 0x98, 0x80];
    expect_eq(sniffBom(input[0..]) ?? unreachable == TextEncoding.Utf8, true);

    // ignore_bom = false -> strips BOM even when split across chunks
    let td = TextDecoder.init(TextEncoding.Utf8, true, false);
    let out_arr: [32]u8 = std.mem.zeroes([32]u8);
    let out: []u8 = out_arr[0..];

    const r1 = td.decode(input[0..2], true, &out) ?? unreachable;
    expect_eq(r1.bytes_written, 0);
    expect_eq(td.hasPendingBytes(), true);

    const r2 = td.decode(input[2..5], true, &out) ?? unreachable; // completes BOM (stripped) + 2 bytes of "中"
    expect_eq(r2.bytes_written, 0);
    expect_eq(td.hasPendingBytes(), true);

    const r3 = td.decode(input[5..8], true, &out) ?? unreachable; // completes "中" (3B) + 2 bytes of "😀"
    expect_eq(r3.bytes_written, 3);
    expect_eq(out_arr[0] == 0xE4 && out_arr[1] == 0xB8 && out_arr[2] == 0xAD, true);

    const r4 = td.decode(input[8..10], false, &out) ?? unreachable; // completes "😀" (4B)
    expect_eq(r4.bytes_written, 4);
    expect_eq(r4.had_error, false);
    expect_eq(out_arr[0] == 0xF0 && out_arr[1] == 0x9F && out_arr[2] == 0x98 && out_arr[3] == 0x80, true);
    expect_eq(td.hasPendingBytes(), false);

    // ignore_bom = true -> keeps BOM bytes
    let dec_keep = TextDecoder.init(TextEncoding.Utf8, true, true);
    const rk = dec_keep.decode(input[0..], false, &out) ?? unreachable;
    expect_eq(rk.bytes_written, 10);
    expect_eq(out_arr[0] == 0xEF && out_arr[1] == 0xBB && out_arr[2] == 0xBF, true);
});

test("2. UTF-16LE & UTF-16BE streaming with odd-byte split, BOM stripping, and surrogate pair", () => {
    // UTF-16LE: BOM (FF FE) + 'A' (41 00) + 😀 (3D D8 00 DE)
    const le_bytes: [8]u8 = [0xFF, 0xFE, 0x41, 0x00, 0x3D, 0xD8, 0x00, 0xDE];
    expect_eq(sniffBom(le_bytes[0..]) ?? unreachable == TextEncoding.Utf16Le, true);

    let dec_le = TextDecoder.init(TextEncoding.Utf16Le, true, false);
    let out_arr: [32]u8 = std.mem.zeroes([32]u8);
    let out: []u8 = out_arr[0..];

    // Split at odd byte boundaries: [0..3], [3..5], [5..8]
    const r1 = dec_le.decode(le_bytes[0..3], true, &out) ?? unreachable;
    expect_eq(r1.bytes_written, 0);
    expect_eq(dec_le.hasPendingBytes(), true);

    const r2 = dec_le.decode(le_bytes[3..5], true, &out) ?? unreachable; // completes 'A' + 1 byte of lead surrogate
    expect_eq(r2.bytes_written, 1);
    expect_eq(out_arr[0], 'A');

    const r3 = dec_le.decode(le_bytes[5..8], false, &out) ?? unreachable; // completes 😀 -> F0 9F 98 80
    expect_eq(r3.bytes_written, 4);
    expect_eq(out_arr[0] == 0xF0 && out_arr[1] == 0x9F && out_arr[2] == 0x98 && out_arr[3] == 0x80, true);

    // UTF-16BE: BOM (FE FF) + 'B' (00 42) + 'Ω' (03 A9)
    const be_bytes: [6]u8 = [0xFE, 0xFF, 0x00, 0x42, 0x03, 0xA9];
    expect_eq(sniffBom(be_bytes[0..]) ?? unreachable == TextEncoding.Utf16Be, true);
    let dec_be = TextDecoder.init(TextEncoding.Utf16Be, true, false);
    const rb = dec_be.decode(be_bytes[0..], false, &out) ?? unreachable;
    expect_eq(rb.bytes_written, 3);
    expect_eq(out_arr[0] == 'B' && out_arr[1] == 0xCE && out_arr[2] == 0xA9, true);
});

test("3. WHATWG invalid UTF-8 handling: overlong, surrogate, >U+10FFFF in fatal vs replacement mode", () => {
    let out_arr: [32]u8 = std.mem.zeroes([32]u8);
    let out: []u8 = out_arr[0..];

    // Overlong '/' (C0 AF), UTF-16 surrogate U+D800 (ED A0 80), >U+10FFFF (F4 90 80 80)
    const overlong: [2]u8 = [0xC0, 0xAF];
    const surrogate: [3]u8 = [0xED, 0xA0, 0x80];
    const too_high: [4]u8 = [0xF4, 0x90, 0x80, 0x80];

    let dec_fatal = TextDecoder.init(TextEncoding.Utf8, true, false);
    expect_eq(dec_fatal.decode(overlong[0..], false, &out) == null, true);
    dec_fatal.reset();
    expect_eq(dec_fatal.decode(surrogate[0..], false, &out) == null, true);
    dec_fatal.reset();
    expect_eq(dec_fatal.decode(too_high[0..], false, &out) == null, true);

    // Non-fatal mode replaces invalid bytes with U+FFFD and preserves trailing ASCII
    let dec_rep = TextDecoder.init(TextEncoding.Utf8, false, false);
    const mixed: [3]u8 = [0xFF, 'X', 0xE4]; // invalid 0xFF, valid 'X', truncated 0xE4 at EOF
    const res = dec_rep.decode(mixed[0..], false, &out) ?? unreachable;
    expect_eq(res.had_error, true);
    expect_eq(res.bytes_written, 7); // U+FFFD (3) + 'X' (1) + U+FFFD (3)
    expect_eq(out_arr[0] == 0xEF && out_arr[1] == 0xBF && out_arr[2] == 0xBD, true);
    expect_eq(out_arr[3], 'X');
    expect_eq(out_arr[4] == 0xEF && out_arr[5] == 0xBF && out_arr[6] == 0xBD, true);
});

test("4. Latin-1 lossless decoding to canonical UTF-8", () => {
    let td = TextDecoder.init(TextEncoding.Latin1, true, false);
    let out_arr: [16]u8 = std.mem.zeroes([16]u8);
    let out: []u8 = out_arr[0..];
    const lat: [3]u8 = ['A', 0xE9, 0xFF]; // 'A', 'é' (C3 A9), 'ÿ' (C3 BF)
    const res = td.decode(lat[0..], false, &out) ?? unreachable;
    expect_eq(res.had_error, false);
    expect_eq(res.bytes_written, 5);
    expect_eq(out_arr[0] == 'A' && out_arr[1] == 0xC3 && out_arr[2] == 0xA9 && out_arr[3] == 0xC3 && out_arr[4] == 0xBF, true);
});

test("5. Defensive guards: truncated UTF-16 EOF, lone surrogates in fatal mode, and small output buffer", () => {
    let out_arr: [16]u8 = std.mem.zeroes([16]u8);
    let out: []u8 = out_arr[0..];
    let tiny: [2]u8 = std.mem.zeroes([2]u8);
    let tiny_s: []u8 = tiny[0..];

    // Odd-length UTF-16LE at EOF: fatal -> null, non-fatal -> U+FFFD
    const odd_le: [1]u8 = [0x41];
    let dec_f = TextDecoder.init(TextEncoding.Utf16Le, true, false);
    expect_eq(dec_f.decode(odd_le[0..], false, &out) == null, true);

    let dec_nf = TextDecoder.init(TextEncoding.Utf16Le, false, false);
    const r_odd = dec_nf.decode(odd_le[0..], false, &out) ?? unreachable;
    expect_eq(r_odd.had_error, true);
    expect_eq(r_odd.bytes_written, 3);

    // Lone trail surrogate in fatal UTF-16LE -> null
    const lone_trail: [2]u8 = [0x00, 0xDC];
    dec_f.reset();
    expect_eq(dec_f.decode(lone_trail[0..], false, &out) == null, true);

    // Output buffer too small -> null
    let dec_u8 = TextDecoder.init(TextEncoding.Utf8, false, false);
    expect_eq(dec_u8.decode("hello", false, &tiny_s) == null, true);
    expect_eq(sniffBom("hi") == null, true);
});
