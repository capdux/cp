// Source: /data/dev/bun/src/base64/base64.cp & /data/dev/bun/src/url/url.cp
// cap/test/test_encoding_url_full.cp
// Comprehensive Full E2E Test Suite for cap:encoding (Base64 & Hex) and cap:url (URL, PercentEncoding, SearchParams)

import { test, expect_eq } from "cap:test";
import {
    Base64,
    Base64Encoder,
    Base64Decoder,
    Codecs,
    standard,
    standard_no_pad,
    url_safe,
    url_safe_no_pad,
    Hex,
} from "cap:encoding";
import {
    URL,
    Url,
    URLSearchParams,
    SearchParams,
    Query,
    QueryParam,
    Scanner,
    PercentEncoding,
    Percent,
} from "cap:url";
const std = @import("std");

fn testBase64StandardAndBoundary(): !void {
    let enc_buf: [128]u8 = undefined;
    let dec_buf: [128]u8 = undefined;

    // RFC 4648 Section 10 test vectors (0..6 bytes)
    const n0 = Base64.encode(&enc_buf, "");
    expect_eq(n0, 0 as usize);
    const d0 = Base64.decode(&dec_buf, enc_buf[0..n0])?;
    expect_eq(d0, 0 as usize);

    const n1 = Base64.encode(&enc_buf, "f");
    expect_eq(std.mem.eql(u8, enc_buf[0..n1], "Zg=="), true);
    const d1 = Base64.decode(&dec_buf, enc_buf[0..n1])?;
    expect_eq(std.mem.eql(u8, dec_buf[0..d1], "f"), true);

    const n2 = Base64.encode(&enc_buf, "fo");
    expect_eq(std.mem.eql(u8, enc_buf[0..n2], "Zm8="), true);
    const d2 = Base64.decode(&dec_buf, enc_buf[0..n2])?;
    expect_eq(std.mem.eql(u8, dec_buf[0..d2], "fo"), true);

    const n3 = Base64.encode(&enc_buf, "foo");
    expect_eq(std.mem.eql(u8, enc_buf[0..n3], "Zm9v"), true);
    const d3 = Base64.decode(&dec_buf, enc_buf[0..n3])?;
    expect_eq(std.mem.eql(u8, dec_buf[0..d3], "foo"), true);

    const n4 = Base64.encode(&enc_buf, "foob");
    expect_eq(std.mem.eql(u8, enc_buf[0..n4], "Zm9vYg=="), true);
    const d4 = Base64.decode(&dec_buf, enc_buf[0..n4])?;
    expect_eq(std.mem.eql(u8, dec_buf[0..d4], "foob"), true);

    const n5 = Base64.encode(&enc_buf, "fooba");
    expect_eq(std.mem.eql(u8, enc_buf[0..n5], "Zm9vYmE="), true);
    const d5 = Base64.decode(&dec_buf, enc_buf[0..n5])?;
    expect_eq(std.mem.eql(u8, dec_buf[0..d5], "fooba"), true);

    const n6 = Base64.encode(&enc_buf, "foobar");
    expect_eq(std.mem.eql(u8, enc_buf[0..n6], "Zm9vYmFy"), true);
    const d6 = Base64.decode(&dec_buf, enc_buf[0..n6])?;
    expect_eq(std.mem.eql(u8, dec_buf[0..d6], "foobar"), true);

    // Forgiving whitespace decoding
    const dw = Base64.decode(&dec_buf, "  Zm9v\r\n\t YmFy  ")?;
    expect_eq(std.mem.eql(u8, dec_buf[0..dw], "foobar"), true);

    // In-place decoding
    let inplace_buf: [16]u8 = undefined;
    @memcpy(inplace_buf[0..8], "Zm9vYmE=");
    const di = Base64.decodeInPlace(inplace_buf[0..8])?;
    expect_eq(di, 5 as usize);
    expect_eq(std.mem.eql(u8, inplace_buf[0..di], "fooba"), true);

    // Dynamic allocation round-trip
    const alloc = std.heap.page_allocator;
    const msg = "CP Standard Library Base64 & URL Full Audit 2026";
    const heap_enc = Base64.encodeAlloc(alloc, msg)?;
    defer alloc.free(heap_enc);
    const heap_dec = Base64.decodeAlloc(alloc, heap_enc)?;
    defer alloc.free(heap_dec);
    expect_eq(std.mem.eql(u8, heap_dec, msg), true);

    // Invalid input rejection
    expect_eq(Base64.isValid("Zm9vYmFy"), true);
    expect_eq(Base64.isValid("Zm9v*mFy"), false);
    let caught_err = false;
    * = Base64.decode(&dec_buf, "Zm9$YmFy") catch e => {
        caught_err = true;
    };
    expect_eq(caught_err, true);
}

fn testBase64URLSafeAndCodecs(): !void {
    let enc_buf: [64]u8 = undefined;
    let dec_buf: [64]u8 = undefined;

    // Bytes 0xFB, 0xFF, 0xFE produce "+//+" in standard Base64 and "-__-" in URL-Safe Base64
    const raw_bytes = "\xFB\xFF\xFE\xFF";

    const std_len = standard.encode(&enc_buf, raw_bytes);
    expect_eq(std.mem.eql(u8, enc_buf[0..std_len], "+//+/w=="), true);

    const std_np_len = standard_no_pad.encode(&enc_buf, raw_bytes);
    expect_eq(std.mem.eql(u8, enc_buf[0..std_np_len], "+//+/w"), true);

    const url_pad_len = Base64.encodeURLSafePadded(&enc_buf, raw_bytes);
    expect_eq(std.mem.eql(u8, enc_buf[0..url_pad_len], "-__-_w=="), true);

    const url_np_len = Base64.encodeURLSafe(&enc_buf, raw_bytes);
    expect_eq(std.mem.eql(u8, enc_buf[0..url_np_len], "-__-_w"), true);
    expect_eq(Base64.isValidURLSafe(enc_buf[0..url_np_len]), true);

    const dec_len = Base64.decodeURLSafe(&dec_buf, enc_buf[0..url_np_len])?;
    expect_eq(std.mem.eql(u8, dec_buf[0..dec_len], raw_bytes), true);

    // Dynamic URL-safe allocation
    const alloc = std.heap.page_allocator;
    const url_alloc_enc = Base64.encodeURLSafeAlloc(alloc, raw_bytes)?;
    defer alloc.free(url_alloc_enc);
    expect_eq(std.mem.eql(u8, url_alloc_enc, "-__-_w"), true);

    const url_alloc_dec = Base64.decodeURLSafeAlloc(alloc, url_alloc_enc)?;
    defer alloc.free(url_alloc_dec);
    expect_eq(std.mem.eql(u8, url_alloc_dec, raw_bytes), true);
}

fn testHexEncodingAndBoundaries(): !void {
    // 1. Empty input boundary
    let buf: [64]u8 = undefined;
    let out: [32]u8 = undefined;
    expect_eq(Hex.encodedLen(0), 0 as usize);
    expect_eq(Hex.decodedLen(0), 0 as usize);
    expect_eq(Hex.encodeLower("", &buf), 0 as usize);
    const d0 = Hex.decode("", &out)?;
    expect_eq(d0, 0 as usize);

    // 2. Lowercase & Uppercase encoding
    const sample = "CP-Hex\x00\xFF";
    const lower_n = Hex.encodeLower(sample, &buf);
    expect_eq(lower_n, 16 as usize);
    expect_eq(std.mem.eql(u8, buf[0..lower_n], "43502d48657800ff"), true);
    expect_eq(Hex.isValid(buf[0..lower_n]), true);

    let upper_buf: [64]u8 = undefined;
    const upper_n = Hex.encodeUpper(sample, &upper_buf);
    expect_eq(upper_n, 16 as usize);
    expect_eq(std.mem.eql(u8, upper_buf[0..upper_n], "43502D48657800FF"), true);
    expect_eq(Hex.isValid(upper_buf[0..upper_n]), true);

    // 3. Decoding and In-Place Decoding
    const dec_n = Hex.decode(upper_buf[0..upper_n], &out)?;
    expect_eq(dec_n, 8 as usize);
    expect_eq(std.mem.eql(u8, out[0..dec_n], sample), true);

    const inplace_n = Hex.decodeInPlace(buf[0..lower_n])?;
    expect_eq(inplace_n, 8 as usize);
    expect_eq(std.mem.eql(u8, buf[0..inplace_n], sample), true);

    // 4. Allocating Hex API
    const alloc = std.heap.page_allocator;
    const h_enc = Hex.encodeAlloc(alloc, "Antigravity")?;
    defer alloc.free(h_enc);
    expect_eq(std.mem.eql(u8, h_enc, "416e746967726176697479"), true);

    const h_dec = Hex.decodeAlloc(alloc, h_enc)?;
    defer alloc.free(h_dec);
    expect_eq(std.mem.eql(u8, h_dec, "Antigravity"), true);

    // 5. Odd length and invalid character errors
    expect_eq(Hex.isValid("abc"), false);
    expect_eq(Hex.isValid("abg0"), false);

    let odd_err = false;
    * = Hex.decode("abc", &out) catch e => {
        odd_err = true;
    };
    expect_eq(odd_err, true);

    let char_err = false;
    * = Hex.decode("abzz", &out) catch e => {
        char_err = true;
    };
    expect_eq(char_err, true);
}

fn testUrlParsingAndComponents(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Full URL with protocol/scheme, username, password, host, hostname, port, pathname, search, hash
    let u = Url.parse(
        "https://admin:p4ssw0rd@gateway.cp.dev:9443/v1/clusters/nodes?region=ap-east&role=worker#metrics",
        alloc
    )?;
    defer u.deinit();

    expect_eq(u.protocol, "https:");
    expect_eq(u.scheme, "https");
    expect_eq(u.getScheme(), "https");
    expect_eq(u.username, "admin");
    expect_eq(u.password, "p4ssw0rd");
    expect_eq(u.host, "gateway.cp.dev:9443");
    expect_eq(u.hostname, "gateway.cp.dev");
    expect_eq(u.port, "9443");
    expect_eq(u.pathname, "/v1/clusters/nodes");
    expect_eq(u.path, "/v1/clusters/nodes");
    expect_eq(u.search, "?region=ap-east&role=worker");
    expect_eq(u.hash, "#metrics");
    expect_eq(u.origin, "https://gateway.cp.dev:9443");
    expect_eq(u.hostWithPath(), "gateway.cp.dev:9443/v1/clusters/nodes");

    expect_eq(u.isHTTPS(), true);
    expect_eq(u.isHTTP(), false);
    expect_eq(u.hasHTTPLikeProtocol(), true);
    expect_eq(u.hasValidPort(), true);
    expect_eq(u.getPortAuto(), 9443 as u16);
    if (p := u.getPort()) {
        expect_eq(p, 9443 as u16);
    }

    // 2. IPv6, IPv4, Default Ports, Special Schemes (s3://, file://, unix://, blob:)
    let u_ipv6 = URL.parseSlice("http://[::1]:8080/health/");
    expect_eq(u_ipv6.scheme, "http");
    expect_eq(u_ipv6.host, "[::1]:8080");
    expect_eq(u_ipv6.hostname, "[::1]");
    expect_eq(u_ipv6.port, "8080");
    expect_eq(u_ipv6.isIPAddress(), true);
    expect_eq(u_ipv6.isLocalhost(), true);
    expect_eq(u_ipv6.hostWithPath(), "[::1]:8080/health");

    let u_def = URL.parseSlice("https://cp-lang.org");
    expect_eq(u_def.port, "");
    expect_eq(u_def.getPort() == null, true);
    expect_eq(u_def.getPortAuto(), 443 as u16);
    expect_eq(u_def.pathname, "/");
    expect_eq(u_def.origin, "https://cp-lang.org");

    let u_s3 = URL.parseSlice("s3://ai-models/llama4/weights.bin?partNumber=2#sha256");
    expect_eq(u_s3.isS3(), true);
    expect_eq(u_s3.s3Path(), "ai-models/llama4/weights.bin");

    let u_file = URL.parseSlice("file:///etc/hosts");
    expect_eq(u_file.isFile(), true);
    expect_eq(u_file.pathname, "/etc/hosts");
    expect_eq(u_file.origin, "null");

    // 3. Path normalization
    let norm_buf: [128]u8 = undefined;
    const joined = URL.joinNormalize(&norm_buf, "v2", "/users/profile", "avatar", ".webp");
    expect_eq(joined, "/v2/users/profile/avatar.webp");
}

fn testPercentAndSearchParams(): !void {
    const alloc = std.heap.page_allocator;

    // 1. PercentEncoding / Percent
    expect_eq(Percent.isUnreserved('A'), true);
    expect_eq(Percent.isUnreserved('~'), true);
    expect_eq(Percent.isUnreserved('/'), false);
    expect_eq(Percent.needsEncodeComponent("clean_token-1.0~"), false);
    expect_eq(Percent.needsEncodeComponent("a/b?c=1&d=2"), true);

    let enc_buf: [128]u8 = undefined;
    const enc_n = Percent.encode("key=val&msg=hello world!", &enc_buf);
    expect_eq(enc_buf[0..enc_n], "key%3Dval%26msg%3Dhello%20world%21");

    let dec_buf: [128]u8 = undefined;
    const dec_n = Percent.decode(enc_buf[0..enc_n], &dec_buf);
    expect_eq(dec_buf[0..dec_n], "key=val&msg=hello world!");

    const q_n = Percent.decodeQuery("search+term%2B1", &dec_buf);
    expect_eq(dec_buf[0..q_n], "search term+1");

    const heap_enc = PercentEncoding.encodeComponent("a+b/c", alloc)?;
    defer alloc.free(heap_enc);
    expect_eq(heap_enc, "a%2Bb%2Fc");

    const heap_dec = PercentEncoding.decodeAlloc(heap_enc, false, alloc)?;
    defer alloc.free(heap_dec);
    expect_eq(heap_dec, "a+b/c");

    // 2. SearchParams / Query / URLSearchParams
    let sp = SearchParams.parse("?tag=zig&name=Alice+Smith&tag=rust&lang=CP%202.0", alloc);
    defer sp.deinit();

    expect_eq(sp.count(), 4 as usize);
    expect_eq(sp.has("tag"), true);
    expect_eq(sp.has("missing"), false);

    if (first_tag := sp.get("tag")) {
        expect_eq(first_tag, "zig");
    }

    let val_buf: [64]u8 = undefined;
    if (dec_name := sp.getDecoded("name", &val_buf)) {
        expect_eq(dec_name, "Alice Smith");
    }
    if (dec_lang := sp.getDecoded("lang", &val_buf)) {
        expect_eq(dec_lang, "CP 2.0");
    }

    let all_tags: [4]string = undefined;
    const tag_cnt = sp.getAll("tag", all_tags[0..4]);
    expect_eq(tag_cnt, 2 as usize);
    expect_eq(all_tags[0], "zig");
    expect_eq(all_tags[1], "rust");

    // Modify & Sort
    sp.set("name", "Bob");
    sp.delete("lang");
    sp.append("arch", "x86_64");
    sp.sort();

    let ser_buf: [128]u8 = undefined;
    const serialized = sp.toString(&ser_buf);
    expect_eq(serialized, "arch=x86_64&name=Bob&tag=zig&tag=rust");

    const owned_qs = sp.toOwnedString(alloc)?;
    defer alloc.free(owned_qs);
    expect_eq(owned_qs, "arch=x86_64&name=Bob&tag=zig&tag=rust");
}

fn main() {
    test("1. cap:encoding — Standard Base64 encode, decode, decodeInPlace, alloc & RFC 4648 vectors", () => {
        try testBase64StandardAndBoundary();
    });
    test("2. cap:encoding — URL-Safe Base64 (padded & unpadded) and Codecs matrix", () => {
        try testBase64URLSafeAndCodecs();
    });
    test("3. cap:encoding — Hex encodeLower, encodeUpper, decode, decodeInPlace & boundary validation", () => {
        try testHexEncodingAndBoundaries();
    });
    test("4. cap:url — WHATWG URL component parsing, ports, IPv6, special schemes & joinNormalize", () => {
        try testUrlParsingAndComponents();
    });
    test("5. cap:url — RFC 3986 PercentEncoding and URLSearchParams (SearchParams/Query) operations", () => {
        try testPercentAndSearchParams();
    });
}
