import std from "std";
import { test, expect_eq } from "cap:test";
import {
    isAsciiOnly,
    encodePunycode,
    decodePunycode,
    encodeIdnLabel,
    decodeIdnLabel,
} from "cap:codec";

test("1. RFC 3492 canonical vectors: German umlauts, Chinese, and Japanese", () => {
    let enc_buf: [128]u8 = std.mem.zeroes([128]u8);
    let dec_buf: [128]u8 = std.mem.zeroes([128]u8);

    // münchen <-> mnchen-3ya
    const e1 = encodePunycode("münchen", &enc_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e1, "mnchen-3ya"), true);
    const d1 = decodePunycode(e1, &dec_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, d1, "münchen"), true);

    // bücher <-> bcher-kva
    const e2 = encodePunycode("bücher", &enc_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e2, "bcher-kva"), true);
    const d2 = decodePunycode(e2, &dec_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, d2, "bücher"), true);

    // 中文 <-> fiq228c
    const e3 = encodePunycode("中文", &enc_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e3, "fiq228c"), true);
    const d3 = decodePunycode(e3, &dec_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, d3, "中文"), true);

    // 日本語 <-> wgv71a119e
    const e4 = encodePunycode("日本語", &enc_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e4, "wgv71a119e"), true);
    const d4 = decodePunycode(e4, &dec_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, d4, "日本語"), true);
});

test("2. 4-byte Unicode emoji round-trip and pure ASCII handling", () => {
    let enc_buf: [128]u8 = std.mem.zeroes([128]u8);
    let dec_buf: [128]u8 = std.mem.zeroes([128]u8);

    // 4 字节 UTF-8 码点（U+1F680 🚀）
    const rocket = "cp🚀";
    expect_eq(isAsciiOnly(rocket), false);
    const enc = encodePunycode(rocket, &enc_buf) ?? unreachable;
    expect_eq(isAsciiOnly(enc), true);
    const decoded_rocket = decodePunycode(enc, &dec_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, decoded_rocket, rocket), true);

    // 纯 ASCII 经 RFC 3492 编解码无损还原
    expect_eq(isAsciiOnly("example"), true);
    const asc_enc = encodePunycode("example", &enc_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, asc_enc, "example-"), true);
    const asc_dec = decodePunycode(asc_enc, &dec_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, asc_dec, "example"), true);
});

test("3. encodeIdnLabel and decodeIdnLabel handle ASCII passthrough and case-insensitive xn--", () => {
    let buf1: [128]u8 = std.mem.zeroes([128]u8);
    let buf2: [128]u8 = std.mem.zeroes([128]u8);

    // 纯 ASCII 原样透传
    const plain = encodeIdnLabel("api-v2", &buf1) ?? unreachable;
    expect_eq(std.mem.eql(u8, plain, "api-v2"), true);
    const plain_dec = decodeIdnLabel("api-v2", &buf2) ?? unreachable;
    expect_eq(std.mem.eql(u8, plain_dec, "api-v2"), true);

    // 非 ASCII 编码为 xn-- 前缀
    const ace = encodeIdnLabel("münchen", &buf1) ?? unreachable;
    expect_eq(std.mem.eql(u8, ace, "xn--mnchen-3ya"), true);

    // 大小写不敏感解码 XN--
    const decoded = decodeIdnLabel("XN--MNCHEN-3YA", &buf2) ?? unreachable;
    expect_eq(std.mem.eql(u8, decoded, "münchen"), true);
});

test("4. decodePunycode rejects overflow, truncated delta, invalid chars, and non-ASCII input", () => {
    let out: [128]u8 = std.mem.zeroes([128]u8);

    // 非 ASCII 字节
    expect_eq(decodePunycode("münchen", &out) == null, true);
    // 非法 Punycode 字符（'!'）
    expect_eq(decodePunycode("abc-!@#", &out) == null, true);
    // 截断的变长整数（'z' >= threshold 但无后续字节）
    expect_eq(decodePunycode("zzzz", &out) == null, true);
    // 极大溢出序列安全拒绝
    expect_eq(decodePunycode("99999999999999a", &out) == null, true);
});

test("5. buffer-too-small safety guards and invalid UTF-8 rejection", () => {
    let tiny: [3]u8 = std.mem.zeroes([3]u8);
    expect_eq(encodePunycode("münchen", &tiny) == null, true);
    expect_eq(decodePunycode("mnchen-3ya", &tiny) == null, true);
    expect_eq(encodeIdnLabel("münchen", &tiny) == null, true);
    expect_eq(decodeIdnLabel("xn--mnchen-3ya", &tiny) == null, true);

    // 非法 UTF-8 字节序列拒绝编码
    let out: [64]u8 = std.mem.zeroes([64]u8);
    const bad_utf8: [2]u8 = [0xFF, 0xFE];
    expect_eq(encodePunycode(&bad_utf8, &out) == null, true);
});
