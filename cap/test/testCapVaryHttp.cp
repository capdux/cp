import { test, expect_eq } from "cap:test";
import {
    appendVary, hasVaryField, countVaryFields, isValidFieldName
} from "../../packages/@cap/vary-http/src/index.cp";

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
    test("1. @cap/vary-http — Append to empty Vary header", () => {
        let out: [128]u8 = undefined;
        let n = appendVary("", "Accept-Encoding", out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "Accept-Encoding"), true);
    });

    test("2. @cap/vary-http — Case-insensitive deduplication via @cap/unicase", () => {
        let out: [128]u8 = undefined;
        let n = appendVary("Accept-Encoding, Origin", "origin, User-Agent, ACCEPT-ENCODING", out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "Accept-Encoding, Origin, User-Agent"), true);
        expect_eq(countVaryFields(out[0..n] as string), 3);
    });

    test("3. @cap/vary-http — Wildcard '*' absorbs all other Vary fields", () => {
        let out: [64]u8 = undefined;
        let n1 = appendVary("Origin, Accept", "*", out[0..64]);
        expect_eq(eqBuf(out[0..n1], n1, "*"), true);
        let n2 = appendVary("*", "Origin", out[0..64]);
        expect_eq(eqBuf(out[0..n2], n2, "*"), true);
    });

    test("4. @cap/vary-http — hasVaryField case-insensitive & wildcard check", () => {
        expect_eq(hasVaryField("Accept-Encoding, Origin", "origin"), true);
        expect_eq(hasVaryField("Accept-Encoding, Origin", "Cookie"), false);
        expect_eq(hasVaryField("*", "Anything"), true);
        expect_eq(hasVaryField("", "Origin"), false);
    });

    test("5. @cap/vary-http — isValidFieldName rejects invalid RFC 7230 tokens", () => {
        expect_eq(isValidFieldName("Accept-Encoding"), true);
        expect_eq(isValidFieldName("X_Custom-1"), true);
        expect_eq(isValidFieldName("*"), true);
        expect_eq(isValidFieldName(""), false);
        expect_eq(isValidFieldName("Bad Header"), false);
        expect_eq(isValidFieldName("Bad:Header"), false);
    });

    test("6. @cap/vary-http — Ignores empty/invalid tokens gracefully", () => {
        let out: [128]u8 = undefined;
        let n = appendVary(" , Origin , ", " , Bad Header , Accept ", out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "Origin, Accept"), true);
    });
}
