import { test, expect_eq } from "cap:test";
import {
    generateEtag, generateStatEtag, parseEtag, compareStrong, compareWeak
} from "../../packages/@cap/etag-http/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/etag-http — generateEtag strong & weak deterministic output", () => {
        let out1: [64]u8 = undefined;
        let out2: [64]u8 = undefined;
        let data: string = "hello world";
        let n1 = generateEtag(data, false, out1[0..64]);
        let n2 = generateEtag(data, true, out2[0..64]);
        expect_eq(out1[0], 34); // '"'
        expect_eq(out1[n1 - 1], 34);
        expect_eq(out2[0], 87); // 'W'
        expect_eq(out2[1], 47); // '/'
        expect_eq(n2, n1 + 2);
    });

    test("2. @cap/etag-http — Different payloads produce different ETags", () => {
        let a: [64]u8 = undefined;
        let b: [64]u8 = undefined;
        let na = generateEtag("payload-a", false, a[0..64]);
        let nb = generateEtag("payload-b", false, b[0..64]);
        expect_eq(eqBuf(a[0..na], na, b[0..nb] as string), false);
    });

    test("3. @cap/etag-http — generateStatEtag formats W/\"<hex_size>-<hex_mtime>\"", () => {
        let out: [64]u8 = undefined;
        let n = generateStatEtag(4096, 0x18f00, out[0..64]);
        expect_eq(eqBuf(out[0..n], n, "W/\"1000-18f00\""), true);
    });

    test("4. @cap/etag-http — parseEtag validates strong, weak & malformed tags", () => {
        let s = parseEtag("\"abc-123\"");
        expect_eq(s.valid, true);
        expect_eq(s.weak, false);
        expect_eq(s.opaque_tag, "abc-123");

        let w = parseEtag("W/\"xyz\"");
        expect_eq(w.valid, true);
        expect_eq(w.weak, true);
        expect_eq(w.opaque_tag, "xyz");

        expect_eq(parseEtag("w/\"xyz\"").valid, false); // lowercase 'w/' invalid per RFC 7232
        expect_eq(parseEtag("unquoted").valid, false);
        expect_eq(parseEtag("\"has space\"").valid, false);
    });

    test("5. @cap/etag-http — compareStrong vs compareWeak (RFC 7232)", () => {
        expect_eq(compareStrong("\"1\"", "\"1\""), true);
        expect_eq(compareStrong("W/\"1\"", "\"1\""), false);
        expect_eq(compareStrong("W/\"1\"", "W/\"1\""), false);
        expect_eq(compareWeak("\"1\"", "\"1\""), true);
        expect_eq(compareWeak("W/\"1\"", "\"1\""), true);
        expect_eq(compareWeak("W/\"1\"", "W/\"1\""), true);
        expect_eq(compareWeak("\"1\"", "\"2\""), false);
    });

    test("6. @cap/etag-http — Empty entity generates valid strong ETag", () => {
        let out: [64]u8 = undefined;
        let n = generateEtag("", false, out[0..64]);
        let p = parseEtag(out[0..n] as string);
        expect_eq(p.valid, true);
        expect_eq(p.weak, false);
        expect_eq(out[1], 48); // '0' hex len
    });
}
