// Source: native-cp-stdlib (RFC 8949 CBOR & Diagnostic Notation)
import std from "std";
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { CborValue, CborEntry, CborTag, decode, encode, encodeCanonical, toDiag } from "cap:cbor_diag";

test "RFC 8949 Appendix A official vectors decode and encode" {
    const alloc = std.testing.allocator;

    // Unsigned 0 -> 0x00
    let v0 = try decode(alloc, "\x00");
    defer v0.deinit(alloc);
    try expect_eq(v0.Unsigned, 0);
    const e0 = try encode(alloc, v0);
    defer alloc.free(e0);
    try expect_eq(std.mem.eql(u8, e0, "\x00"), true);

    // Unsigned 1000000 -> 0x1a 00 0f 42 40
    let v_big = try decode(alloc, "\x1a\x00\x0f\x42\x40");
    defer v_big.deinit(alloc);
    try expect_eq(v_big.Unsigned, 1000000);
    const e_big = try encode(alloc, v_big);
    defer alloc.free(e_big);
    try expect_eq(std.mem.eql(u8, e_big, "\x1a\x00\x0f\x42\x40"), true);

    // Negative -100 -> 0x38 0x63
    let v_neg = try decode(alloc, "\x38\x63");
    defer v_neg.deinit(alloc);
    try expect_eq(v_neg.Negative, -100);
    const e_neg = try encode(alloc, v_neg);
    defer alloc.free(e_neg);
    try expect_eq(std.mem.eql(u8, e_neg, "\x38\x63"), true);

    // Bytes h'01020304' -> 0x44 01 02 03 04
    let v_bytes = try decode(alloc, "\x44\x01\x02\x03\x04");
    defer v_bytes.deinit(alloc);
    try expect_eq(std.mem.eql(u8, v_bytes.Bytes, "\x01\x02\x03\x04"), true);

    // Text "IETF" -> 0x64 49 45 54 46
    let v_text = try decode(alloc, "\x64IETF");
    defer v_text.deinit(alloc);
    try expect_eq(std.mem.eql(u8, v_text.Text, "IETF"), true);

    // Array [1, [2, 3], [4, 5]] -> 0x83 01 82 02 03 82 04 05
    let v_arr = try decode(alloc, "\x83\x01\x82\x02\x03\x82\x04\x05");
    defer v_arr.deinit(alloc);
    try expect_eq(v_arr.Array.len, 3);
    try expect_eq(v_arr.Array[0].Unsigned, 1);
    try expect_eq(v_arr.Array[2].Array[1].Unsigned, 5);

    // Map {1: 2, 3: 4} -> 0xa2 01 02 03 04
    let v_map = try decode(alloc, "\xa2\x01\x02\x03\x04");
    defer v_map.deinit(alloc);
    try expect_eq(v_map.Map.len, 2);
    try expect_eq(v_map.Map[1].key.Unsigned, 3);
    try expect_eq(v_map.Map[1].value.Unsigned, 4);

    // Tag 1(1363896240) -> 0xc1 0x1a 0x51 0x4b 0x67 0xb0
    let v_tag = try decode(alloc, "\xc1\x1a\x51\x4b\x67\xb0");
    defer v_tag.deinit(alloc);
    try expect_eq(v_tag.Tag.tag, 1);
    try expect_eq(v_tag.Tag.value.Unsigned, 1363896240);

    // Simple(16), Float64(1.1)
    let v_simple = try decode(alloc, "\xf0");
    defer v_simple.deinit(alloc);
    try expect_eq(v_simple.Simple, 16);

    let v_f64 = try decode(alloc, "\xfb\x3f\xf1\x99\x99\x99\x99\x99\x9a");
    defer v_f64.deinit(alloc);
    try expect_eq(v_f64.Float, 1.1);
}

test "Indefinite-length decoding (0x5f/0x7f/0x9f/0xbf + 0xff break)" {
    const alloc = std.testing.allocator;

    // Indefinite bytes: (_ h'0102', h'030405')
    let vb = try decode(alloc, "\x5f\x42\x01\x02\x43\x03\x04\x05\xff");
    defer vb.deinit(alloc);
    try expect_eq(std.mem.eql(u8, vb.Bytes, "\x01\x02\x03\x04\x05"), true);

    // Indefinite text: (_ "strea", "ming")
    let vt = try decode(alloc, "\x7f\x65strea\x64ming\xff");
    defer vt.deinit(alloc);
    try expect_eq(std.mem.eql(u8, vt.Text, "streaming"), true);

    // Indefinite array + nested indefinite map: [_ 1, {_ "a": true}]
    let vn = try decode(alloc, "\x9f\x01\xbf\x61a\xf5\xff\xff");
    defer vn.deinit(alloc);
    try expect_eq(vn.Array.len, 2);
    try expect_eq(vn.Array[0].Unsigned, 1);
    try expect_eq(vn.Array[1].Map.len, 1);
    try expect_eq(vn.Array[1].Map[0].value.Bool, true);
}

test "Canonical encoding (RFC 8949 Section 4.2 length-first + lexicographical)" {
    const alloc = std.testing.allocator;

    let entries: [5]CborEntry = undefined;
    entries[0] = CborEntry{ .key = CborValue{ .Text = "bb" }, .value = CborValue{ .Unsigned = 1 } };
    entries[1] = CborEntry{ .key = CborValue{ .Text = "a" }, .value = CborValue{ .Unsigned = 2 } };
    entries[2] = CborEntry{ .key = CborValue{ .Negative = -1 }, .value = CborValue{ .Unsigned = 3 } };
    entries[3] = CborEntry{ .key = CborValue{ .Unsigned = 100 }, .value = CborValue{ .Unsigned = 4 } };
    entries[4] = CborEntry{ .key = CborValue{ .Unsigned = 10 }, .value = CborValue{ .Unsigned = 5 } };

    const map_val = CborValue{ .Map = &entries };
    const encoded = try encodeCanonical(alloc, map_val);
    defer alloc.free(encoded);

    try expect_eq(std.mem.eql(u8, encoded, "\xa5\x0a\x05\x20\x03\x18\x64\x04\x61a\x02\x62bb\x01"), true);

    // Canonical float shortest form: 1.5 -> 0xf9 0x3e 0x00
    const f_encoded = try encodeCanonical(alloc, CborValue{ .Float = 1.5 });
    defer alloc.free(f_encoded);
    try expect_eq(std.mem.eql(u8, f_encoded, "\xf9\x3e\x00"), true);
}

test "Diagnostic notation (toDiag) and malformed/depth-overflow rejection" {
    const alloc = std.testing.allocator;

    // {1: 2, "a": [3, true, h'0102', 1(1363896240)]}
    let val = try decode(alloc, "\xa2\x01\x02\x61a\x84\x03\xf5\x42\x01\x02\xc1\x1a\x51\x4b\x67\xb0");
    defer val.deinit(alloc);

    const diag = try toDiag(alloc, val);
    defer alloc.free(diag);
    try expect_eq(std.mem.eql(u8, diag, "{1: 2, \"a\": [3, true, h'0102', 1(1363896240)]}"), true);

    // Truncated input -> error.UnexpectedEof
    let res_eof = decode(alloc, "\x44\x01\x02");
    try expect_eq(res_eof == error.UnexpectedEof, true);

    // Nesting depth > 64 -> error.NestingTooDeep
    let deep_buf: [66]u8 = undefined;
    let i: usize = 0;
    while (i < 65) {
        deep_buf[i] = 0x81;
        i += 1;
    }
    deep_buf[65] = 0x00;
    let res_deep = decode(alloc, &deep_buf);
    try expect_eq(res_deep == error.NestingTooDeep, true);
}

test "Dynamic mount cap.cbor_diag and zero memory leak" {
    const alloc = std.testing.allocator;

    let decoded = try cap.cbor_diag.decode(alloc, "\x83\xf6\xf7\xf4");
    defer decoded.deinit(alloc);

    const diag = try cap.cbor_diag.toDiag(alloc, decoded);
    defer alloc.free(diag);
    try expect_eq(std.mem.eql(u8, diag, "[null, undefined, false]"), true);
}
