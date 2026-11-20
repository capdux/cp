// Source: cp/cap/test/test_parquet_production_deep.cp (Production deep tests for cap:parquet)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ParquetError,
    ColumnData,
    ThriftWriter,
    ThriftReader,
    ThriftType,
    MAX_THRIFT_DEPTH,
    encodeRleBitPacked,
    decodeRleBitPacked,
    bitWidthForMaxVal,
    writeParquet,
    readParquet,
} from "cap:parquet";

test("Thrift TCompactProtocol Struct/List/ZigZag Varint roundtrip and max_depth guard") {
    const alloc = std.testing.allocator;
    let w = ThriftWriter.init(alloc);
    defer w.deinit();

    try w.writeI32Field(1, -123456);
    try w.writeI64Field(2, -9876543210123);
    try w.writeBoolField(3, true);
    try w.writeBoolField(4, false);
    try w.writeDoubleField(5, 3.1415926535);
    try w.writeBinaryField(6, "parquet-thrift-payload");

    try w.writeFieldHeader(10, .List);
    try w.writeListHeader(.I32, 4);
    try w.writeZigZagI32(-1);
    try w.writeZigZagI32(0);
    try w.writeZigZagI32(42);
    try w.writeZigZagI32(2147483647);

    try w.writeFieldHeader(20, .Struct);
    try w.beginStruct();
    try w.writeI32Field(1, 777);
    try w.writeBinaryField(2, "nested");
    try w.endStruct();

    try w.writeFieldStop();

    let r = ThriftReader.init(w.bytes());
    const f1 = try r.readFieldHeader();
    try expect_eq(f1.field_id, @as(i16, 1));
    try expect_eq(try r.readZigZagI32(), @as(i32, -123456));

    const f2 = try r.readFieldHeader();
    try expect_eq(f2.field_id, @as(i16, 2));
    try expect_eq(try r.readZigZagI64(), @as(i64, -9876543210123));

    const f3 = try r.readFieldHeader();
    try expect_eq(f3.field_type, ThriftType.BoolTrue);

    const f4 = try r.readFieldHeader();
    try expect_eq(f4.field_type, ThriftType.BoolFalse);

    const f5 = try r.readFieldHeader();
    try expect_eq(f5.field_id, @as(i16, 5));
    const dbl = try r.readDouble();
    try std.testing.expect(@abs(dbl - 3.1415926535) < 1e-9);

    const f6 = try r.readFieldHeader();
    try expect_eq(f6.field_id, @as(i16, 6));
    try std.testing.expectEqualStrings("parquet-thrift-payload", try r.readBinary());

    const f10 = try r.readFieldHeader();
    try expect_eq(f10.field_id, @as(i16, 10));
    const lh = try r.readListHeader();
    try expect_eq(lh.count, @as(usize, 4));
    try expect_eq(try r.readZigZagI32(), @as(i32, -1));
    try expect_eq(try r.readZigZagI32(), @as(i32, 0));
    try expect_eq(try r.readZigZagI32(), @as(i32, 42));
    try expect_eq(try r.readZigZagI32(), @as(i32, 2147483647));

    const f20 = try r.readFieldHeader();
    try expect_eq(f20.field_id, @as(i16, 20));
    try r.beginStruct();
    const nf1 = try r.readFieldHeader();
    try expect_eq(nf1.field_id, @as(i16, 1));
    try expect_eq(try r.readZigZagI32(), @as(i32, 777));
    const nf2 = try r.readFieldHeader();
    try expect_eq(nf2.field_id, @as(i16, 2));
    try std.testing.expectEqualStrings("nested", try r.readBinary());
    const nstop = try r.readFieldHeader();
    try expect_eq(nstop.field_type, ThriftType.Stop);
    r.endStruct();

    const stop = try r.readFieldHeader();
    try expect_eq(stop.field_type, ThriftType.Stop);

    // Depth limit guard
    let dw = ThriftWriter.init(alloc);
    defer dw.deinit();
    let d: usize = 0;
    while (d < MAX_THRIFT_DEPTH - 1) : (d += 1) {
        try dw.beginStruct();
    }
    try std.testing.expectError(ParquetError.DepthLimitExceeded, dw.beginStruct());
}

test("RLE / Bit-Packing Hybrid roundtrip (RLE run, bit-packed, mixed)") {
    const alloc = std.testing.allocator;

    // 1. Pure RLE run
    let rle_only: [25]u32 = std.mem.zeroes([25]u32);
    @memset(&rle_only, 5);
    const bw1 = bitWidthForMaxVal(5);
    const enc1 = try encodeRleBitPacked(alloc, &rle_only, bw1);
    defer alloc.free(enc1);
    const dec1 = try decodeRleBitPacked(alloc, enc1, bw1, rle_only.len);
    defer alloc.free(dec1);
    try std.testing.expectEqualSlices(u32, &rle_only, dec1);

    // 2. Pure Bit-Packed (non-multiple of 8)
    const bp_only: [11]u32 = [ 0, 1, 2, 3, 4, 5, 6, 7, 2, 5, 1 ];
    const bw2 = bitWidthForMaxVal(7);
    const enc2 = try encodeRleBitPacked(alloc, &bp_only, bw2);
    defer alloc.free(enc2);
    const dec2 = try decodeRleBitPacked(alloc, enc2, bw2, bp_only.len);
    defer alloc.free(dec2);
    try std.testing.expectEqualSlices(u32, &bp_only, dec2);

    // 3. Mixed RLE + Bit-Packed + RLE
    const mixed: [32]u32 = [
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        3, 1, 4,
    ];
    const bw3 = bitWidthForMaxVal(10);
    const enc3 = try encodeRleBitPacked(alloc, &mixed, bw3);
    defer alloc.free(enc3);
    const dec3 = try decodeRleBitPacked(alloc, enc3, bw3, mixed.len);
    defer alloc.free(dec3);
    try std.testing.expectEqualSlices(u32, &mixed, dec3);

    // 4. Zero bit-width
    const zeros: [4]u32 = [ 0, 0, 0, 0 ];
    const enc4 = try encodeRleBitPacked(alloc, &zeros, 0);
    defer alloc.free(enc4);
    const dec4 = try decodeRleBitPacked(alloc, enc4, 0, zeros.len);
    defer alloc.free(dec4);
    try std.testing.expectEqualSlices(u32, &zeros, dec4);
}

test("Parquet writeParquet and readParquet multi-type roundtrip") {
    const alloc = std.testing.allocator;

    const c_i32: [4]i32 = [ -100, 0, 42, 999999 ];
    const c_i64: [4]i64 = [ -9000000000, 1, 2, 9000000000 ];
    const c_f64: [4]f64 = [ -1.5, 0.0, 2.718281828, 1000000000000.0 ];
    const c_str: [4][]const u8 = [ "alpha", "", "parquet-cp", "zstd-ready" ];
    const c_bool: [4]bool = [ true, false, true, false ];

    const cols: [5]ColumnData = [
        ColumnData.fromInt32("id", &c_i32),
        ColumnData.fromInt64("ts", &c_i64),
        ColumnData.fromDouble("score", &c_f64),
        ColumnData.fromByteArray("tag", &c_str),
        ColumnData.fromBool("active", &c_bool),
    ];

    const bytes = try writeParquet(alloc, &cols);
    defer alloc.free(bytes);

    try std.testing.expectEqualStrings("PAR1", bytes[0..4]);
    try std.testing.expectEqualStrings("PAR1", bytes[bytes.len - 4 .. bytes.len]);

    let file = try readParquet(alloc, bytes);
    defer file.deinit(alloc);

    try expect_eq(file.num_rows, @as(i64, 4));
    try expect_eq(file.num_row_groups, @as(usize, 1));
    try expect_eq(file.columns.len, @as(usize, 5));
    try std.testing.expectEqualStrings("cp-parquet 1.0", file.created_by);

    try std.testing.expectEqualStrings("id", file.columns[0].name);
    try std.testing.expectEqualSlices(i32, &c_i32, file.columns[0].int32_vals);

    try std.testing.expectEqualStrings("ts", file.columns[1].name);
    try std.testing.expectEqualSlices(i64, &c_i64, file.columns[1].int64_vals);

    try std.testing.expectEqualStrings("score", file.columns[2].name);
    try std.testing.expectEqualSlices(f64, &c_f64, file.columns[2].double_vals);

    try std.testing.expectEqualStrings("tag", file.columns[3].name);
    try expect_eq(file.columns[3].byte_array_vals.len, @as(usize, 4));
    try std.testing.expectEqualStrings("alpha", file.columns[3].byte_array_vals[0]);
    try std.testing.expectEqualStrings("", file.columns[3].byte_array_vals[1]);
    try std.testing.expectEqualStrings("parquet-cp", file.columns[3].byte_array_vals[2]);
    try std.testing.expectEqualStrings("zstd-ready", file.columns[3].byte_array_vals[3]);

    try std.testing.expectEqualStrings("active", file.columns[4].name);
    try std.testing.expectEqualSlices(bool, &c_bool, file.columns[4].bool_vals);
}

test("Parquet malformed input and boundary guards") {
    const alloc = std.testing.allocator;

    // Too short
    try std.testing.expectError(ParquetError.TruncatedData, readParquet(alloc, "PAR1PAR1"));

    // Invalid header magic
    const bad_head = "BAD1\x00\x00\x00\x00\x01\x00\x00\x00PAR1";
    try std.testing.expectError(ParquetError.InvalidMagic, readParquet(alloc, bad_head));

    // Invalid tail magic
    const bad_tail = "PAR1\x00\x00\x00\x00\x01\x00\x00\x00BAD1";
    try std.testing.expectError(ParquetError.InvalidMagic, readParquet(alloc, bad_tail));

    // Out-of-bounds footer length
    const bad_flen = "PAR1\x00\x00\x00\x00\xFF\xFF\x00\x00PAR1";
    try std.testing.expectError(ParquetError.InvalidFooterLength, readParquet(alloc, bad_flen));

    // Zero footer length
    const zero_flen = "PAR1\x00\x00\x00\x00\x00\x00\x00\x00PAR1";
    try std.testing.expectError(ParquetError.InvalidFooterLength, readParquet(alloc, zero_flen));

    // Invalid RLE stream & bit width
    try std.testing.expectError(ParquetError.InvalidBitWidth, decodeRleBitPacked(alloc, "", 33, 1));
    try std.testing.expectError(ParquetError.InvalidRleStream, decodeRleBitPacked(alloc, "\x00\x00", 1, 1));
    try std.testing.expectError(ParquetError.TruncatedData, decodeRleBitPacked(alloc, "\x03", 4, 8));
}

test("cap.parquet dynamic mount and zero memory leak") {
    const alloc = std.testing.allocator;

    const nums: [3]i32 = [ 10, 20, 30 ];
    const cols: [1]ColumnData = [
        cap.parquet.ColumnData.fromInt32("v", &nums),
    ];
    const raw = try cap.parquet.writeParquet(alloc, &cols);
    defer alloc.free(raw);

    let pf = try cap.parquet.readParquet(alloc, raw);
    defer pf.deinit(alloc);

    try expect_eq(pf.num_rows, @as(i64, 3));
    try std.testing.expectEqualSlices(i32, &nums, pf.columns[0].int32_vals);

    const rle_in: [10]u32 = [ 1, 2, 3, 3, 3, 3, 3, 3, 3, 3 ];
    const rle_bytes = try cap.parquet.encodeRleBitPacked(alloc, &rle_in, 2);
    defer alloc.free(rle_bytes);
    const rle_out = try cap.parquet.decodeRleBitPacked(alloc, rle_bytes, 2, rle_in.len);
    defer alloc.free(rle_out);
    try std.testing.expectEqualSlices(u32, &rle_in, rle_out);
}
