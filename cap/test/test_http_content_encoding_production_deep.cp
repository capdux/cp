import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ContentCoding,
    parseContentCoding,
    ContentEncodingIter,
    encodingLayerCount,
    isCompressed,
    hasContentEncoding,
    isValidContentEncoding,
} from "cap:http_content_encoding";

test("1. parseContentCoding maps standard codings and legacy x- aliases case-insensitively", () => {
    expect_eq(parseContentCoding("identity") == ContentCoding.identity, true);
    expect_eq(parseContentCoding("GZIP") == ContentCoding.gzip, true);
    expect_eq(parseContentCoding(" x-gzip ") == ContentCoding.gzip, true);
    expect_eq(parseContentCoding("Deflate") == ContentCoding.deflate, true);
    expect_eq(parseContentCoding("BR") == ContentCoding.br, true);
    expect_eq(parseContentCoding("zstd") == ContentCoding.zstd, true);
    expect_eq(parseContentCoding("compress") == ContentCoding.compress, true);
    expect_eq(parseContentCoding("X-Compress") == ContentCoding.compress, true);
    expect_eq(parseContentCoding("lz4") == ContentCoding.unknown, true);
    expect_eq(std.mem.eql(u8, ContentCoding.zstd.asStr(), "zstd"), true);
});

test("2. ContentEncodingIter streams multi-layer encoding stack and skips empty segments", () => {
    let it = ContentEncodingIter.init(" , deflate , x-gzip , , zstd ");
    expect_eq((it.next() ?? unreachable) == ContentCoding.deflate, true);
    expect_eq((it.next() ?? unreachable) == ContentCoding.gzip, true);
    expect_eq((it.next() ?? unreachable) == ContentCoding.zstd, true);
    expect_eq(it.next() == null, true);
});

test("3. encodingLayerCount and isCompressed exclude identity", () => {
    expect_eq(encodingLayerCount(""), usize(0));
    expect_eq(encodingLayerCount("identity"), usize(0));
    expect_eq(encodingLayerCount("identity, identity"), usize(0));
    expect_eq(isCompressed(""), false);
    expect_eq(isCompressed("identity"), false);
    expect_eq(isCompressed("unknown-codec"), false);

    expect_eq(encodingLayerCount("identity, gzip, br"), usize(2));
    expect_eq(isCompressed("identity, gzip, br"), true);
});

test("4. hasContentEncoding matches normalized aliases", () => {
    const hdr = "x-gzip, br";
    expect_eq(hasContentEncoding(hdr, ContentCoding.gzip), true);
    expect_eq(hasContentEncoding(hdr, ContentCoding.br), true);
    expect_eq(hasContentEncoding(hdr, ContentCoding.zstd), false);
});

test("5. isValidContentEncoding guards against unknown codecs and excessive compression depth", () => {
    expect_eq(isValidContentEncoding("", 2), true);
    expect_eq(isValidContentEncoding("identity", 0), true);
    expect_eq(isValidContentEncoding("gzip, br", 2), true);
    // 超过最大层数上限拦截（资源耗尽防护）
    expect_eq(isValidContentEncoding("gzip, gzip, gzip", 2), false);
    // 含未知编码拦截
    expect_eq(isValidContentEncoding("gzip, bogus", 5), false);
});
