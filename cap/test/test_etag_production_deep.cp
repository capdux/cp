import { test, expect } from "cap:test";
import {
    ParsedETag,
    ifNoneMatch,
    ifMatch,
    generateStrong,
    generateWeak,
} from "cap:etag";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("etag: parse, strong/weak comparison, and header roundtrip", () => {
    let buf: [64]u8 = [0; 64];

    const strong = ParsedETag.parse("\"abc123\"").?;
    const weak = ParsedETag.parse("W/\"abc123\"").?;
    expect(!strong.is_weak && weak.is_weak);
    expect(strEq(strong.tag, "abc123"));
    expect(strong.weakMatch(weak));
    expect(!strong.strongMatch(weak));
    expect(strong.strongMatch(strong));
    expect(strEq(weak.toHeader(buf[0..]).?, "W/\"abc123\""));

    // Malformed ETags rejected
    expect(ParsedETag.parse("unquoted") == null);
    expect(ParsedETag.parse("w/\"lowercase-w\"") == null);
    expect(ParsedETag.parse("\"has space\"") == null);
});

test("etag: ifNoneMatch, ifMatch with commas in quotes, and content generators", () => {
    // Quoted comma inside ETag must not split token
    expect(ifNoneMatch("\"a,b\"", "\"x\", W/\"a,b\", \"z\""));
    expect(!ifMatch("\"a,b\"", "\"x\", W/\"a,b\", \"z\""));
    expect(ifMatch("\"a,b\"", "\"x\", \"a,b\", \"z\""));
    expect(ifNoneMatch("\"anything\"", " * "));
    expect(!ifNoneMatch("\"abc\"", "\"def\", \"ghi\""));

    let sbuf: [32]u8 = [0; 32];
    let wbuf: [32]u8 = [0; 32];
    const s1 = generateStrong("hello world", sbuf[0..]).?;
    const w1 = generateWeak("hello world", wbuf[0..]).?;
    expect(s1.len == 18 && w1.len == 20);
    expect(ifNoneMatch(s1, w1));
    expect(!ifMatch(s1, w1));
    expect(ifMatch(s1, s1));
});
