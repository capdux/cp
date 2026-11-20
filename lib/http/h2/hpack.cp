// Source: /data/dev/bun/src/http/lshpack.cp
// lib/http/h2/hpack.cp
// RFC 7541 Complete HPACK Header Compression for HTTP/2:
// - 61-entry Static Table (Appendix A)
// - 257-entry Canonical Huffman Codec (Appendix B)
// - Dynamic Table with RFC 7541 §4.1 FIFO Eviction & Table Size Updates

const std = @import("std");

pub struct HpackIntResult {
    pub value: usize = 0;
    pub bytes_read: usize = 0;
}

pub struct HpackStaticEntry {
    pub index: usize = 0;
    pub name: []const u8 = "";
    pub value: []const u8 = "";
}

pub struct HpackDecodedHeader {
    pub name: []const u8 = "";
    pub value: []const u8 = "";
    pub never_index: bool = false;
    pub bytes_read: usize = 0;
}

pub struct HpackHuffCode {
    pub code: u32 = 0;
    pub bits: u8 = 0;
}

pub struct HpackHuffBucket {
    pub min_code: u32 = 0;
    pub max_code: u32 = 0;
    pub offset: u16 = 0;
    pub count: u16 = 0;
}

/// RFC 7541 Appendix A: Complete 61-entry Static Table (1-based indices 1..61).
pub const HPACK_STATIC_TABLE: [61]HpackStaticEntry = [
    HpackStaticEntry{ index: 1, name: ":authority", value: "" },
    HpackStaticEntry{ index: 2, name: ":method", value: "GET" },
    HpackStaticEntry{ index: 3, name: ":method", value: "POST" },
    HpackStaticEntry{ index: 4, name: ":path", value: "/" },
    HpackStaticEntry{ index: 5, name: ":path", value: "/index.html" },
    HpackStaticEntry{ index: 6, name: ":scheme", value: "http" },
    HpackStaticEntry{ index: 7, name: ":scheme", value: "https" },
    HpackStaticEntry{ index: 8, name: ":status", value: "200" },
    HpackStaticEntry{ index: 9, name: ":status", value: "204" },
    HpackStaticEntry{ index: 10, name: ":status", value: "206" },
    HpackStaticEntry{ index: 11, name: ":status", value: "304" },
    HpackStaticEntry{ index: 12, name: ":status", value: "400" },
    HpackStaticEntry{ index: 13, name: ":status", value: "404" },
    HpackStaticEntry{ index: 14, name: ":status", value: "500" },
    HpackStaticEntry{ index: 15, name: "accept-charset", value: "" },
    HpackStaticEntry{ index: 16, name: "accept-encoding", value: "gzip, deflate" },
    HpackStaticEntry{ index: 17, name: "accept-language", value: "" },
    HpackStaticEntry{ index: 18, name: "accept-ranges", value: "" },
    HpackStaticEntry{ index: 19, name: "accept", value: "" },
    HpackStaticEntry{ index: 20, name: "access-control-allow-origin", value: "" },
    HpackStaticEntry{ index: 21, name: "age", value: "" },
    HpackStaticEntry{ index: 22, name: "allow", value: "" },
    HpackStaticEntry{ index: 23, name: "authorization", value: "" },
    HpackStaticEntry{ index: 24, name: "cache-control", value: "" },
    HpackStaticEntry{ index: 25, name: "content-disposition", value: "" },
    HpackStaticEntry{ index: 26, name: "content-encoding", value: "" },
    HpackStaticEntry{ index: 27, name: "content-language", value: "" },
    HpackStaticEntry{ index: 28, name: "content-length", value: "" },
    HpackStaticEntry{ index: 29, name: "content-location", value: "" },
    HpackStaticEntry{ index: 30, name: "content-range", value: "" },
    HpackStaticEntry{ index: 31, name: "content-type", value: "" },
    HpackStaticEntry{ index: 32, name: "cookie", value: "" },
    HpackStaticEntry{ index: 33, name: "date", value: "" },
    HpackStaticEntry{ index: 34, name: "etag", value: "" },
    HpackStaticEntry{ index: 35, name: "expect", value: "" },
    HpackStaticEntry{ index: 36, name: "expires", value: "" },
    HpackStaticEntry{ index: 37, name: "from", value: "" },
    HpackStaticEntry{ index: 38, name: "host", value: "" },
    HpackStaticEntry{ index: 39, name: "if-match", value: "" },
    HpackStaticEntry{ index: 40, name: "if-modified-since", value: "" },
    HpackStaticEntry{ index: 41, name: "if-none-match", value: "" },
    HpackStaticEntry{ index: 42, name: "if-range", value: "" },
    HpackStaticEntry{ index: 43, name: "if-unmodified-since", value: "" },
    HpackStaticEntry{ index: 44, name: "last-modified", value: "" },
    HpackStaticEntry{ index: 45, name: "link", value: "" },
    HpackStaticEntry{ index: 46, name: "location", value: "" },
    HpackStaticEntry{ index: 47, name: "max-forwards", value: "" },
    HpackStaticEntry{ index: 48, name: "proxy-authenticate", value: "" },
    HpackStaticEntry{ index: 49, name: "proxy-authorization", value: "" },
    HpackStaticEntry{ index: 50, name: "range", value: "" },
    HpackStaticEntry{ index: 51, name: "referer", value: "" },
    HpackStaticEntry{ index: 52, name: "refresh", value: "" },
    HpackStaticEntry{ index: 53, name: "retry-after", value: "" },
    HpackStaticEntry{ index: 54, name: "server", value: "" },
    HpackStaticEntry{ index: 55, name: "set-cookie", value: "" },
    HpackStaticEntry{ index: 56, name: "strict-transport-security", value: "" },
    HpackStaticEntry{ index: 57, name: "transfer-encoding", value: "" },
    HpackStaticEntry{ index: 58, name: "user-agent", value: "" },
    HpackStaticEntry{ index: 59, name: "vary", value: "" },
    HpackStaticEntry{ index: 60, name: "via", value: "" },
    HpackStaticEntry{ index: 61, name: "www-authenticate", value: "" },
];

/// RFC 7541 Appendix B: Complete 257-entry Huffman Code Table (0..255 + EOS=256).
pub const HPACK_HUFFMAN_TABLE: [257]HpackHuffCode = [
    HpackHuffCode{ code: 0x1ff8, bits: 13 }, HpackHuffCode{ code: 0x7fffd8, bits: 23 },
    HpackHuffCode{ code: 0xfffffe2, bits: 28 }, HpackHuffCode{ code: 0xfffffe3, bits: 28 },
    HpackHuffCode{ code: 0xfffffe4, bits: 28 }, HpackHuffCode{ code: 0xfffffe5, bits: 28 },
    HpackHuffCode{ code: 0xfffffe6, bits: 28 }, HpackHuffCode{ code: 0xfffffe7, bits: 28 },
    HpackHuffCode{ code: 0xfffffe8, bits: 28 }, HpackHuffCode{ code: 0xffffea, bits: 24 },
    HpackHuffCode{ code: 0x3ffffffc, bits: 30 }, HpackHuffCode{ code: 0xfffffe9, bits: 28 },
    HpackHuffCode{ code: 0xfffffea, bits: 28 }, HpackHuffCode{ code: 0x3ffffffd, bits: 30 },
    HpackHuffCode{ code: 0xfffffeb, bits: 28 }, HpackHuffCode{ code: 0xfffffec, bits: 28 },
    HpackHuffCode{ code: 0xfffffed, bits: 28 }, HpackHuffCode{ code: 0xfffffee, bits: 28 },
    HpackHuffCode{ code: 0xfffffef, bits: 28 }, HpackHuffCode{ code: 0xffffff0, bits: 28 },
    HpackHuffCode{ code: 0xffffff1, bits: 28 }, HpackHuffCode{ code: 0xffffff2, bits: 28 },
    HpackHuffCode{ code: 0x3ffffffe, bits: 30 }, HpackHuffCode{ code: 0xffffff3, bits: 28 },
    HpackHuffCode{ code: 0xffffff4, bits: 28 }, HpackHuffCode{ code: 0xffffff5, bits: 28 },
    HpackHuffCode{ code: 0xffffff6, bits: 28 }, HpackHuffCode{ code: 0xffffff7, bits: 28 },
    HpackHuffCode{ code: 0xffffff8, bits: 28 }, HpackHuffCode{ code: 0xffffff9, bits: 28 },
    HpackHuffCode{ code: 0xffffffa, bits: 28 }, HpackHuffCode{ code: 0xffffffb, bits: 28 },
    HpackHuffCode{ code: 0x14, bits: 6 }, HpackHuffCode{ code: 0x3f8, bits: 10 },
    HpackHuffCode{ code: 0x3f9, bits: 10 }, HpackHuffCode{ code: 0xffa, bits: 12 },
    HpackHuffCode{ code: 0x1ff9, bits: 13 }, HpackHuffCode{ code: 0x15, bits: 6 },
    HpackHuffCode{ code: 0xf8, bits: 8 }, HpackHuffCode{ code: 0x7fa, bits: 11 },
    HpackHuffCode{ code: 0x3fa, bits: 10 }, HpackHuffCode{ code: 0x3fb, bits: 10 },
    HpackHuffCode{ code: 0xf9, bits: 8 }, HpackHuffCode{ code: 0x7fb, bits: 11 },
    HpackHuffCode{ code: 0xfa, bits: 8 }, HpackHuffCode{ code: 0x16, bits: 6 },
    HpackHuffCode{ code: 0x17, bits: 6 }, HpackHuffCode{ code: 0x18, bits: 6 },
    HpackHuffCode{ code: 0x0, bits: 5 }, HpackHuffCode{ code: 0x1, bits: 5 },
    HpackHuffCode{ code: 0x2, bits: 5 }, HpackHuffCode{ code: 0x19, bits: 6 },
    HpackHuffCode{ code: 0x1a, bits: 6 }, HpackHuffCode{ code: 0x1b, bits: 6 },
    HpackHuffCode{ code: 0x1c, bits: 6 }, HpackHuffCode{ code: 0x1d, bits: 6 },
    HpackHuffCode{ code: 0x1e, bits: 6 }, HpackHuffCode{ code: 0x1f, bits: 6 },
    HpackHuffCode{ code: 0x5c, bits: 7 }, HpackHuffCode{ code: 0xfb, bits: 8 },
    HpackHuffCode{ code: 0x7ffc, bits: 15 }, HpackHuffCode{ code: 0x20, bits: 6 },
    HpackHuffCode{ code: 0xffb, bits: 12 }, HpackHuffCode{ code: 0x3fc, bits: 10 },
    HpackHuffCode{ code: 0x1ffa, bits: 13 }, HpackHuffCode{ code: 0x21, bits: 6 },
    HpackHuffCode{ code: 0x5d, bits: 7 }, HpackHuffCode{ code: 0x5e, bits: 7 },
    HpackHuffCode{ code: 0x5f, bits: 7 }, HpackHuffCode{ code: 0x60, bits: 7 },
    HpackHuffCode{ code: 0x61, bits: 7 }, HpackHuffCode{ code: 0x62, bits: 7 },
    HpackHuffCode{ code: 0x63, bits: 7 }, HpackHuffCode{ code: 0x64, bits: 7 },
    HpackHuffCode{ code: 0x65, bits: 7 }, HpackHuffCode{ code: 0x66, bits: 7 },
    HpackHuffCode{ code: 0x67, bits: 7 }, HpackHuffCode{ code: 0x68, bits: 7 },
    HpackHuffCode{ code: 0x69, bits: 7 }, HpackHuffCode{ code: 0x6a, bits: 7 },
    HpackHuffCode{ code: 0x6b, bits: 7 }, HpackHuffCode{ code: 0x6c, bits: 7 },
    HpackHuffCode{ code: 0x6d, bits: 7 }, HpackHuffCode{ code: 0x6e, bits: 7 },
    HpackHuffCode{ code: 0x6f, bits: 7 }, HpackHuffCode{ code: 0x70, bits: 7 },
    HpackHuffCode{ code: 0x71, bits: 7 }, HpackHuffCode{ code: 0x72, bits: 7 },
    HpackHuffCode{ code: 0xfc, bits: 8 }, HpackHuffCode{ code: 0x73, bits: 7 },
    HpackHuffCode{ code: 0xfd, bits: 8 }, HpackHuffCode{ code: 0x1ffb, bits: 13 },
    HpackHuffCode{ code: 0x7fff0, bits: 19 }, HpackHuffCode{ code: 0x1ffc, bits: 13 },
    HpackHuffCode{ code: 0x3ffc, bits: 14 }, HpackHuffCode{ code: 0x22, bits: 6 },
    HpackHuffCode{ code: 0x7ffd, bits: 15 }, HpackHuffCode{ code: 0x3, bits: 5 },
    HpackHuffCode{ code: 0x23, bits: 6 }, HpackHuffCode{ code: 0x4, bits: 5 },
    HpackHuffCode{ code: 0x24, bits: 6 }, HpackHuffCode{ code: 0x5, bits: 5 },
    HpackHuffCode{ code: 0x25, bits: 6 }, HpackHuffCode{ code: 0x26, bits: 6 },
    HpackHuffCode{ code: 0x27, bits: 6 }, HpackHuffCode{ code: 0x6, bits: 5 },
    HpackHuffCode{ code: 0x74, bits: 7 }, HpackHuffCode{ code: 0x75, bits: 7 },
    HpackHuffCode{ code: 0x28, bits: 6 }, HpackHuffCode{ code: 0x29, bits: 6 },
    HpackHuffCode{ code: 0x2a, bits: 6 }, HpackHuffCode{ code: 0x7, bits: 5 },
    HpackHuffCode{ code: 0x2b, bits: 6 }, HpackHuffCode{ code: 0x76, bits: 7 },
    HpackHuffCode{ code: 0x2c, bits: 6 }, HpackHuffCode{ code: 0x8, bits: 5 },
    HpackHuffCode{ code: 0x9, bits: 5 }, HpackHuffCode{ code: 0x2d, bits: 6 },
    HpackHuffCode{ code: 0x77, bits: 7 }, HpackHuffCode{ code: 0x78, bits: 7 },
    HpackHuffCode{ code: 0x79, bits: 7 }, HpackHuffCode{ code: 0x7a, bits: 7 },
    HpackHuffCode{ code: 0x7b, bits: 7 }, HpackHuffCode{ code: 0x7ffe, bits: 15 },
    HpackHuffCode{ code: 0x7fc, bits: 11 }, HpackHuffCode{ code: 0x3ffd, bits: 14 },
    HpackHuffCode{ code: 0x1ffd, bits: 13 }, HpackHuffCode{ code: 0xffffffc, bits: 28 },
    HpackHuffCode{ code: 0xfffe6, bits: 20 }, HpackHuffCode{ code: 0x3fffd2, bits: 22 },
    HpackHuffCode{ code: 0xfffe7, bits: 20 }, HpackHuffCode{ code: 0xfffe8, bits: 20 },
    HpackHuffCode{ code: 0x3fffd3, bits: 22 }, HpackHuffCode{ code: 0x3fffd4, bits: 22 },
    HpackHuffCode{ code: 0x3fffd5, bits: 22 }, HpackHuffCode{ code: 0x7fffd9, bits: 23 },
    HpackHuffCode{ code: 0x3fffd6, bits: 22 }, HpackHuffCode{ code: 0x7fffda, bits: 23 },
    HpackHuffCode{ code: 0x7fffdb, bits: 23 }, HpackHuffCode{ code: 0x7fffdc, bits: 23 },
    HpackHuffCode{ code: 0x7fffdd, bits: 23 }, HpackHuffCode{ code: 0x7fffde, bits: 23 },
    HpackHuffCode{ code: 0xffffeb, bits: 24 }, HpackHuffCode{ code: 0x7fffdf, bits: 23 },
    HpackHuffCode{ code: 0xffffec, bits: 24 }, HpackHuffCode{ code: 0xffffed, bits: 24 },
    HpackHuffCode{ code: 0x3fffd7, bits: 22 }, HpackHuffCode{ code: 0x7fffe0, bits: 23 },
    HpackHuffCode{ code: 0xffffee, bits: 24 }, HpackHuffCode{ code: 0x7fffe1, bits: 23 },
    HpackHuffCode{ code: 0x7fffe2, bits: 23 }, HpackHuffCode{ code: 0x7fffe3, bits: 23 },
    HpackHuffCode{ code: 0x7fffe4, bits: 23 }, HpackHuffCode{ code: 0x1fffdc, bits: 21 },
    HpackHuffCode{ code: 0x3fffd8, bits: 22 }, HpackHuffCode{ code: 0x7fffe5, bits: 23 },
    HpackHuffCode{ code: 0x3fffd9, bits: 22 }, HpackHuffCode{ code: 0x7fffe6, bits: 23 },
    HpackHuffCode{ code: 0x7fffe7, bits: 23 }, HpackHuffCode{ code: 0xffffef, bits: 24 },
    HpackHuffCode{ code: 0x3fffda, bits: 22 }, HpackHuffCode{ code: 0x1fffdd, bits: 21 },
    HpackHuffCode{ code: 0xfffe9, bits: 20 }, HpackHuffCode{ code: 0x3fffdb, bits: 22 },
    HpackHuffCode{ code: 0x3fffdc, bits: 22 }, HpackHuffCode{ code: 0x7fffe8, bits: 23 },
    HpackHuffCode{ code: 0x7fffe9, bits: 23 }, HpackHuffCode{ code: 0x1fffde, bits: 21 },
    HpackHuffCode{ code: 0x7fffea, bits: 23 }, HpackHuffCode{ code: 0x3fffdd, bits: 22 },
    HpackHuffCode{ code: 0x3fffde, bits: 22 }, HpackHuffCode{ code: 0xfffff0, bits: 24 },
    HpackHuffCode{ code: 0x1fffdf, bits: 21 }, HpackHuffCode{ code: 0x3fffdf, bits: 22 },
    HpackHuffCode{ code: 0x7fffeb, bits: 23 }, HpackHuffCode{ code: 0x7fffec, bits: 23 },
    HpackHuffCode{ code: 0x1fffe0, bits: 21 }, HpackHuffCode{ code: 0x1fffe1, bits: 21 },
    HpackHuffCode{ code: 0x3fffe0, bits: 22 }, HpackHuffCode{ code: 0x1fffe2, bits: 21 },
    HpackHuffCode{ code: 0x7fffed, bits: 23 }, HpackHuffCode{ code: 0x3fffe1, bits: 22 },
    HpackHuffCode{ code: 0x7fffee, bits: 23 }, HpackHuffCode{ code: 0x7fffef, bits: 23 },
    HpackHuffCode{ code: 0xfffea, bits: 20 }, HpackHuffCode{ code: 0x3fffe2, bits: 22 },
    HpackHuffCode{ code: 0x3fffe3, bits: 22 }, HpackHuffCode{ code: 0x3fffe4, bits: 22 },
    HpackHuffCode{ code: 0x7ffff0, bits: 23 }, HpackHuffCode{ code: 0x3fffe5, bits: 22 },
    HpackHuffCode{ code: 0x3fffe6, bits: 22 }, HpackHuffCode{ code: 0x7ffff1, bits: 23 },
    HpackHuffCode{ code: 0x3ffffe0, bits: 26 }, HpackHuffCode{ code: 0x3ffffe1, bits: 26 },
    HpackHuffCode{ code: 0xfffeb, bits: 20 }, HpackHuffCode{ code: 0x7fff1, bits: 19 },
    HpackHuffCode{ code: 0x3fffe7, bits: 22 }, HpackHuffCode{ code: 0x7ffff2, bits: 23 },
    HpackHuffCode{ code: 0x3fffe8, bits: 22 }, HpackHuffCode{ code: 0x1ffffec, bits: 25 },
    HpackHuffCode{ code: 0x3ffffe2, bits: 26 }, HpackHuffCode{ code: 0x3ffffe3, bits: 26 },
    HpackHuffCode{ code: 0x3ffffe4, bits: 26 }, HpackHuffCode{ code: 0x7ffffde, bits: 27 },
    HpackHuffCode{ code: 0x7ffffdf, bits: 27 }, HpackHuffCode{ code: 0x3ffffe5, bits: 26 },
    HpackHuffCode{ code: 0xfffff1, bits: 24 }, HpackHuffCode{ code: 0x1ffffed, bits: 25 },
    HpackHuffCode{ code: 0x7fff2, bits: 19 }, HpackHuffCode{ code: 0x1fffe3, bits: 21 },
    HpackHuffCode{ code: 0x3ffffe6, bits: 26 }, HpackHuffCode{ code: 0x7ffffe0, bits: 27 },
    HpackHuffCode{ code: 0x7ffffe1, bits: 27 }, HpackHuffCode{ code: 0x3ffffe7, bits: 26 },
    HpackHuffCode{ code: 0x7ffffe2, bits: 27 }, HpackHuffCode{ code: 0xfffff2, bits: 24 },
    HpackHuffCode{ code: 0x1fffe4, bits: 21 }, HpackHuffCode{ code: 0x1fffe5, bits: 21 },
    HpackHuffCode{ code: 0x3ffffe8, bits: 26 }, HpackHuffCode{ code: 0x3ffffe9, bits: 26 },
    HpackHuffCode{ code: 0xffffffd, bits: 28 }, HpackHuffCode{ code: 0x7ffffe3, bits: 27 },
    HpackHuffCode{ code: 0x7ffffe4, bits: 27 }, HpackHuffCode{ code: 0x7ffffe5, bits: 27 },
    HpackHuffCode{ code: 0xfffec, bits: 20 }, HpackHuffCode{ code: 0xfffff3, bits: 24 },
    HpackHuffCode{ code: 0xfffed, bits: 20 }, HpackHuffCode{ code: 0x1fffe6, bits: 21 },
    HpackHuffCode{ code: 0x3fffe9, bits: 22 }, HpackHuffCode{ code: 0x1fffe7, bits: 21 },
    HpackHuffCode{ code: 0x1fffe8, bits: 21 }, HpackHuffCode{ code: 0x7ffff3, bits: 23 },
    HpackHuffCode{ code: 0x3fffea, bits: 22 }, HpackHuffCode{ code: 0x3fffeb, bits: 22 },
    HpackHuffCode{ code: 0x1ffffee, bits: 25 }, HpackHuffCode{ code: 0x1ffffef, bits: 25 },
    HpackHuffCode{ code: 0xfffff4, bits: 24 }, HpackHuffCode{ code: 0xfffff5, bits: 24 },
    HpackHuffCode{ code: 0x3ffffea, bits: 26 }, HpackHuffCode{ code: 0x7ffff4, bits: 23 },
    HpackHuffCode{ code: 0x3ffffeb, bits: 26 }, HpackHuffCode{ code: 0x7ffffe6, bits: 27 },
    HpackHuffCode{ code: 0x3ffffec, bits: 26 }, HpackHuffCode{ code: 0x3ffffed, bits: 26 },
    HpackHuffCode{ code: 0x7ffffe7, bits: 27 }, HpackHuffCode{ code: 0x7ffffe8, bits: 27 },
    HpackHuffCode{ code: 0x7ffffe9, bits: 27 }, HpackHuffCode{ code: 0x7ffffea, bits: 27 },
    HpackHuffCode{ code: 0x7ffffeb, bits: 27 }, HpackHuffCode{ code: 0xffffffe, bits: 28 },
    HpackHuffCode{ code: 0x7ffffec, bits: 27 }, HpackHuffCode{ code: 0x7ffffed, bits: 27 },
    HpackHuffCode{ code: 0x7ffffee, bits: 27 }, HpackHuffCode{ code: 0x7ffffef, bits: 27 },
    HpackHuffCode{ code: 0x7fffff0, bits: 27 }, HpackHuffCode{ code: 0x3ffffee, bits: 26 },
    HpackHuffCode{ code: 0x3fffffff, bits: 30 },
];

pub const HPACK_HUFF_BUCKETS: [31]HpackHuffBucket = [
    HpackHuffBucket{ min_code: 0, max_code: 0, offset: 0, count: 0 },
    HpackHuffBucket{ min_code: 0, max_code: 0, offset: 0, count: 0 },
    HpackHuffBucket{ min_code: 0, max_code: 0, offset: 0, count: 0 },
    HpackHuffBucket{ min_code: 0, max_code: 0, offset: 0, count: 0 },
    HpackHuffBucket{ min_code: 0, max_code: 0, offset: 0, count: 0 },
    HpackHuffBucket{ min_code: 0, max_code: 9, offset: 0, count: 10 },
    HpackHuffBucket{ min_code: 20, max_code: 45, offset: 10, count: 26 },
    HpackHuffBucket{ min_code: 92, max_code: 123, offset: 36, count: 32 },
    HpackHuffBucket{ min_code: 248, max_code: 253, offset: 68, count: 6 },
    HpackHuffBucket{ min_code: 0, max_code: 0, offset: 0, count: 0 },
    HpackHuffBucket{ min_code: 1016, max_code: 1020, offset: 74, count: 5 },
    HpackHuffBucket{ min_code: 2042, max_code: 2044, offset: 79, count: 3 },
    HpackHuffBucket{ min_code: 4090, max_code: 4091, offset: 82, count: 2 },
    HpackHuffBucket{ min_code: 8184, max_code: 8189, offset: 84, count: 6 },
    HpackHuffBucket{ min_code: 16380, max_code: 16381, offset: 90, count: 2 },
    HpackHuffBucket{ min_code: 32764, max_code: 32766, offset: 92, count: 3 },
    HpackHuffBucket{ min_code: 0, max_code: 0, offset: 0, count: 0 },
    HpackHuffBucket{ min_code: 0, max_code: 0, offset: 0, count: 0 },
    HpackHuffBucket{ min_code: 0, max_code: 0, offset: 0, count: 0 },
    HpackHuffBucket{ min_code: 524272, max_code: 524274, offset: 95, count: 3 },
    HpackHuffBucket{ min_code: 1048550, max_code: 1048557, offset: 98, count: 8 },
    HpackHuffBucket{ min_code: 2097116, max_code: 2097128, offset: 106, count: 13 },
    HpackHuffBucket{ min_code: 4194258, max_code: 4194283, offset: 119, count: 26 },
    HpackHuffBucket{ min_code: 8388568, max_code: 8388596, offset: 145, count: 29 },
    HpackHuffBucket{ min_code: 16777194, max_code: 16777205, offset: 174, count: 12 },
    HpackHuffBucket{ min_code: 33554412, max_code: 33554415, offset: 186, count: 4 },
    HpackHuffBucket{ min_code: 67108832, max_code: 67108846, offset: 190, count: 15 },
    HpackHuffBucket{ min_code: 134217694, max_code: 134217712, offset: 205, count: 19 },
    HpackHuffBucket{ min_code: 268435426, max_code: 268435454, offset: 224, count: 29 },
    HpackHuffBucket{ min_code: 0, max_code: 0, offset: 0, count: 0 },
    HpackHuffBucket{ min_code: 1073741820, max_code: 1073741823, offset: 253, count: 4 },
];

pub const HPACK_HUFF_SORTED_SYMS: [257]u16 = [
    48, 49, 50, 97, 99, 101, 105, 111, 115, 116, 32, 37, 45, 46, 47, 51, 52, 53, 54, 55,
    56, 57, 61, 65, 95, 98, 100, 102, 103, 104, 108, 109, 110, 112, 114, 117, 58, 66, 67, 68,
    69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 89,
    106, 107, 113, 118, 119, 120, 121, 122, 38, 42, 44, 59, 88, 90, 33, 34, 40, 41, 63, 39,
    43, 124, 35, 62, 0, 36, 64, 91, 93, 126, 94, 125, 60, 96, 123, 92, 195, 208, 128, 130,
    131, 162, 184, 194, 224, 226, 153, 161, 167, 172, 176, 177, 179, 209, 216, 217, 227, 229, 230, 129,
    132, 133, 134, 136, 146, 154, 156, 160, 163, 164, 169, 170, 173, 178, 181, 185, 186, 187, 189, 190,
    196, 198, 228, 232, 233, 1, 135, 137, 138, 139, 140, 141, 143, 147, 149, 150, 151, 152, 155, 157,
    158, 165, 166, 168, 174, 175, 180, 182, 183, 188, 191, 197, 231, 239, 9, 142, 144, 145, 148, 159,
    171, 206, 215, 225, 236, 237, 199, 207, 234, 235, 192, 193, 200, 201, 202, 205, 210, 213, 218, 219,
    238, 240, 242, 243, 255, 203, 204, 211, 212, 214, 221, 222, 223, 241, 244, 245, 246, 247, 248, 250,
    251, 252, 253, 254, 2, 3, 4, 5, 6, 7, 8, 11, 12, 14, 15, 16, 17, 18, 19, 20,
    21, 23, 24, 25, 26, 27, 28, 29, 30, 31, 127, 220, 249, 10, 13, 22, 256,
];

pub struct HpackCodec {
    /// RFC 7541 §5.1 Integer Representation Encoder.
    pub static fn encodePrefixInt(out: []u8, value: usize, prefix_bits: u8, first_byte_mask: u8): usize {
        if (out.len == 0 || prefix_bits == 0 || prefix_bits > 8) {
            return 0;
        }
        const max_prefix: usize = ((1 as usize) << prefix_bits) - 1;
        if (value < max_prefix) {
            out[0] = first_byte_mask | (value as u8);
            return 1;
        }
        out[0] = first_byte_mask | (max_prefix as u8);
        let rem = value - max_prefix;
        let pos: usize = 1;
        while (rem >= 128) {
            if (pos >= out.len) { return 0; }
            out[pos] = ((rem & 0x7F) as u8) | 0x80;
            pos += 1;
            rem = rem >> 7;
        }
        if (pos >= out.len) { return 0; }
        out[pos] = (rem & 0x7F) as u8;
        return pos + 1;
    }

    /// RFC 7541 §5.1 Integer Representation Decoder.
    pub static fn decodePrefixInt(buf: []const u8, prefix_bits: u8): HpackIntResult? {
        if (buf.len == 0 || prefix_bits == 0 || prefix_bits > 8) {
            return null;
        }
        const max_prefix: usize = ((1 as usize) << prefix_bits) - 1;
        let value: usize = (buf[0] as usize) & max_prefix;
        if (value < max_prefix) {
            return HpackIntResult {
                value: value,
                bytes_read: 1,
            };
        }
        let shift: usize = 0;
        let pos: usize = 1;
        while (pos < buf.len) {
            const b = buf[pos];
            pos += 1;
            value += ((b & 0x7F) as usize) << shift;
            if ((b & 0x80) == 0) {
                return HpackIntResult {
                    value: value,
                    bytes_read: pos,
                };
            }
            if (shift >= 56) {
                return null;
            }
            shift += 7;
        }
        return null;
    }

    /// RFC 7541 Appendix A Static Table Lookup (1..61).
    pub static fn lookupStatic(index: usize): HpackStaticEntry? {
        if (index == 0 || index > HPACK_STATIC_TABLE.len) {
            return null;
        }
        return HPACK_STATIC_TABLE[index - 1];
    }

    /// Finds an exact (name + value) match in the complete 61-entry RFC 7541 static table, returning 0 if not found.
    pub static fn findStaticExact(name: []const u8, value: []const u8): usize {
        let i: usize = 0;
        while (i < HPACK_STATIC_TABLE.len) : (i += 1) {
            const e = HPACK_STATIC_TABLE[i];
            if (std.mem.eql(u8, e.name, name) && std.mem.eql(u8, e.value, value)) {
                return e.index;
            }
        }
        return 0;
    }

    /// Finds the first matching header name in the RFC 7541 static table, returning 0 if not found.
    pub static fn findStaticName(name: []const u8): usize {
        let i: usize = 0;
        while (i < HPACK_STATIC_TABLE.len) : (i += 1) {
            if (std.mem.eql(u8, HPACK_STATIC_TABLE[i].name, name)) {
                return HPACK_STATIC_TABLE[i].index;
            }
        }
        return 0;
    }

    /// Computes exact Huffman-encoded byte length for `str`.
    pub static fn huffmanEncodedLen(str: []const u8): usize {
        let total_bits: usize = 0;
        for (ch of str) {
            total_bits += HPACK_HUFFMAN_TABLE[ch as usize].bits as usize;
        }
        return (total_bits + 7) / 8;
    }

    /// RFC 7541 §5.2 Huffman Encoder (MSB-first, padded with EOS 1-bits).
    pub static fn encodeHuffman(out: []u8, str: []const u8): usize {
        const needed = HpackCodec.huffmanEncodedLen(str);
        if (out.len < needed) return 0;

        let bits: u64 = 0;
        let nbits: u8 = 0;
        let pos: usize = 0;
        for (ch of str) {
            const entry = HPACK_HUFFMAN_TABLE[ch as usize];
            const sh_bits: u6 = @intCast(entry.bits);
            bits = (bits << sh_bits) | (entry.code as u64);
            nbits += entry.bits;
            while (nbits >= 8) {
                nbits -= (8 as u8);
                const sh_out: u6 = @intCast(nbits);
                out[pos] = ((bits >> sh_out) & 0xFF) as u8;
                pos += 1;
            }
        }
        if (nbits > 0) {
            const rem_shift: u8 = (8 as u8) - nbits;
            const sh_rem: u6 = @intCast(rem_shift);
            const pad: u64 = ((1 as u64) << sh_rem) - 1;
            out[pos] = (((bits << sh_rem) | pad) & 0xFF) as u8;
            pos += 1;
        }
        return pos;
    }

    /// RFC 7541 §5.2 Canonical Huffman Decoder with strict EOS and padding validation.
    pub static fn decodeHuffman(out: []u8, src: []const u8): usize! {
        let acc: u32 = 0;
        let bit_len: usize = 0;
        let out_pos: usize = 0;

        for (byte_val of src) {
            let b_idx: usize = 8;
            while (b_idx > 0) {
                b_idx -= 1;
                const sh_b: u3 = @intCast(b_idx);
                const bit: u32 = ((byte_val >> sh_b) & 1) as u32;
                acc = (acc << 1) | bit;
                bit_len += 1;
                if (bit_len > 30) {
                    return error.InvalidHuffmanCode;
                }
                const bucket = HPACK_HUFF_BUCKETS[bit_len];
                if (bucket.count > 0 && acc >= bucket.min_code && acc <= bucket.max_code) {
                    const sym_idx = (bucket.offset as usize) + ((acc - bucket.min_code) as usize);
                    const sym = HPACK_HUFF_SORTED_SYMS[sym_idx];
                    if (sym == 256) {
                        return error.InvalidHuffmanEOS;
                    }
                    if (out_pos >= out.len) {
                        return error.BufferTooSmall;
                    }
                    out[out_pos] = sym as u8;
                    out_pos += 1;
                    acc = 0;
                    bit_len = 0;
                }
            }
        }

        if (bit_len > 7) {
            return error.InvalidHuffmanPadding;
        }
        if (bit_len > 0) {
            const sh_p: u5 = @intCast(bit_len);
            const expected_pad: u32 = ((1 as u32) << sh_p) - 1;
            if (acc != expected_pad) {
                return error.InvalidHuffmanPadding;
            }
        }
        return out_pos;
    }

    /// Encodes a non-Huffman string literal (7-bit length prefix + raw octets).
    pub static fn encodeStringLiteral(out: []u8, str: []const u8): usize {
        const hdr_len = HpackCodec.encodePrefixInt(out, str.len, 7, 0x00);
        if (hdr_len == 0 || out.len < hdr_len + str.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < str.len) {
            out[hdr_len + i] = str[i];
            i += 1;
        }
        return hdr_len + str.len;
    }

    /// Encodes a string literal using Huffman compression if `use_huffman` is true (and shorter or forced).
    pub static fn encodeStringLiteralOpt(out: []u8, str: []const u8, use_huffman: bool): usize {
        if (!use_huffman) {
            return HpackCodec.encodeStringLiteral(out, str);
        }
        const huff_len = HpackCodec.huffmanEncodedLen(str);
        if (huff_len >= str.len) {
            return HpackCodec.encodeStringLiteral(out, str);
        }
        const hdr_len = HpackCodec.encodePrefixInt(out, huff_len, 7, 0x80);
        if (hdr_len == 0 || out.len < hdr_len + huff_len) {
            return 0;
        }
        const written = HpackCodec.encodeHuffman(out[hdr_len..], str);
        if (written != huff_len) return 0;
        return hdr_len + written;
    }

    /// Decodes a non-Huffman string literal (zero-copy).
    pub static fn decodeStringLiteral(buf: []const u8): HpackDecodedHeader? {
        if (buf.len == 0) { return null; }
        if ((buf[0] & 0x80) != 0) { return null; }
        const len_res_opt = HpackCodec.decodePrefixInt(buf, 7);
        if (len_res_opt == null) { return null; }
        const len_res = len_res_opt.?;
        const total = len_res.bytes_read + len_res.value;
        if (buf.len < total) { return null; }
        return HpackDecodedHeader {
            name: "",
            value: buf[len_res.bytes_read .. total],
            never_index: false,
            bytes_read: total,
        };
    }

    /// Decodes either a plain or Huffman-encoded string literal into `scratch`, returning the string slice and bytes consumed.
    pub static fn decodeStringInto(buf: []const u8, scratch: []u8): HpackDecodedHeader? {
        if (buf.len == 0) return null;
        const is_huff = (buf[0] & 0x80) != 0;
        const len_res = HpackCodec.decodePrefixInt(buf, 7) ?? return null;
        const total = len_res.bytes_read + len_res.value;
        if (buf.len < total) return null;
        const raw_bytes = buf[len_res.bytes_read .. total];
        if (!is_huff) {
            return HpackDecodedHeader{
                name: "",
                value: raw_bytes,
                never_index: false,
                bytes_read: total,
            };
        }
        const dec_len = HpackCodec.decodeHuffman(scratch, raw_bytes) catch return null;
        return HpackDecodedHeader{
            name: "",
            value: scratch[0..dec_len],
            never_index: false,
            bytes_read: total,
        };
    }

    /// Encodes an Indexed Header Field Representation (`0x80` bit mask, 7-bit prefix).
    pub static fn encodeIndexed(out: []u8, index: usize): usize {
        if (index == 0) { return 0; }
        return HpackCodec.encodePrefixInt(out, index, 7, 0x80);
    }

    /// Encodes a header field using static table match or Literal without/never indexing.
    pub static fn encodeHeader(out: []u8, name: []const u8, value: []const u8, never_index: bool): usize {
        if (!never_index) {
            const exact_idx = HpackCodec.findStaticExact(name, value);
            if (exact_idx > 0) {
                return HpackCodec.encodeIndexed(out, exact_idx);
            }
        }
        if (out.len < 3) { return 0; }
        out[0] = if (never_index) (0x10 as u8) else (0x00 as u8);
        let pos: usize = 1;
        const n_len = HpackCodec.encodeStringLiteral(out[pos..], name);
        if (n_len == 0) { return 0; }
        pos += n_len;
        const v_len = HpackCodec.encodeStringLiteral(out[pos..], value);
        if (v_len == 0) { return 0; }
        return pos + v_len;
    }

    /// Decodes a single stateless HPACK header field representation from `buf`.
    pub static fn decodeHeader(buf: []const u8): HpackDecodedHeader? {
        if (buf.len == 0) { return null; }
        const first = buf[0];

        if ((first & 0x80) != 0) {
            const idx_res_opt = HpackCodec.decodePrefixInt(buf, 7);
            if (idx_res_opt == null) { return null; }
            const idx_res = idx_res_opt.?;
            const entry_opt = HpackCodec.lookupStatic(idx_res.value);
            if (entry_opt == null) { return null; }
            const entry = entry_opt.?;
            return HpackDecodedHeader {
                name: entry.name,
                value: entry.value,
                never_index: false,
                bytes_read: idx_res.bytes_read,
            };
        }

        const never_idx = (first & 0xF0) == 0x10;
        const prefix_bits: u8 = if ((first & 0x40) != 0) (6 as u8) else (4 as u8);
        const name_idx_opt = HpackCodec.decodePrefixInt(buf, prefix_bits);
        if (name_idx_opt == null) { return null; }
        const name_idx = name_idx_opt.?;
        let pos: usize = name_idx.bytes_read;

        let header_name: []const u8 = "";
        if (name_idx.value > 0) {
            const st_opt = HpackCodec.lookupStatic(name_idx.value);
            if (st_opt == null) { return null; }
            header_name = st_opt.?.name;
        } else {
            const name_str_opt = HpackCodec.decodeStringLiteral(buf[pos..]);
            if (name_str_opt == null) { return null; }
            const name_str = name_str_opt.?;
            header_name = name_str.value;
            pos += name_str.bytes_read;
        }

        const val_str_opt = HpackCodec.decodeStringLiteral(buf[pos..]);
        if (val_str_opt == null) { return null; }
        const val_str = val_str_opt.?;
        pos += val_str.bytes_read;

        return HpackDecodedHeader {
            name: header_name,
            value: val_str.value,
            never_index: never_idx,
            bytes_read: pos,
        };
    }
}

// ============================================================================
// RFC 7541 §4 Dynamic Table + Stateful Encoder/Decoder
// ============================================================================

pub enum HpackIndexMode: u8 {
    Incremental = 0,
    WithoutIndexing = 1,
    NeverIndexed = 2,
}

pub struct HpackDynamicEntry {
    pub name: []u8 = [],
    pub value: []u8 = [],

    pub fn byteSize(): usize {
        return _.name.len + _.value.len + 32;
    }
}

pub const MAX_HPACK_DYNAMIC_ENTRIES: usize = 256;

pub struct HpackDynamicTable {
    pub entries: [MAX_HPACK_DYNAMIC_ENTRIES]HpackDynamicEntry = undefined;
    pub count: usize = 0;
    pub current_size: usize = 0;
    pub max_size: usize = 4096;
    pub protocol_max_size: usize = 4096;
    pub allocator: std.mem.Allocator = undefined;
    pub evictions: u64 = 0;

    pub static fn init(allocator: std.mem.Allocator, max_size: usize = 4096): HpackDynamicTable {
        return HpackDynamicTable{
            entries: undefined,
            count: 0,
            current_size: 0,
            max_size: max_size,
            protocol_max_size: max_size,
            allocator: allocator,
            evictions: 0,
        };
    }

    pub fn deinit(*_) {
        _.clear();
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            _.allocator.free(_.entries[i].name);
            _.allocator.free(_.entries[i].value);
        }
        _.count = 0;
        _.current_size = 0;
    }

    fn evictOne(*_): bool {
        if (_.count == 0) return false;
        const last = _.count - 1;
        const sz = _.entries[last].byteSize();
        _.allocator.free(_.entries[last].name);
        _.allocator.free(_.entries[last].value);
        _.count -= 1;
        _.current_size -= sz;
        _.evictions += 1;
        return true;
    }

    /// RFC 7541 §4.3: Updates dynamic table max size, evicting oldest entries until `current_size <= new_max`.
    pub fn setMaxSize(*_, new_max: usize): bool {
        if (new_max > _.protocol_max_size) return false;
        _.max_size = new_max;
        while (_.current_size > _.max_size && _.count > 0) {
            * = _.evictOne();
        }
        return true;
    }

    /// RFC 7541 §4.4: Inserts at index 0 (dynamic index 62); clears table if `entry_size > max_size`.
    pub fn insert(*_, name: []const u8, value: []const u8): bool {
        const entry_size = name.len + value.len + 32;
        if (entry_size > _.max_size) {
            _.clear();
            return true;
        }
        while ((_.current_size + entry_size > _.max_size || _.count >= MAX_HPACK_DYNAMIC_ENTRIES) && _.count > 0) {
            * = _.evictOne();
        }
        if (_.count >= MAX_HPACK_DYNAMIC_ENTRIES) return false;

        const n_dup = _.allocator.dupe(u8, name) catch return false;
        const v_dup = _.allocator.dupe(u8, value) catch {
            _.allocator.free(n_dup);
            return false;
        };

        // Shift right by 1 so newest entry sits at slot 0 (index 62)
        let i = _.count;
        while (i > 0) {
            _.entries[i] = _.entries[i - 1];
            i -= 1;
        }
        _.entries[0] = HpackDynamicEntry{
            name: n_dup,
            value: v_dup,
        };
        _.count += 1;
        _.current_size += entry_size;
        return true;
    }

    /// Looks up combined static (1..61) + dynamic (62..) HPACK index space.
    pub fn lookup(index: usize): HpackStaticEntry? {
        if (index == 0) return null;
        if (index <= HPACK_STATIC_TABLE.len) {
            return HPACK_STATIC_TABLE[index - 1];
        }
        const dyn_idx = index - HPACK_STATIC_TABLE.len - 1;
        if (dyn_idx >= _.count) return null;
        return HpackStaticEntry{
            index: index,
            name: _.entries[dyn_idx].name,
            value: _.entries[dyn_idx].value,
        };
    }

    /// Finds exact (name, value) match across static (1..61) and dynamic (62..) tables.
    pub fn findExact(name: []const u8, value: []const u8): usize {
        const st = HpackCodec.findStaticExact(name, value);
        if (st > 0) return st;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (std.mem.eql(u8, _.entries[i].name, name) && std.mem.eql(u8, _.entries[i].value, value)) {
                return HPACK_STATIC_TABLE.len + 1 + i;
            }
        }
        return 0;
    }

    /// Finds name match across static (1..61) and dynamic (62..) tables.
    pub fn findName(name: []const u8): usize {
        const st = HpackCodec.findStaticName(name);
        if (st > 0) return st;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (std.mem.eql(u8, _.entries[i].name, name)) {
                return HPACK_STATIC_TABLE.len + 1 + i;
            }
        }
        return 0;
    }
}

pub struct HpackSessionCodec {
    pub enc_table: HpackDynamicTable = undefined,
    pub dec_table: HpackDynamicTable = undefined;

    pub static fn init(allocator: std.mem.Allocator, max_table_size: usize = 4096): HpackSessionCodec {
        return HpackSessionCodec{
            enc_table: HpackDynamicTable.init(allocator, max_table_size),
            dec_table: HpackDynamicTable.init(allocator, max_table_size),
        };
    }

    pub fn deinit(*_) {
        _.enc_table.deinit();
        _.dec_table.deinit();
    }

    /// Encodes a Dynamic Table Size Update (`0x20` 5-bit prefix) and updates encoder table.
    pub fn encodeTableSizeUpdate(*_, out: []u8, new_max_size: usize): usize {
        if (!_.enc_table.setMaxSize(new_max_size)) return 0;
        return HpackCodec.encodePrefixInt(out, new_max_size, 5, 0x20);
    }

    /// Encodes a header using combined static + dynamic table, optional Huffman compression, and indexing mode.
    pub fn encodeHeader(
        *_,
        out: []u8,
        name: []const u8,
        value: []const u8,
        mode: HpackIndexMode,
        use_huffman: bool,
    ): usize {
        if (mode != HpackIndexMode.NeverIndexed) {
            const exact = _.enc_table.findExact(name, value);
            if (exact > 0) {
                return HpackCodec.encodeIndexed(out, exact);
            }
        }

        const prefix_mask: u8 = switch (mode) {
            .Incremental => 0x40,
            .WithoutIndexing => 0x00,
            .NeverIndexed => 0x10,
        };
        const prefix_bits: u8 = if (mode == HpackIndexMode.Incremental) (6 as u8) else (4 as u8);
        const name_idx = _.enc_table.findName(name);

        let pos = HpackCodec.encodePrefixInt(out, name_idx, prefix_bits, prefix_mask);
        if (pos == 0) return 0;

        if (name_idx == 0) {
            const n_len = HpackCodec.encodeStringLiteralOpt(out[pos..], name, use_huffman);
            if (n_len == 0) return 0;
            pos += n_len;
        }
        const v_len = HpackCodec.encodeStringLiteralOpt(out[pos..], value, use_huffman);
        if (v_len == 0) return 0;
        pos += v_len;

        if (mode == HpackIndexMode.Incremental) {
            * = _.enc_table.insert(name, value);
        }
        return pos;
    }

    /// Decodes a single HPACK header or Dynamic Table Size Update, populating `name_scratch` / `val_scratch`.
    /// Returns `null` on error; if the representation was a Dynamic Table Size Update, returns `name == ""`, `value == ""`, `bytes_read > 0`.
    pub fn decodeHeader(*_, buf: []const u8, name_scratch: []u8, val_scratch: []u8): HpackDecodedHeader? {
        if (buf.len == 0) return null;
        const first = buf[0];

        // 1. Indexed Header Field (`1xxxxxxx`)
        if ((first & 0x80) != 0) {
            const idx_res = HpackCodec.decodePrefixInt(buf, 7) ?? return null;
            const entry = _.dec_table.lookup(idx_res.value) ?? return null;
            return HpackDecodedHeader{
                name: entry.name,
                value: entry.value,
                never_index: false,
                bytes_read: idx_res.bytes_read,
            };
        }

        // 2. Dynamic Table Size Update (`001xxxxx`)
        if ((first & 0xE0) == 0x20) {
            const sz_res = HpackCodec.decodePrefixInt(buf, 5) ?? return null;
            if (!_.dec_table.setMaxSize(sz_res.value)) return null;
            return HpackDecodedHeader{
                name: "",
                value: "",
                never_index: false,
                bytes_read: sz_res.bytes_read,
            };
        }

        // 3. Literal Header Field (Incremental `01xxxxxx`, Without `0000xxxx`, Never `0001xxxx`)
        const incremental = (first & 0x40) != 0;
        const never_idx = !incremental && ((first & 0xF0) == 0x10);
        const prefix_bits: u8 = if (incremental) (6 as u8) else (4 as u8);
        const name_idx = HpackCodec.decodePrefixInt(buf, prefix_bits) ?? return null;
        let pos = name_idx.bytes_read;

        let hdr_name: []const u8 = "";
        if (name_idx.value > 0) {
            const entry = _.dec_table.lookup(name_idx.value) ?? return null;
            hdr_name = entry.name;
        } else {
            const name_dec = HpackCodec.decodeStringInto(buf[pos..], name_scratch) ?? return null;
            hdr_name = name_dec.value;
            pos += name_dec.bytes_read;
        }

        const val_dec = HpackCodec.decodeStringInto(buf[pos..], val_scratch) ?? return null;
        pos += val_dec.bytes_read;

        if (incremental) {
            if (!_.dec_table.insert(hdr_name, val_dec.value)) return null;
        }

        return HpackDecodedHeader{
            name: hdr_name,
            value: val_dec.value,
            never_index: never_idx,
            bytes_read: pos,
        };
    }
}
