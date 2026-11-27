import { test, expect_eq } from "cap:test";
import {
    isValidNsid,
    isValidRkey,
    parseAtUri,
    formatAtUri,
} from "../../packages/@atproto/syntax/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/at-uri — DID full 3-segment at:// URI parsing", () => {
        const u = parseAtUri("at://did:plc:z72i7hdynmk6r22z27h6tvur/app.bsky.feed.post/3k2yihcrp6f2c");
        expect_eq(u.valid, true);
        expect_eq(u.is_did, true);
        expect_eq(testEq(u.authority, "did:plc:z72i7hdynmk6r22z27h6tvur"), true);
        expect_eq(testEq(u.collection, "app.bsky.feed.post"), true);
        expect_eq(testEq(u.rkey, "3k2yihcrp6f2c"), true);
    });

    test("2. @cap/at-uri — Handle full 3-segment at:// URI parsing", () => {
        const u = parseAtUri("at://alice.bsky.social/app.bsky.actor.profile/self");
        expect_eq(u.valid, true);
        expect_eq(u.is_did, false);
        expect_eq(testEq(u.authority, "alice.bsky.social"), true);
        expect_eq(testEq(u.collection, "app.bsky.actor.profile"), true);
        expect_eq(testEq(u.rkey, "self"), true);
    });

    test("3. @cap/at-uri — Authority-only & authority+collection URIs", () => {
        const u1 = parseAtUri("at://bob.example.com");
        expect_eq(u1.valid, true);
        expect_eq(testEq(u1.authority, "bob.example.com"), true);
        expect_eq(testEq(u1.collection, ""), true);
        expect_eq(testEq(u1.rkey, ""), true);

        const u2 = parseAtUri("at://did:web:cap.dev/com.example.getProfile");
        expect_eq(u2.valid, true);
        expect_eq(u2.is_did, true);
        expect_eq(testEq(u2.collection, "com.example.getProfile"), true);
        expect_eq(testEq(u2.rkey, ""), true);
    });

    test("4. @cap/at-uri — isValidNsid & isValidRkey boundary checks", () => {
        expect_eq(isValidNsid("app.bsky.feed.post"), true);
        expect_eq(isValidNsid("com.example.myRecord2"), true);
        expect_eq(isValidNsid("too.short"), false);
        expect_eq(isValidNsid("1bad.bsky.post"), false);
        expect_eq(isValidNsid("App.bsky.post"), false);
        expect_eq(isValidNsid("app.bsky.1post"), false);

        expect_eq(isValidRkey("self"), true);
        expect_eq(isValidRkey("3k2yihcrp6f2c"), true);
        expect_eq(isValidRkey("a_b~c:d-e.f"), true);
        expect_eq(isValidRkey(""), false);
        expect_eq(isValidRkey("."), false);
        expect_eq(isValidRkey(".."), false);
        expect_eq(isValidRkey("bad/slash"), false);
    });

    test("5. @cap/at-uri — formatAtUri roundtrip", () => {
        let buf: [128]u8 = undefined;
        const n = formatAtUri("alice.bsky.social", "app.bsky.feed.post", "3k2yihcrp6f2c", buf[0..128]);
        const expected = "at://alice.bsky.social/app.bsky.feed.post/3k2yihcrp6f2c";
        expect_eq(n, expected.len);
        expect_eq(testEq(buf[0..n], expected), true);

        const reparsed = parseAtUri(buf[0..n]);
        expect_eq(reparsed.valid, true);
        expect_eq(testEq(reparsed.rkey, "3k2yihcrp6f2c"), true);
    });

    test("6. @cap/at-uri — Rejects bad prefix, extra segments, trailing slash, bad NSID & small buffer", () => {
        expect_eq(parseAtUri("https://bsky.app/a/b").valid, false);
        expect_eq(parseAtUri("at://alice.bsky.social/").valid, false);
        expect_eq(parseAtUri("at://alice.bsky.social/app.bsky.feed.post/self/extra").valid, false);
        expect_eq(parseAtUri("at://alice.bsky.social/badnsid/self").valid, false);
        expect_eq(parseAtUri("at://alice.bsky.social/app.bsky.feed.post/..").valid, false);

        let tiny: [8]u8 = undefined;
        expect_eq(formatAtUri("alice.bsky.social", "app.bsky.feed.post", "self", tiny[0..8]), 0 as usize);
        expect_eq(formatAtUri("alice.bsky.social", "", "orphan", tiny[0..8]), 0 as usize);
    });
}
