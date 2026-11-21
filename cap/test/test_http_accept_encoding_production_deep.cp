import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EncodingKind,
    AcceptEncodingIter,
    negotiateEncoding,
    isEncodingAcceptable,
} from "cap:http";

test("1. AcceptEncodingIter parses codings, x-gzip alias, q_permille, and skips malformed entries", () => {
    const hdr = "zstd, br;q=0.9, x-gzip; q = 0.85 , deflate;q=0.5, identity;q=0, *;q=0.1, bad coding, gzip;q=1.5, br;foo=1";
    let it = AcceptEncodingIter.init(hdr);

    const i1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, i1.coding, "zstd"), true);
    expect_eq(i1.kind == EncodingKind.zstd, true);
    expect_eq(i1.q_permille, u16(1000));

    const i2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, i2.coding, "br"), true);
    expect_eq(i2.kind == EncodingKind.br, true);
    expect_eq(i2.q_permille, u16(900));

    const i3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, i3.coding, "x-gzip"), true);
    expect_eq(i3.kind == EncodingKind.gzip, true);
    expect_eq(i3.q_permille, u16(850));

    const i4 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, i4.coding, "deflate"), true);
    expect_eq(i4.kind == EncodingKind.deflate, true);
    expect_eq(i4.q_permille, u16(500));

    const i5 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, i5.coding, "identity"), true);
    expect_eq(i5.kind == EncodingKind.identity, true);
    expect_eq(i5.q_permille, u16(0));

    const i6 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, i6.coding, "*"), true);
    expect_eq(i6.kind == EncodingKind.wildcard, true);
    expect_eq(i6.q_permille, u16(100));

    expect_eq(it.next() == null, true);
});

test("2. negotiateEncoding selects highest q_permille and breaks ties by server supported order", () => {
    const supported: [4][]const u8 = ["zstd", "br", "gzip", "identity"];

    // Highest q wins regardless of server order
    const best1 = negotiateEncoding("zstd;q=0.5, br;q=0.9, gzip;q=0.8", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best1, "br"), true);

    // Equal q=1.0: server order picks zstd over br and gzip
    const best2 = negotiateEncoding("gzip, deflate, br, zstd", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best2, "zstd"), true);

    // x-gzip matches server "gzip"
    const best3 = negotiateEncoding("x-gzip;q=0.9", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best3, "gzip"), true);
});

test("3. negotiateEncoding handles wildcard priority and q=0 explicit rejections", () => {
    const supported: [3][]const u8 = ["br", "gzip", "identity"];

    // br explicitly rejected with q=0 overrides *;q=0.8, so gzip is picked
    const best1 = negotiateEncoding("br;q=0, *;q=0.8", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best1, "gzip"), true);

    // *;q=0 rejects all non-listed encodings including identity
    expect_eq(negotiateEncoding("zstd, *;q=0", &supported) == null, true);

    // Explicit positive encoding survives *;q=0
    const best2 = negotiateEncoding("gzip;q=0.5, *;q=0", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best2, "gzip"), true);
});

test("4. negotiateEncoding falls back to identity when unmentioned unless rejected", () => {
    const supported: [2][]const u8 = ["gzip", "identity"];

    // Empty header allows implicit identity fallback
    const best1 = negotiateEncoding("", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best1, "identity"), true);

    // Unmatched compression algorithm falls back to identity
    const best2 = negotiateEncoding("br, zstd", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best2, "identity"), true);

    // Explicit identity;q=0 prevents fallback
    expect_eq(negotiateEncoding("br, identity;q=0", &supported) == null, true);

    // Explicit low q=0.001 still beats implicit identity fallback
    const best3 = negotiateEncoding("gzip;q=0.001", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best3, "gzip"), true);
});

test("5. isEncodingAcceptable validates exact, alias, wildcard, identity, and invalid queries", () => {
    expect_eq(isEncodingAcceptable("gzip, br;q=0.8", "br"), true);
    expect_eq(isEncodingAcceptable("x-gzip", "gzip"), true);
    expect_eq(isEncodingAcceptable("gzip", "x-gzip"), true);
    expect_eq(isEncodingAcceptable("gzip, br;q=0", "br"), false);
    expect_eq(isEncodingAcceptable("gzip", "identity"), true);
    expect_eq(isEncodingAcceptable("gzip, identity;q=0", "identity"), false);
    expect_eq(isEncodingAcceptable("gzip, *;q=0", "identity"), false);
    expect_eq(isEncodingAcceptable("*", "zstd"), true);

    // Invalid coding queries return false
    expect_eq(isEncodingAcceptable("*", "*"), false);
    expect_eq(isEncodingAcceptable("*", ""), false);
    expect_eq(isEncodingAcceptable("*", "bad coding"), false);
});
