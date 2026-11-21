// Source: cp/cap/test/test_http_types_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Encoding,
    FetchCacheMode,
    FetchRedirect,
    FetchRequestMode,
    Method,
    parseETag,
    computeETag,
    ifNoneMatch,
    URLPath,
} from "cap:http_types";

test "http_types: Method lookup & semantics (hasBody, hasRequestBody, isIdempotent)" {
    const get_m = Method.find("GET").?;
    const post_m = Method.find("post").?;
    const head_m = Method.find("HEAD").?;
    const msearch = Method.find("M-SEARCH").?;

    try expect_eq(get_m == .GET, true);
    try expect_eq(post_m == .POST, true);
    try expect_eq(msearch == .@"M-SEARCH", true);

    try expect_eq(get_m.hasBody(), true);
    try expect_eq(get_m.hasRequestBody(), false);
    try expect_eq(get_m.isIdempotent(), true);

    try expect_eq(post_m.hasRequestBody(), true);
    try expect_eq(post_m.isIdempotent(), false);

    try expect_eq(head_m.hasBody(), false);
    try expect_eq(Method.find("INVALID") == null, true);
}

test "http_types: ETag parse, computeETag, and ifNoneMatch weak/wildcard" {
    let buf: [40]u8 = undefined;
    const tag = computeETag("hello bun", &buf);
    try expect_eq(tag[0] == '"' and tag[tag.len - 1] == '"', true);

    const p = parseETag("W/\"12345\"");
    try expect_eq(p.is_weak, true);
    try expect_eq(std.mem.eql(u8, p.tag, "12345"), true);

    try expect_eq(ifNoneMatch("\"abc\"", "*"), true);
    try expect_eq(ifNoneMatch("\"abc\"", "W/\"xyz\", W/\"abc\""), true);
    try expect_eq(ifNoneMatch("\"abc\"", "\"xyz\""), false);
}

test "http_types: Encoding & Fetch enums" {
    try expect_eq(Encoding.gzip.canUseLibDeflate(), true);
    try expect_eq(Encoding.brotli.canUseLibDeflate(), false);
    try expect_eq(Encoding.zstd.isCompressed(), true);
    try expect_eq(Encoding.identity.isCompressed(), false);
    try expect_eq(Encoding.fromHeader("br").? == .brotli, true);

    try expect_eq(FetchCacheMode.parse("no-store").? == .@"no-store", true);
    try expect_eq(FetchRedirect.parse("manual").? == .manual, true);
    try expect_eq(FetchRequestMode.parse("cors").? == .cors, true);
}

test "http_types: URLPath zero-alloc parsing & source map extraction" {
    const p1 = URLPath.parse("/static/app.js?t=123");
    try expect_eq(std.mem.eql(u8, p1.extname, "js"), true);
    try expect_eq(std.mem.eql(u8, p1.first_segment, "static"), true);
    try expect_eq(std.mem.eql(u8, p1.query_string, "?t=123"), true);
    try expect_eq(p1.is_source_map, false);

    const p2 = URLPath.parse("/dist/bundle.css.map");
    try expect_eq(p2.is_source_map, true);
    try expect_eq(std.mem.eql(u8, p2.extname, "css"), true);
    try expect_eq(std.mem.eql(u8, p2.path, "dist/bundle.css"), true);

    const root = URLPath.parse("/");
    try expect_eq(root.isRoot(""), true);
}

test "http_types: cap.http_types dynamic mount" {
    const m = cap.http_types.Method.find("DELETE").?;
    try expect_eq(m.isIdempotent(), true);
    try expect_eq(cap.http_types.ifNoneMatch("\"v1\"", "\"v1\""), true);
}
