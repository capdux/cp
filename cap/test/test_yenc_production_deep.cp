// Source: cp/cap/test/test_yenc_production_deep.cp (Production deep tests for cap:yenc)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    YENC_OFFSET,
    YENC_ESCAPE_OFFSET,
    YENC_ESCAPE_CHAR,
    YENC_DEFAULT_LINE_LEN,
    yencCrc32,
    yencEncode,
    yencDecode,
    YencBeginHeader,
    YencPartHeader,
    YencEndTrailer,
    parseYencBegin,
    parseYencPart,
    parseYencEnd,
} from "cap:yenc";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. All 256 byte values (0x00..0xFF) yencEncode/yencDecode exact roundtrip & critical escape verification", () => {
    expect_eq(YENC_OFFSET, 42 as u8);
    expect_eq(YENC_ESCAPE_OFFSET, 64 as u8);
    expect_eq(YENC_ESCAPE_CHAR, 0x3D as u8);
    expect_eq(YENC_DEFAULT_LINE_LEN, 128 as usize);
    expect_eq(yencCrc32("123456789"), 0xCBF43926 as u32);

    let all256: [256]u8 = std.mem.zeroes([256]u8);
    let i: usize = 0;
    while (i < 256) {
        all256[i] = i as u8;
        i += 1;
    }

    let enc: [600]u8 = std.mem.zeroes([600]u8);
    let dec_buf: [256]u8 = std.mem.zeroes([256]u8);

    const elen = yencEncode(all256[0..], 64, enc[0..]) ?? unreachable;
    // Verify no raw NUL (0x00) appears anywhere in encoded output
    let k: usize = 0;
    while (k < elen) {
        expect_eq(enc[k] != 0x00, true);
        k += 1;
    }

    const dlen = yencDecode(enc[0..elen], dec_buf[0..]) ?? unreachable;
    expect_eq(dlen, 256 as usize);
    expect_eq(bytesEq(dec_buf[0..], all256[0..]), true);

    // Explicitly test line-start '.' (src = 0x2E - 42 = 4) and line-start/end space & TAB
    // 0x2E - 42 = 4 -> encodes to '.' -> MUST be escaped at col 0 as "=n" (0x2E + 64 = 0x6E)
    // 0x20 - 42 mod 256 = 246 -> encodes to ' ' -> MUST be escaped at col 0 and EOL
    // 0x09 - 42 mod 256 = 223 -> encodes to '\t' -> MUST be escaped at col 0 and EOL
    const edge_bytes: [3]u8 = [4, 246, 223];
    let edge_enc: [16]u8 = std.mem.zeroes([16]u8);
    let edge_dec: [3]u8 = std.mem.zeroes([3]u8);
    const e_len = yencEncode(edge_bytes[0..], 128, edge_enc[0..]) ?? unreachable;
    expect_eq(edge_enc[0], YENC_ESCAPE_CHAR); // leading '.' escaped
    expect_eq(edge_enc[1], 0x6E as u8);       // '.' + 64 = 'n'
    const d_len = yencDecode(edge_enc[0..e_len], edge_dec[0..]) ?? unreachable;
    expect_eq(d_len, 3 as usize);
    expect_eq(bytesEq(edge_dec[0..], edge_bytes[0..]), true);
});

test("2. Single-part & multi-part =ybegin / =ypart / =yend header & trailer parsing (including spaces in filename)", () => {
    // Single-part =ybegin with spaces in filename & CRLF
    const b1 = parseYencBegin("=ybegin line=128 size=123456 name=My Vacation Photo 2026.jpg\r\n") ?? unreachable;
    expect_eq(b1.line, 128 as u32);
    expect_eq(b1.size, 123456 as u64);
    expect_eq(b1.isMultipart(), false);
    expect_eq(b1.part == null, true);
    expect_eq(b1.total == null, true);
    expect_eq(bytesEq(b1.name, "My Vacation Photo 2026.jpg"), true);

    // Multi-part =ybegin
    const b2 = parseYencBegin("=ybegin part=2 total=10 line=128 size=5000000 name=release v1.3.tar.gz\n") ?? unreachable;
    expect_eq(b2.isMultipart(), true);
    expect_eq(b2.part ?? unreachable, 2 as u32);
    expect_eq(b2.total ?? unreachable, 10 as u32);
    expect_eq(b2.size, 5000000 as u64);
    expect_eq(bytesEq(b2.name, "release v1.3.tar.gz"), true);

    // =ypart
    const p = parseYencPart("=ypart begin=11251 end=22500\r\n") ?? unreachable;
    expect_eq(p.begin, 11251 as u64);
    expect_eq(p.end, 22500 as u64);
    expect_eq(p.partSize(), 11250 as u64);

    // =yend with part, pcrc32 & crc32 (mixed-case hex)
    const e = parseYencEnd("=yend size=11250 part=2 pcrc32=DeAdBeEf crc32=01234567\r\n") ?? unreachable;
    expect_eq(e.size, 11250 as u64);
    expect_eq(e.part ?? unreachable, 2 as u32);
    expect_eq(e.pcrc32 ?? unreachable, 0xDEADBEEF as u32);
    expect_eq(e.crc32 ?? unreachable, 0x01234567 as u32);
});

test("3. Full yEnc message decode + IEEE CRC32 verification end-to-end", () => {
    const payload = "The quick brown fox jumps over the lazy dog.\x00\r\n=123456789";
    const expected_crc = yencCrc32(payload);

    let enc: [256]u8 = std.mem.zeroes([256]u8);
    const elen = yencEncode(payload, 0, enc[0..]) ?? unreachable; // 0 -> default 128

    const hdr = parseYencBegin("=ybegin line=128 size=57 name=fox.bin") ?? unreachable;
    expect_eq(hdr.size, payload.len as u64);

    let dec_buf: [64]u8 = std.mem.zeroes([64]u8);
    const dlen = yencDecode(enc[0..elen], dec_buf[0..]) ?? unreachable;
    expect_eq(dlen, payload.len);
    expect_eq(bytesEq(dec_buf[0..dlen], payload), true);
    expect_eq(yencCrc32(dec_buf[0..dlen]), expected_crc);
});

test("4. Decode error guards: dangling trailing '=', escaped CRLF & output buffer overflow", () => {
    let out: [4]u8 = std.mem.zeroes([4]u8);

    // Dangling trailing '='
    expect_eq(yencDecode("abc=", out[0..]) == null, true);
    expect_eq(yencDecode("=", out[0..]) == null, true);

    // '=' immediately followed by CR or LF is malformed
    expect_eq(yencDecode("=\r\nA", out[0..]) == null, true);
    expect_eq(yencDecode("=\nA", out[0..]) == null, true);

    // Output buffer overflow during decode & encode
    expect_eq(yencDecode("ABCDE", out[0..]) == null, true); // 5 bytes into 4B out
    expect_eq(yencEncode("ABCDEFGH", 128, out[0..]) == null, true);
    expect_eq(yencEncode("A", 1, out[0..]) == null, true); // line_len < 2 rejected
});

test("5. Malformed =ybegin / =ypart / =yend header & trailer rejection", () => {
    // Missing prefix or required fields
    expect_eq(parseYencBegin("ybegin line=128 size=10 name=a") == null, true);
    expect_eq(parseYencBegin("=ybegin line=128 size=10") == null, true); // missing name
    expect_eq(parseYencBegin("=ybegin line=0 size=10 name=a") == null, true); // line=0
    expect_eq(parseYencBegin("=ybegin line=128 name=a") == null, true); // missing size
    expect_eq(parseYencBegin("=ybegin line=128 size=10 name=") == null, true); // empty name

    // Invalid =ypart (begin=0 or end < begin or missing)
    expect_eq(parseYencPart("=ypart begin=0 end=10") == null, true);
    expect_eq(parseYencPart("=ypart begin=20 end=10") == null, true);
    expect_eq(parseYencPart("=ypart begin=1") == null, true);

    // Invalid =yend (missing size or bad hex crc32)
    expect_eq(parseYencEnd("=yend crc32=12345678") == null, true);
    expect_eq(parseYencEnd("=yend size=10 crc32=ZZZZZZZZ") == null, true);
    expect_eq(parseYencEnd("=yend size=10 part=0") == null, true);
});
