// Source: cp/cap/test/test_uuencode_production_deep.cp (Production deep tests for cap:uuencode)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    UU_MAX_LINE_BYTES,
    UU_DEFAULT_MODE,
    XX_ALPHABET,
    UuHeader,
    UuDecoded,
    parseUuHeader,
    uuEncodeRaw,
    uuDecodeRaw,
    uuEncode,
    uuDecode,
    xxEncode,
    xxDecode,
} from "cap:codec";

fn slicesEq(a: []const u8, b: []const u8): bool {
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

test("1. Standard POSIX uuencode vector ('Cat' -> 'begin 644 cat.txt\\n#0V%T\\n`\\nend\\n') and parseUuHeader", () => {
    expect_eq(UU_MAX_LINE_BYTES, 45 as usize);
    expect_eq(UU_DEFAULT_MODE, 0o644 as u16);
    expect_eq(XX_ALPHABET.len, 64 as usize);

    let enc_arr: [128]u8 = std.mem.zeroes([128]u8);
    let enc: []u8 = enc_arr[0..];
    let out_arr: [64]u8 = std.mem.zeroes([64]u8);
    let out: []u8 = out_arr[0..];

    const expected = "begin 644 cat.txt\n#0V%T\n`\nend\n";
    if (n := uuEncode("Cat", UU_DEFAULT_MODE, "cat.txt", *enc)) {
        expect_eq(slicesEq(enc_arr[0..n], expected), true);

        if (hdr := parseUuHeader(enc_arr[0..n])) {
            expect_eq(hdr.mode, 0o644 as u16);
            expect_eq(slicesEq(hdr.filename, "cat.txt"), true);
            expect_eq(hdr.body_offset, 18 as usize);
        } else {
            expect_eq(true, false);
        }

        if (res := uuDecode(enc_arr[0..n], *out)) {
            expect_eq(res.mode, 0o644 as u16);
            expect_eq(slicesEq(res.filename, "cat.txt"), true);
            expect_eq(res.len, 3 as usize);
            expect_eq(slicesEq(out_arr[0..res.len], "Cat"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("2. Multi-line 45B boundary and full 256-byte alphabet uuEncode/uuDecode + uuEncodeRaw/uuDecodeRaw", () => {
    let all256: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        all256[i] = i as u8;
        i += 1;
    }

    let enc_arr: [512]u8 = std.mem.zeroes([512]u8);
    let enc: []u8 = enc_arr[0..];
    let out_arr: [256]u8 = std.mem.zeroes([256]u8);
    let out: []u8 = out_arr[0..];

    // Full wrapped uuencode (256 bytes = 5 * 45B lines + 1 * 31B line)
    if (n := uuEncode(all256[0..], 0o755, "all256.bin", *enc)) {
        if (res := uuDecode(enc_arr[0..n], *out)) {
            expect_eq(res.mode, 0o755 as u16);
            expect_eq(slicesEq(res.filename, "all256.bin"), true);
            expect_eq(res.len, 256 as usize);
            expect_eq(slicesEq(out_arr[0..256], all256[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Raw body-only round-trip
    if (rn := uuEncodeRaw(all256[0..], *enc)) {
        if (dn := uuDecodeRaw(enc_arr[0..rn], *out)) {
            expect_eq(dn, 256 as usize);
            expect_eq(slicesEq(out_arr[0..256], all256[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. Historical space (0x20) and modern backtick ('`') zero-encoding compatibility + CRLF support", () => {
    // 3 zero bytes: historical uuencode uses 0x20 spaces ("#    \r\n \r\nend\r\n")
    const historic = "begin 600 zeros.dat\r\n#    \r\n \r\nend\r\n";
    let out_arr: [16]u8 = std.mem.zeroes([16]u8);
    let out: []u8 = out_arr[0..];
    out_arr[0] = 0xFF;
    out_arr[1] = 0xFF;
    out_arr[2] = 0xFF;

    if (res := uuDecode(historic, *out)) {
        expect_eq(res.mode, 0o600 as u16);
        expect_eq(slicesEq(res.filename, "zeros.dat"), true);
        expect_eq(res.len, 3 as usize);
        expect_eq(out_arr[0], 0 as u8);
        expect_eq(out_arr[1], 0 as u8);
        expect_eq(out_arr[2], 0 as u8);
    } else {
        expect_eq(true, false);
    }
});

test("4. xxEncode / xxDecode full round-trip (64-char +-0-9A-Za-z alphabet, empty + 100-byte binary)", () => {
    let enc_arr: [256]u8 = std.mem.zeroes([256]u8);
    let enc: []u8 = enc_arr[0..];
    let out_arr: [128]u8 = std.mem.zeroes([128]u8);
    let out: []u8 = out_arr[0..];

    // 4a. Empty payload
    if (n := xxEncode("", 0o644, "empty.xx", *enc)) {
        expect_eq(slicesEq(enc_arr[0..n], "begin 644 empty.xx\n+\nend\n"), true);
        if (res := xxDecode(enc_arr[0..n], *out)) {
            expect_eq(res.len, 0 as usize);
            expect_eq(slicesEq(res.filename, "empty.xx"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 4b. 100-byte binary payload crossing 45B lines
    let bin: [100]u8 = undefined;
    let i: usize = 0;
    while (i < 100) {
        bin[i] = ((i * 37 + 13) & 0xFF) as u8;
        i += 1;
    }
    if (n := xxEncode(bin[0..], 0o700, "bin100.xx", *enc)) {
        if (res := xxDecode(enc_arr[0..n], *out)) {
            expect_eq(res.mode, 0o700 as u16);
            expect_eq(slicesEq(res.filename, "bin100.xx"), true);
            expect_eq(res.len, 100 as usize);
            expect_eq(slicesEq(out_arr[0..100], bin[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. Malformed input rejection: missing begin/end, invalid octal mode, truncated line, output overflow", () => {
    let out_arr: [32]u8 = std.mem.zeroes([32]u8);
    let out: []u8 = out_arr[0..];

    // 5a. Missing begin
    if (_ := uuDecode("#0V%T\n`\nend\n", *out)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. Missing end
    if (_ := uuDecode("begin 644 cat.txt\n#0V%T\n`\n", *out)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. Invalid octal digit ('8') in mode
    if (_ := parseUuHeader("begin 684 cat.txt\n")) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5d. Truncated data line (prefix '#' = 3 bytes needs 4 chars, only 2 present)
    if (_ := uuDecode("begin 644 cat.txt\n#0V\n`\nend\n", *out)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5e. Output buffer overflow on encode and decode
    let tiny_arr: [2]u8 = std.mem.zeroes([2]u8);
    let tiny: []u8 = tiny_arr[0..];
    if (_ := uuEncode("Cat", 0o644, "cat.txt", *tiny)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
    if (_ := uuDecode("begin 644 cat.txt\n#0V%T\n`\nend\n", *tiny)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});
