import {
    VaryIter,
    isVaryWildcard,
    varyContains,
    appendVary,
    mergeVary,
} from "cap:http_vary";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("VaryIter, varyContains, and wildcard detection", () => {
    const v = "Accept-Encoding, Origin, , Accept-Language";
    expect(!isVaryWildcard(v));
    expect(varyContains(v, "accept-encoding"));
    expect(varyContains(v, "ORIGIN"));
    expect(!varyContains(v, "Cookie"));

    let it = VaryIter.init(v);
    expect(strEq(it.next().?, "Accept-Encoding"));
    expect(strEq(it.next().?, "Origin"));
    expect(strEq(it.next().?, "Accept-Language"));
    expect(it.next() == null);

    expect(isVaryWildcard("Accept-Encoding, *"));
});

test("appendVary and mergeVary deduplication and wildcard collapse", () => {
    let buf: [128]u8 = [0; 128];

    // Append new field
    expect(strEq(appendVary("Origin", "Accept-Encoding", buf[0..]).?, "Origin, Accept-Encoding"));
    // Append duplicate (case-insensitive) is a no-op
    expect(strEq(appendVary("Origin, Accept-Encoding", "origin", buf[0..]).?, "Origin, Accept-Encoding"));
    // Append wildcard collapses to "*"
    expect(strEq(appendVary("Origin", "*", buf[0..]).?, "*"));
    // Invalid field name returns null
    expect(appendVary("Origin", "Bad Header", buf[0..]) == null);

    // Merge two Vary headers with lowercase dedup
    expect(strEq(
        mergeVary("Accept-Encoding, Origin", "origin, Accept-Language, ACCEPT-ENCODING", buf[0..]).?,
        "accept-encoding, origin, accept-language",
    ));
    expect(strEq(mergeVary("Origin", "*", buf[0..]).?, "*"));
});
