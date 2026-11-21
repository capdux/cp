import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EncodedWordEncoding,
    parseEncodedWord,
    decodeEncodedWord,
    decodeHeaderValue,
    encodeUtf8Base64Word,
} from "cap:mail";

test("1. parseEncodedWord and decodeEncodedWord handle B and Q encodings", () => {
    const ew_b = parseEncodedWord("=?UTF-8?B?SGVsbG8gV29ybGQ=?=") ?? unreachable;
    expect_eq(std.mem.eql(u8, ew_b.charset, "UTF-8"), true);
    expect_eq(ew_b.encoding == EncodedWordEncoding.base64, true);

    let out: [128]u8 = std.mem.zeroes([128]u8);
    expect_eq(std.mem.eql(u8, decodeEncodedWord("=?UTF-8?B?SGVsbG8gV29ybGQ=?=", &out) ?? unreachable, "Hello World"), true);
    expect_eq(std.mem.eql(u8, decodeEncodedWord("=?utf-8?q?Hello_CP=21=20=E4=B8=AD=E6=96=87?=", &out) ?? unreachable, "Hello CP! 中文"), true);

    // 拒绝空白、空 charset、非法编码标志与内嵌 '?'
    expect_eq(parseEncodedWord("=?UTF-8?B?a b?=") == null, true);
    expect_eq(parseEncodedWord("=??B?abc?=") == null, true);
    expect_eq(parseEncodedWord("=?UTF-8?X?abc?=") == null, true);
    expect_eq(parseEncodedWord("=?UTF-8?Q?a?b?=") == null, true);
});

test("2. decodeHeaderValue folds whitespace between adjacent encoded-words per RFC 2047 section 6.2", () => {
    let out: [128]u8 = std.mem.zeroes([128]u8);
    const hdr = "Subject: =?UTF-8?Q?Hello_?=   \t =?UTF-8?B?V29ybGQ=?= from CP";
    const decoded = decodeHeaderValue(hdr, &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, decoded, "Subject: Hello World from CP"), true);
});

test("3. decodeHeaderValue preserves plain text and malformed encoded-words verbatim", () => {
    let out: [128]u8 = std.mem.zeroes([128]u8);
    const hdr = "plain =?UTF-8?B?not_valid_b64!?= =?bad?= end";
    const decoded = decodeHeaderValue(hdr, &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, decoded, hdr), true);
});

test("4. encodeUtf8Base64Word round-trips ASCII, Chinese, and 4-byte emoji across all mod-3 lengths", () => {
    let enc_buf: [128]u8 = std.mem.zeroes([128]u8);
    let dec_buf: [128]u8 = std.mem.zeroes([128]u8);

    const samples: [4][]const u8 = ["A", "AB", "ABC", "你好 CP 🚀"];
    for (s of samples) {
        const encoded = encodeUtf8Base64Word(s, &enc_buf) ?? unreachable;
        const decoded = decodeEncodedWord(encoded, &dec_buf) ?? unreachable;
        expect_eq(std.mem.eql(u8, decoded, s), true);
    }
});

test("5. buffer-too-small guards for encode and decode", () => {
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(encodeUtf8Base64Word("hello", &tiny) == null, true);
    expect_eq(decodeEncodedWord("=?UTF-8?B?SGVsbG8gV29ybGQ=?=", &tiny) == null, true);
    expect_eq(decodeHeaderValue("hello world", &tiny) == null, true);
});
