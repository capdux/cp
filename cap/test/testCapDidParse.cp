// cap/test/testCapDidParse.cp
// Comprehensive unit tests for @cap/did-parse (W3C DID Core 1.0)

import { test, expect_eq } from "cap:test";
import {
    DidUrl,
    isValidMethod,
    isValidMethodId,
    parseDid,
    formatDid,
} from "../../packages/did-resolver/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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

fn main() {
    test("1. @cap/did-parse — did:web with multi-colon method-specific ID", () => {
        const raw = "did:web:example.com:user:alice";
        const d: DidUrl = parseDid(raw);
        expect_eq(d.valid, true);
        expect_eq(sliceEq(d.did, raw), true);
        expect_eq(sliceEq(d.method, "web"), true);
        expect_eq(sliceEq(d.id, "example.com:user:alice"), true);
        expect_eq(d.path.len, 0 as usize);
        expect_eq(d.query.len, 0 as usize);
        expect_eq(d.fragment.len, 0 as usize);
    });

    test("2. @cap/did-parse — did:key with #fragment", () => {
        const raw = "did:key:z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK#key-1";
        const d: DidUrl = parseDid(raw);
        expect_eq(d.valid, true);
        expect_eq(sliceEq(d.method, "key"), true);
        expect_eq(sliceEq(d.did, "did:key:z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK"), true);
        expect_eq(sliceEq(d.fragment, "key-1"), true);
    });

    test("3. @cap/did-parse — full DID URL with /path, ?query & #fragment", () => {
        const raw = "did:example:123456/path/to/res?service=agent&v=1#auth-2";
        const d: DidUrl = parseDid(raw);
        expect_eq(d.valid, true);
        expect_eq(sliceEq(d.did, "did:example:123456"), true);
        expect_eq(sliceEq(d.method, "example"), true);
        expect_eq(sliceEq(d.id, "123456"), true);
        expect_eq(sliceEq(d.path, "/path/to/res"), true);
        expect_eq(sliceEq(d.query, "service=agent&v=1"), true);
        expect_eq(sliceEq(d.fragment, "auth-2"), true);
    });

    test("4. @cap/did-parse — formatDid round-trip with & without fragment", () => {
        let buf: [128]u8 = undefined;
        const n1: usize = formatDid("web", "example.com%3A8080:alice", "", buf[0..128]);
        expect_eq(sliceEq(buf[0..n1], "did:web:example.com%3A8080:alice"), true);
        expect_eq(parseDid(buf[0..n1]).valid, true);

        const n2: usize = formatDid("key", "z6MkTest", "keys-1", buf[0..128]);
        expect_eq(sliceEq(buf[0..n2], "did:key:z6MkTest#keys-1"), true);
        const parsed: DidUrl = parseDid(buf[0..n2]);
        expect_eq(parsed.valid, true);
        expect_eq(sliceEq(parsed.fragment, "keys-1"), true);
    });

    test("5. @cap/did-parse — rejects uppercase DID:/method, trailing colon & empty id", () => {
        expect_eq(parseDid("DID:web:example.com").valid, false);
        expect_eq(parseDid("did:WEB:example.com").valid, false);
        expect_eq(parseDid("did:web:example.com:").valid, false);
        expect_eq(parseDid("did:web:").valid, false);
        expect_eq(parseDid("did::123").valid, false);
        expect_eq(parseDid("did:web:bad%ZZid").valid, false);

        expect_eq(isValidMethod("web"), true);
        expect_eq(isValidMethod("ion2"), true);
        expect_eq(isValidMethod("Web"), false);
        expect_eq(isValidMethod("a-b"), false);
        expect_eq(isValidMethodId("a:b_c-d.e%20f"), true);
        expect_eq(isValidMethodId("bad:"), false);
    });

    test("6. @cap/did-parse — tiny buffer protection", () => {
        let tiny: [6]u8 = undefined;
        expect_eq(formatDid("web", "example.com", "k1", tiny[0..6]), 0 as usize);
        expect_eq(formatDid("BAD", "example.com", "", tiny[0..6]), 0 as usize);
    });
}
