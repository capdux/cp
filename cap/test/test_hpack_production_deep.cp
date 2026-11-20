// Source: cp/cap/test/test_hpack_production_deep.cp
// Deep Production Verification Suite for cap:hpack & cap.hpack
// Covers:
//   1. RFC 7541 multi-request roundtrip encoding/decoding with static & dynamic table indexing
//   2. RFC 7541 Appendix B Huffman exact encoding/decoding & malformed EOS/padding rejection
//   3. Dynamic table FIFO eviction, oversized entry clear, and setMaxSize() shrinking
//   4. WithoutIndexing / NeverIndexed sensitive header protection & TableSizeExceeded guard
//   5. `import cap from "cap"; cap.hpack` dynamic mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HpackError,
    HeaderField,
    IndexingMode,
    DynamicTable,
    HpackEncoder,
    HpackDecoder,
    huffmanEncode,
    huffmanDecode,
    getStaticEntry,
    encodeInt,
    decodeInt,
} from "cap:hpack";

test("cap:hpack - RFC 7541 roundtrip & static/dynamic table indexing") {
    const alloc = std.testing.allocator;

    // Verify RFC 7541 Appendix A static table boundary entries
    const s1 = getStaticEntry(1) orelse unreachable;
    expect_eq(std.mem.eql(u8, s1.name, ":authority"), true);
    expect_eq(std.mem.eql(u8, s1.value, ""), true);
    const s2 = getStaticEntry(2) orelse unreachable;
    expect_eq(std.mem.eql(u8, s2.name, ":method"), true);
    expect_eq(std.mem.eql(u8, s2.value, "GET"), true);
    const s61 = getStaticEntry(61) orelse unreachable;
    expect_eq(std.mem.eql(u8, s61.name, "www-authenticate"), true);
    expect_eq(getStaticEntry(0) == null, true);
    expect_eq(getStaticEntry(62) == null, true);

    let encoder = HpackEncoder.init(alloc, 4096);
    defer encoder.deinit();
    let decoder = HpackDecoder.init(alloc, 4096);
    defer decoder.deinit();

    // First request (RFC 7541 C.3/C.4 style headers)
    const req1 = [
        HeaderField.init(":method", "GET"),
        HeaderField.init(":scheme", "http"),
        HeaderField.init(":path", "/"),
        HeaderField.init(":authority", "www.example.com"),
        HeaderField.init("custom-key", "custom-header"),
    ];
    const wire1 = encoder.encode(req1[0..]) catch unreachable;
    defer alloc.free(wire1);

    let decoded1 = decoder.decode(wire1) catch unreachable;
    defer decoded1.deinit();
    expect_eq(decoded1.len(), 5);
    expect_eq(std.mem.eql(u8, decoded1.get(0).?.value, "GET"), true);
    expect_eq(std.mem.eql(u8, decoded1.get(3).?.name, ":authority"), true);
    expect_eq(std.mem.eql(u8, decoded1.get(3).?.value, "www.example.com"), true);
    expect_eq(std.mem.eql(u8, decoded1.get(4).?.name, "custom-key"), true);
    expect_eq(std.mem.eql(u8, decoded1.get(4).?.value, "custom-header"), true);

    // Dynamic table should now contain ":authority: www.example.com" and "custom-key: custom-header"
    expect_eq(encoder.dynamicTableLen(), 2);
    expect_eq(decoder.dynamicTableLen(), 2);

    // Second request reuses dynamic table entries -> wire2 must be strictly smaller than wire1
    const req2 = [
        HeaderField.init(":method", "GET"),
        HeaderField.init(":scheme", "http"),
        HeaderField.init(":path", "/"),
        HeaderField.init(":authority", "www.example.com"),
        HeaderField.init("custom-key", "custom-header"),
        HeaderField.init("cache-control", "no-cache"),
    ];
    const wire2 = encoder.encode(req2[0..]) catch unreachable;
    defer alloc.free(wire2);
    expect_eq(wire2.len < wire1.len, true);

    let decoded2 = decoder.decode(wire2) catch unreachable;
    defer decoded2.deinit();
    expect_eq(decoded2.len(), 6);
    expect_eq(std.mem.eql(u8, decoded2.get(3).?.value, "www.example.com"), true);
    expect_eq(std.mem.eql(u8, decoded2.get(5).?.name, "cache-control"), true);
    expect_eq(std.mem.eql(u8, decoded2.get(5).?.value, "no-cache"), true);
}

test("cap:hpack - RFC 7541 Appendix B Huffman encode/decode & malformed EOS/padding rejection") {
    const alloc = std.testing.allocator;

    // RFC 7541 Appendix C.4.1: "www.example.com" -> f1e3 c2e5 f23a 6ba0 ab90 f4ff
    const h1 = huffmanEncode(alloc, "www.example.com") catch unreachable;
    defer alloc.free(h1);
    const expected_h1: [12]u8 = [0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff];
    expect_eq(std.mem.eql(u8, h1, expected_h1[0..]), true);
    const d1 = huffmanDecode(alloc, h1) catch unreachable;
    defer alloc.free(d1);
    expect_eq(std.mem.eql(u8, d1, "www.example.com"), true);

    // RFC 7541 Appendix C.4.2: "no-cache" -> a8eb 1064 9cbf
    const h2 = huffmanEncode(alloc, "no-cache") catch unreachable;
    defer alloc.free(h2);
    const expected_h2: [6]u8 = [0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf];
    expect_eq(std.mem.eql(u8, h2, expected_h2[0..]), true);
    const d2 = huffmanDecode(alloc, h2) catch unreachable;
    defer alloc.free(d2);
    expect_eq(std.mem.eql(u8, d2, "no-cache"), true);

    // Reject non-all-1s padding: '0' is 5 bits (00000), valid padded byte is 0x07 (00000_111),
    // invalid padded byte is 0x00 (00000_000).
    let non_ones_pad_err = false;
    const bad_pad: [1]u8 = [0x00];
    const r1: ([]u8)? = huffmanDecode(alloc, bad_pad[0..]) catch |e| blk: {
        if (e == HpackError.InvalidHuffman) {
            non_ones_pad_err = true;
        }
        break :blk null;
    };
    expect_eq(non_ones_pad_err, true);
    expect_eq(r1 == null, true);

    // Reject >7 bits of padding: a single 0xFF byte is 8 bits of 1s (not a valid complete symbol).
    let overlong_pad_err = false;
    const pad_8bits: [1]u8 = [0xff];
    const r2: ([]u8)? = huffmanDecode(alloc, pad_8bits[0..]) catch |e| blk: {
        if (e == HpackError.InvalidHuffman) {
            overlong_pad_err = true;
        }
        break :blk null;
    };
    expect_eq(overlong_pad_err, true);
    expect_eq(r2 == null, true);

    // Reject explicit EOS symbol (30 bits of 1s + 2 bits of 1s = 0xff, 0xff, 0xff, 0xff).
    let eos_err = false;
    const eos_bytes: [4]u8 = [0xff, 0xff, 0xff, 0xff];
    const r3: ([]u8)? = huffmanDecode(alloc, eos_bytes[0..]) catch |e| blk: {
        if (e == HpackError.InvalidHuffman) {
            eos_err = true;
        }
        break :blk null;
    };
    expect_eq(eos_err, true);
    expect_eq(r3 == null, true);
}

test("cap:hpack - dynamic table FIFO eviction, oversized entry clear, and setMaxSize") {
    const alloc = std.testing.allocator;
    let dt = DynamicTable.init(alloc, 128);
    defer dt.deinit();

    // Entry size = 4 + 4 + 32 = 40 bytes
    dt.insert("key1", "val1") catch unreachable;
    dt.insert("key2", "val2") catch unreachable;
    dt.insert("key3", "val3") catch unreachable;
    expect_eq(dt.len(), 3);
    expect_eq(dt.currentSize(), 120);

    // Inserting a 4th 40-byte entry (120 + 40 = 160 > 128) must evict "key1"
    dt.insert("key4", "val4") catch unreachable;
    expect_eq(dt.len(), 3);
    expect_eq(dt.currentSize(), 120);
    expect_eq(std.mem.eql(u8, dt.get(0).?.name, "key4"), true);
    expect_eq(std.mem.eql(u8, dt.get(2).?.name, "key2"), true);

    // setMaxSize(80) must evict oldest entry "key2" so size <= 80
    dt.setMaxSize(80);
    expect_eq(dt.len(), 2);
    expect_eq(dt.currentSize(), 80);
    expect_eq(std.mem.eql(u8, dt.get(0).?.name, "key4"), true);
    expect_eq(std.mem.eql(u8, dt.get(1).?.name, "key3"), true);

    // Inserting an entry larger than max_size (e.g. 60 + 32 = 92 > 80) clears table without error
    const big_val = "012345678901234567890123456789012345678901234567890123456789";
    dt.insert("k", big_val) catch unreachable;
    expect_eq(dt.len(), 0);
    expect_eq(dt.currentSize(), 0);

    // Varint prefix roundtrip check (5-bit & 7-bit)
    let vbuf: std.ArrayList(u8) = .empty;
    defer vbuf.deinit(alloc);
    encodeInt(&vbuf, alloc, 1337, 5, 0x20) catch unreachable;
    const dec_v = decodeInt(vbuf.items, 5) catch unreachable;
    expect_eq(dec_v.value, 1337);
    expect_eq(dec_v.consumed, vbuf.items.len);
}

test("cap:hpack - WithoutIndexing / NeverIndexed protection & TableSizeExceeded guard") {
    const alloc = std.testing.allocator;
    let encoder = HpackEncoder.init(alloc, 4096);
    defer encoder.deinit();
    let decoder = HpackDecoder.init(alloc, 4096);
    defer decoder.deinit();

    const headers = [
        HeaderField.init("authorization", "Bearer secret-token-123"),
        HeaderField.init("cookie", "session=xyz"),
        HeaderField.init("x-public", "ok"),
    ];
    const modes = [
        IndexingMode.NeverIndexed,
        IndexingMode.WithoutIndexing,
        IndexingMode.Incremental,
    ];

    const wire = encoder.encodeWithModes(headers[0..], modes[0..]) catch unreachable;
    defer alloc.free(wire);

    // Only "x-public" should enter the dynamic table
    expect_eq(encoder.dynamicTableLen(), 1);

    let decoded = decoder.decode(wire) catch unreachable;
    defer decoded.deinit();
    expect_eq(decoded.len(), 3);
    expect_eq(decoder.dynamicTableLen(), 1);
    expect_eq(std.mem.eql(u8, decoded.get(0).?.value, "Bearer secret-token-123"), true);
    expect_eq(std.mem.eql(u8, decoded.get(1).?.value, "session=xyz"), true);
    expect_eq(std.mem.eql(u8, decoded.get(2).?.value, "ok"), true);

    // TableSizeExceeded guard: encoder raises size to 8192 while decoder only allows 4096
    encoder.setMaxTableSize(8192);
    const probe = [HeaderField.init("x-test", "1")];
    const bad_wire = encoder.encode(probe[0..]) catch unreachable;
    defer alloc.free(bad_wire);

    let size_exceeded_caught = false;
    const bad_res = decoder.decode(bad_wire) catch |e| blk: {
        if (e == HpackError.TableSizeExceeded) {
            size_exceeded_caught = true;
        }
        break :blk null;
    };
    expect_eq(size_exceeded_caught, true);
    expect_eq(bad_res == null, true);
}

test("cap.hpack - dynamic mounting via import cap from \"cap\" with std.testing.allocator") {
    const alloc = std.testing.allocator;
    let encoder = cap.hpack.HpackEncoder.init(alloc, 2048);
    defer encoder.deinit();
    let decoder = cap.hpack.HpackDecoder.init(alloc, 2048);
    defer decoder.deinit();

    encoder.setHuffman(false);
    const hdrs = [
        cap.hpack.HeaderField.init(":method", "POST"),
        cap.hpack.HeaderField.init(":path", "/api/v1/infer"),
        cap.hpack.HeaderField.init("content-type", "application/json"),
    ];
    const wire = encoder.encode(hdrs[0..]) catch unreachable;
    defer alloc.free(wire);

    let out = decoder.decode(wire) catch unreachable;
    defer out.deinit();
    expect_eq(out.len(), 3);
    expect_eq(std.mem.eql(u8, out.get(0).?.value, "POST"), true);
    expect_eq(std.mem.eql(u8, out.get(1).?.value, "/api/v1/infer"), true);
    expect_eq(std.mem.eql(u8, out.get(2).?.value, "application/json"), true);
}
