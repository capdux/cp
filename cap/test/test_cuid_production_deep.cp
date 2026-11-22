// Source: cp/cap/test/test_cuid_production_deep.cp (Production deep tests for cap:cuid)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CuidVersion,
    isValidCuid1,
    isValidCuid2,
    detectCuidVersion,
    encodeBase36,
    decodeBase36,
    formatCuid1,
    parseCuid1Timestamp,
} from "cap:codec";

test("1. isValidCuid1, isValidCuid2 & detectCuidVersion classification", () => {
    const v1_id = "cjld2cjxh0000qzrmn831i7rn";
    expect_eq(isValidCuid1(v1_id), true);
    expect_eq(detectCuidVersion(v1_id) == CuidVersion.v1, true);

    // Canonical 24-char CUID2 and boundary lengths (2 and 32)
    const v2_24 = "tz4a98xxat96iws9zmbrgj3a";
    const v2_min = "a0";
    const v2_max = "z0123456789abcdefghijklmnopqrstu";
    expect_eq(isValidCuid1(v2_24), false);
    expect_eq(isValidCuid2(v2_24), true);
    expect_eq(isValidCuid2(v2_min), true);
    expect_eq(isValidCuid2(v2_max), true);
    expect_eq(detectCuidVersion(v2_24) == CuidVersion.v2, true);
    expect_eq(detectCuidVersion(v2_min) == CuidVersion.v2, true);
    expect_eq(detectCuidVersion(v2_max) == CuidVersion.v2, true);

    // 25-char starting with non-'c' letter is v2, not v1
    const v2_25 = "ajld2cjxh0000qzrmn831i7rn";
    expect_eq(isValidCuid1(v2_25), false);
    expect_eq(detectCuidVersion(v2_25) == CuidVersion.v2, true);
});

test("2. Reject invalid CUID1 and CUID2 inputs (length, case, prefix, symbols)", () => {
    expect_eq(isValidCuid1(""), false);
    expect_eq(isValidCuid2(""), false);
    expect_eq(isValidCuid2("a"), false); // length 1 < 2
    expect_eq(isValidCuid2("0abcd"), false); // starts with digit
    expect_eq(isValidCuid2("a0123456789abcdefghijklmnopqrstuv"), false); // length 33 > 32
    expect_eq(isValidCuid1("Cjld2cjxh0000qzrmn831i7rn"), false); // uppercase 'C'
    expect_eq(isValidCuid1("cjld2cjxh0000QZrmn831i7rn"), false); // uppercase body
    expect_eq(isValidCuid2("tz4a98xx_t96iws9zmbrgj3a"), false); // underscore
    expect_eq(detectCuidVersion("INVALID-ID") == CuidVersion.unknown, true);
    expect_eq(detectCuidVersion("9abc") == CuidVersion.unknown, true);
});

test("3. encodeBase36 & decodeBase36 round-trip, u64 max, and overflow protection", () => {
    let buf: [16]u8 = std.mem.zeroes([16]u8);

    // Zero
    const z = encodeBase36(0, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, z, "0"), true);
    expect_eq(decodeBase36("0") ?? 99, 0);

    // Known values
    const s35 = encodeBase36(35, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, s35, "z"), true);
    expect_eq(decodeBase36("z") ?? 0, 35);

    const s1296 = encodeBase36(1296, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, s1296, "100"), true);
    expect_eq(decodeBase36("100") ?? 0, 1296);

    // u64::MAX = 18446744073709551615 -> "3w5e11264sgsf"
    const max_u64: u64 = 18446744073709551615;
    const smax = encodeBase36(max_u64, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, smax, "3w5e11264sgsf"), true);
    expect_eq(decodeBase36(smax) ?? 0, max_u64);

    // Overflow (max + 1 -> "3w5e11264sgsg") and invalid chars
    expect_eq(decodeBase36("3w5e11264sgsg") == null, true);
    expect_eq(decodeBase36("zzzzzzzzzzzzzz") == null, true);
    expect_eq(decodeBase36("") == null, true);
    expect_eq(decodeBase36("ABC") == null, true);
    expect_eq(decodeBase36("12-34") == null, true);

    // Buffer too small
    expect_eq(encodeBase36(1296, buf[0..2]) == null, true);
    expect_eq(encodeBase36(0, buf[0..0]) == null, true);
});

test("4. formatCuid1 deterministic layout & parseCuid1Timestamp round-trip", () => {
    let buf: [32]u8 = std.mem.zeroes([32]u8);
    const ts: u64 = 1700000000000;
    const id = formatCuid1(ts, 42, "ab12", 123456789, buf[0..]) ?? unreachable;

    expect_eq(id.len as i64, 25);
    expect_eq(isValidCuid1(id), true);
    expect_eq(detectCuidVersion(id) == CuidVersion.v1, true);
    expect_eq(std.mem.eql(u8, id[13..17], "ab12"), true);
    expect_eq(parseCuid1Timestamp(id) ?? 0, ts);

    // Deterministic repeatability
    let buf2: [32]u8 = std.mem.zeroes([32]u8);
    const id2 = formatCuid1(ts, 42, "ab12", 123456789, buf2[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, id, id2), true);
});

test("5. formatCuid1 fingerprint edge cases, buffer boundary & parseCuid1Timestamp guard", () => {
    let buf: [25]u8 = std.mem.zeroes([25]u8);

    // Short base36 fingerprint left-padded with '0'
    const id_short = formatCuid1(1000, 1, "xy", 7, buf[0..]) ?? unreachable;
    expect_eq(isValidCuid1(id_short), true);
    expect_eq(std.mem.eql(u8, id_short[13..17], "00xy"), true);
    expect_eq(parseCuid1Timestamp(id_short) ?? 0, 1000);

    // Non-base36 hostname fingerprint deterministically hashed to valid base36
    let buf_host: [25]u8 = std.mem.zeroes([25]u8);
    const id_host = formatCuid1(2000, 99, "prod-node-01.local", 42, buf_host[0..]) ?? unreachable;
    expect_eq(isValidCuid1(id_host), true);
    expect_eq(parseCuid1Timestamp(id_host) ?? 0, 2000);

    // Buffer too small (24 bytes < 25)
    expect_eq(formatCuid1(1000, 1, "ab12", 7, buf[0..24]) == null, true);

    // Invalid CUID1 returns null timestamp
    expect_eq(parseCuid1Timestamp("tz4a98xxat96iws9zmbrgj3a") == null, true);
    expect_eq(parseCuid1Timestamp("not-a-cuid") == null, true);
});
