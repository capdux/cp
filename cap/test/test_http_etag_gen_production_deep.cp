import std from "std";
import { test, expect_eq } from "cap:test";
import {
    fnv1a64,
    generateWeakStatEtag,
    generateStrongHashEtag,
    generateWeakHashEtag,
    isValidEtagSyntax,
} from "cap:http";

test("1. fnv1a64 matches official 64-bit FNV-1a test vectors", () => {
    expect_eq(fnv1a64(""), u64(0xcbf29ce484222325));
    expect_eq(fnv1a64("a"), u64(0xaf63dc4c8601ec8c));
    expect_eq(fnv1a64("foobar"), u64(0x85944171f73967e8));

    // Distinct payloads yield distinct hashes
    expect_eq(fnv1a64("hello") != fnv1a64("hellp"), true);
});

test("2. generateWeakStatEtag formats hex metadata and handles zero / u64 max", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const e0 = generateWeakStatEtag(0, 0, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e0, "W/\"0-0\""), true);
    expect_eq(isValidEtagSyntax(e0), true);

    const e1 = generateWeakStatEtag(4096, 0x18bcecc7b00, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e1, "W/\"1000-18bcecc7b00\""), true);
    expect_eq(isValidEtagSyntax(e1), true);

    const emax = generateWeakStatEtag(0xffffffffffffffff, 0xdeadbeef, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, emax, "W/\"ffffffffffffffff-deadbeef\""), true);
    expect_eq(isValidEtagSyntax(emax), true);
});

test("3. generateStrongHashEtag and generateWeakHashEtag produce fixed 16-hex hash with length suffix", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const s_empty = generateStrongHashEtag("", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s_empty, "\"cbf29ce484222325-0\""), true);
    expect_eq(isValidEtagSyntax(s_empty), true);

    const s_foo = generateStrongHashEtag("foobar", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s_foo, "\"85944171f73967e8-6\""), true);
    expect_eq(isValidEtagSyntax(s_foo), true);

    const w_foo = generateWeakHashEtag("foobar", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, w_foo, "W/\"85944171f73967e8-6\""), true);
    expect_eq(isValidEtagSyntax(w_foo), true);
});

test("4. isValidEtagSyntax enforces RFC 9110 §8.8.3 weak indicator and etagc byte classes", () => {
    // Valid strong and weak ETags (including empty opaque-tag and obs-text >= 0x80)
    expect_eq(isValidEtagSyntax("\"\""), true);
    expect_eq(isValidEtagSyntax("W/\"\""), true);
    expect_eq(isValidEtagSyntax("\"!#~abc-123\""), true);
    expect_eq(isValidEtagSyntax("W/\"abc\""), true);
    const obs_etag: [4]u8 = ['"', 0x80, 0xFF, '"'];
    expect_eq(isValidEtagSyntax(obs_etag[0..4]), true);

    // Invalid syntax: lowercase w/, unquoted, embedded quote, space, control chars, DEL
    expect_eq(isValidEtagSyntax(""), false);
    expect_eq(isValidEtagSyntax("\""), false);
    expect_eq(isValidEtagSyntax("abc"), false);
    expect_eq(isValidEtagSyntax("w/\"abc\""), false);
    expect_eq(isValidEtagSyntax("W/abc"), false);
    expect_eq(isValidEtagSyntax("\"a\"b\""), false);
    expect_eq(isValidEtagSyntax("\"has space\""), false);
    expect_eq(isValidEtagSyntax("\"ctrl\nchar\""), false);
    const del_etag: [3]u8 = ['"', 0x7F, '"'];
    expect_eq(isValidEtagSyntax(del_etag[0..3]), false);
});

test("5. Tiny output buffers fail cleanly with null without panic", () => {
    let tiny4: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(generateWeakStatEtag(0, 0, &tiny4) == null, true);

    // Exact fit for W/"0-0" is 7 bytes
    let fit7: [7]u8 = std.mem.zeroes([7]u8);
    const exact = generateWeakStatEtag(0, 0, &fit7) ?? unreachable;
    expect_eq(std.mem.eql(u8, exact, "W/\"0-0\""), true);

    let tiny19: [19]u8 = std.mem.zeroes([19]u8);
    expect_eq(generateStrongHashEtag("foobar", &tiny19) == null, true);

    // Exact fit for "85944171f73967e8-6" is 20 bytes
    let fit20: [20]u8 = std.mem.zeroes([20]u8);
    const s20 = generateStrongHashEtag("foobar", &fit20) ?? unreachable;
    expect_eq(std.mem.eql(u8, s20, "\"85944171f73967e8-6\""), true);

    // Weak hash ETag requires 22 bytes for "foobar"
    let tiny21: [21]u8 = std.mem.zeroes([21]u8);
    expect_eq(generateWeakHashEtag("foobar", &tiny21) == null, true);
});
