// Source: cp/cap/test/test_protobuf_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:protobuf & cap.protobuf

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ProtoError,
    WireType,
    zigzagEncode32,
    zigzagDecode32,
    zigzagEncode64,
    zigzagDecode64,
    ProtoField,
    ProtoWriter,
    ProtoReader,
    GrpcFrameCodec,
    GrpcStatus,
    GrpcTimeout,
} from "cap:compiler";

fn testVarintZigZagFixedFloatAndStrings(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    const alloc = gpa.allocator();

    // 1. ZigZag 32-bit & 64-bit negative and extreme values
    expect_eq(zigzagEncode32(0), 0 as u32);
    expect_eq(zigzagEncode32(-1), 1 as u32);
    expect_eq(zigzagEncode32(1), 2 as u32);
    expect_eq(zigzagEncode32(-2), 3 as u32);
    expect_eq(zigzagEncode32(std.math.maxInt(i32)), 0xFFFFFFFE as u32);
    expect_eq(zigzagEncode32(std.math.minInt(i32)), 0xFFFFFFFF as u32);

    expect_eq(zigzagDecode32(0), 0 as i32);
    expect_eq(zigzagDecode32(1), -1 as i32);
    expect_eq(zigzagDecode32(2), 1 as i32);
    expect_eq(zigzagDecode32(3), -2 as i32);
    expect_eq(zigzagDecode32(0xFFFFFFFE), std.math.maxInt(i32));
    expect_eq(zigzagDecode32(0xFFFFFFFF), std.math.minInt(i32));

    expect_eq(zigzagEncode64(0), 0 as u64);
    expect_eq(zigzagEncode64(-1), 1 as u64);
    expect_eq(zigzagEncode64(1), 2 as u64);
    expect_eq(zigzagEncode64(std.math.maxInt(i64)), std.math.maxInt(u64) - 1);
    expect_eq(zigzagEncode64(std.math.minInt(i64)), std.math.maxInt(u64));

    expect_eq(zigzagDecode64(0), 0 as i64);
    expect_eq(zigzagDecode64(1), -1 as i64);
    expect_eq(zigzagDecode64(std.math.maxInt(u64) - 1), std.math.maxInt(i64));
    expect_eq(zigzagDecode64(std.math.maxInt(u64)), std.math.minInt(i64));

    // 2. Official proto3 wire vector: field 1 varint = 150 -> [0x08, 0x96, 0x01]
    let w_vec = ProtoWriter.init(alloc);
    defer w_vec.deinit();
    w_vec.writeTag(1, WireType.Varint)?;
    w_vec.writeVarint(150)?;
    const v1: [3]u8 = [0x08, 0x96, 0x01];
    expect_eq(std.mem.eql(u8, w_vec.slice(), &v1), true);

    // 3. Full scalar roundtrip: int32, int64, sint32, sint64, bool, fixed32, fixed64, float, double, string, bytes
    let w = ProtoWriter.init(alloc);
    defer w.deinit();
    w.writeInt32(1, -123)?;
    w.writeInt64(2, -9876543210123)?;
    w.writeSint32(3, std.math.minInt(i32))?;
    w.writeSint64(4, std.math.minInt(i64))?;
    w.writeBool(5, true)?;
    w.writeFixed32(6, 0xDEADBEEF)?;
    const f64_val: u64 = ((0xCAFEBABE as u64) << 32) | (0x01020304 as u64);
    w.writeFixed64(7, f64_val)?;
    w.writeFloat(8, 3.5)?;
    w.writeDouble(9, -125.75)?;
    w.writeString(10, "cp-protobuf-codec")?;
    const raw_payload: [4]u8 = [0x00, 0xFF, 0xAA, 0x55];
    w.writeBytes(11, &raw_payload)?;

    let r = ProtoReader.init(w.slice());
    let seen_count: usize = 0;
    while (field := r.next()?) {
        seen_count += 1;
        switch (field.field_number) {
            1 => {
                expect_eq(field.wire_type == WireType.Varint, true);
                expect_eq(field.asInt32(), -123 as i32);
            },
            2 => expect_eq(field.asInt64(), -9876543210123 as i64),
            3 => expect_eq(field.asSint32(), std.math.minInt(i32)),
            4 => expect_eq(field.asSint64(), std.math.minInt(i64)),
            5 => expect_eq(field.asBool(), true),
            6 => {
                expect_eq(field.wire_type == WireType.Fixed32, true);
                expect_eq(field.asFixed32(), 0xDEADBEEF as u32);
            },
            7 => {
                expect_eq(field.wire_type == WireType.Fixed64, true);
                expect_eq(field.asFixed64(), f64_val);
            },
            8 => expect_eq(field.asFloat(), 3.5 as f32),
            9 => expect_eq(field.asDouble(), -125.75 as f64),
            10 => {
                expect_eq(field.wire_type == WireType.LengthDelimited, true);
                expect_eq(std.mem.eql(u8, field.asString(), "cp-protobuf-codec"), true);
            },
            11 => expect_eq(std.mem.eql(u8, field.asBytes(), &raw_payload), true),
            else => return error.UnexpectedField,
        }
    }
    expect_eq(seen_count, 11 as usize);
    expect_eq(r.hasMore(), false);
}

fn testNestedSubmessagePackedAndSkipField(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    const alloc = gpa.allocator();

    // Build inner submessage
    let sub_w = ProtoWriter.init(alloc);
    defer sub_w.deinit();
    sub_w.writeSint32(1, -4096)?;
    sub_w.writeString(2, "inner-node")?;

    // Build outer message with interleaved unknown fields (Varint, Fixed64, LengthDelimited, Fixed32)
    let w = ProtoWriter.init(alloc);
    defer w.deinit();
    w.writeUint64(1, 42)?;
    w.writeUint64(900, 999999)?;                    // unknown Varint
    w.writeFixed64(901, 0x1122334455667788)?;       // unknown Fixed64
    w.writeSubmessage(2, sub_w.slice())?;
    w.writeBytes(902, "unknown-len-payload")?;      // unknown LengthDelimited
    const packed_ids: [4]u64 = [3, 270, 86942, 2097152];
    w.writePackedVarints(3, &packed_ids)?;
    w.writeFixed32(903, 0x12345678)?;               // unknown Fixed32
    w.writeString(4, "after-unknowns")?;

    let r = ProtoReader.init(w.slice());
    let got_id: u64 = 0;
    let got_sub_sint: i32 = 0;
    let got_sub_str_ok: bool = false;
    let got_packed_ok: bool = false;
    let got_tail_ok: bool = false;
    let skipped_unknowns: usize = 0;

    while (field := r.next()?) {
        switch (field.field_number) {
            1 => got_id = field.asUint64(),
            2 => {
                let sub_r = r.readSubmessage(field)?;
                while (sf := sub_r.next()?) {
                    switch (sf.field_number) {
                        1 => got_sub_sint = sf.asSint32(),
                        2 => got_sub_str_ok = std.mem.eql(u8, sf.asString(), "inner-node"),
                        else => sub_r.skipField(sf)?,
                    }
                }
            },
            3 => {
                const decoded = ProtoReader.decodePackedVarints(alloc, field.asBytes())?;
                defer alloc.free(decoded);
                expect_eq(decoded.len, 4 as usize);
                got_packed_ok = (decoded[0] == 3)
                    && (decoded[1] == 270)
                    && (decoded[2] == 86942)
                    && (decoded[3] == 2097152);
            },
            4 => got_tail_ok = std.mem.eql(u8, field.asString(), "after-unknowns"),
            else => {
                r.skipField(field)?;
                skipped_unknowns += 1;
            },
        }
    }

    expect_eq(got_id, 42 as u64);
    expect_eq(got_sub_sint, -4096 as i32);
    expect_eq(got_sub_str_ok, true);
    expect_eq(got_packed_ok, true);
    expect_eq(got_tail_ok, true);
    expect_eq(skipped_unknowns, 4 as usize);
}

fn expectNextErr(r: *ProtoReader, expected: anyerror): bool {
    * = r.next() catch e => return e == expected;
    return false;
}

fn expectSubmessageErr(r: *ProtoReader, field: ProtoField, expected: anyerror): bool {
    * = r.readSubmessage(field) catch e => return e == expected;
    return false;
}

fn testAbnormalBoundaries(): !void {
    // 1. >10-byte malformed Varint (11 continuation bytes) -> ProtoError.VarintOverflow
    const bad_varint_11b: [12]u8 = [
        0x08, // field 1, Varint
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01,
    ];
    let r1 = ProtoReader.init(&bad_varint_11b);
    expect_eq(bad_varint_11b.len > 10, true);
    expect_eq(expectNextErr(&r1, ProtoError.VarintOverflow), true);

    // 2. 10th byte bit-63 overflow (0x02 in 10th byte) -> ProtoError.VarintOverflow
    const bad_varint_10b: [11]u8 = [
        0x08,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
    ];
    let r2 = ProtoReader.init(&bad_varint_10b);
    expect_eq(expectNextErr(&r2, ProtoError.VarintOverflow), true);

    // 3. Field number 0 (tag 0x00) -> ProtoError.InvalidFieldNumber
    const zero_field: [2]u8 = [0x00, 0x01];
    let r3 = ProtoReader.init(&zero_field);
    expect_eq(expectNextErr(&r3, ProtoError.InvalidFieldNumber), true);

    // 4. Invalid WireType 6 (tag = 1 << 3 | 6 = 0x0E) and 7 (0x0F) -> ProtoError.InvalidWireType
    const bad_wt6: [1]u8 = [0x0E];
    let r4 = ProtoReader.init(&bad_wt6);
    expect_eq(expectNextErr(&r4, ProtoError.InvalidWireType), true);

    const bad_wt7: [1]u8 = [0x0F];
    let r5 = ProtoReader.init(&bad_wt7);
    expect_eq(expectNextErr(&r5, ProtoError.InvalidWireType), true);

    // 5. Truncated LengthDelimited payload (length says 10, only 3 bytes remain) -> ProtoError.TruncatedLengthDelimited
    const trunc_len: [5]u8 = [0x12, 0x0A, 0x61, 0x62, 0x63];
    let r6 = ProtoReader.init(&trunc_len);
    expect_eq(expectNextErr(&r6, ProtoError.TruncatedLengthDelimited), true);
}

fn testMaxDepthExceededProtection(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    const alloc = gpa.allocator();

    // Build 4-level nested submessages: level4 -> level3 -> level2 -> level1
    let w4 = ProtoWriter.init(alloc);
    defer w4.deinit();
    w4.writeUint64(1, 999)?;

    let w3 = ProtoWriter.init(alloc);
    defer w3.deinit();
    w3.writeSubmessage(1, w4.slice())?;

    let w2 = ProtoWriter.init(alloc);
    defer w2.deinit();
    w2.writeSubmessage(1, w3.slice())?;

    let w1 = ProtoWriter.init(alloc);
    defer w1.deinit();
    w1.writeSubmessage(1, w2.slice())?;

    // With max_depth = 2, descending 3 levels must fail with ProtoError.MaxDepthExceeded
    let r0 = ProtoReader.initWithMaxDepth(w1.slice(), 2);
    const f1 = (r0.next()?).?;
    let r1 = r0.readSubmessage(f1)?; // depth 1
    const f2 = (r1.next()?).?;
    let r2 = r1.readSubmessage(f2)?; // depth 2
    const f3 = (r2.next()?).?;

    expect_eq(expectSubmessageErr(&r2, f3, ProtoError.MaxDepthExceeded), true);

    // With max_depth = 4, descending all levels succeeds
    let ok0 = ProtoReader.initWithMaxDepth(w1.slice(), 4);
    const ok_f1 = (ok0.next()?).?;
    let ok1 = ok0.readSubmessage(ok_f1)?;
    const ok_f2 = (ok1.next()?).?;
    let ok2 = ok1.readSubmessage(ok_f2)?;
    const ok_f3 = (ok2.next()?).?;
    let ok3 = ok2.readSubmessage(ok_f3)?;
    const leaf = (ok3.next()?).?;
    expect_eq(leaf.asUint64(), 999 as u64);
}

fn testCapNamespaceDynamicMountingAndZeroLeak(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    const alloc = gpa.allocator();

    // Verify `import cap from "cap"; cap.compiler.*` dynamic lazy mounting + zero-leak
    let pw = cap.compiler.ProtoWriter.init(alloc);
    defer pw.deinit();
    pw.writeString(1, "cap-protobuf-mounted")?;
    pw.writeSint32(2, -777)?;

    const frame_bytes = cap.compiler.GrpcFrameCodec.encode(alloc, false, pw.slice())?;
    defer alloc.free(frame_bytes);

    const decoded_frame = cap.compiler.GrpcFrameCodec.decode(frame_bytes)?;
    let pr = cap.compiler.ProtoReader.init(decoded_frame.payload);

    const f1 = (pr.next()?).?;
    expect_eq(f1.field_number, 1 as u32);
    expect_eq(std.mem.eql(u8, f1.asString(), "cap-protobuf-mounted"), true);

    const f2 = (pr.next()?).?;
    expect_eq(f2.field_number, 2 as u32);
    expect_eq(f2.asSint32(), -777 as i32);
    expect_eq(pr.next()? == null, true);

    expect_eq(cap.compiler.GrpcStatus.Unavailable.isRetryable(), true);
    expect_eq(cap.compiler.GrpcTimeout.parse("5S")?, 5000000000 as u64);
    expect_eq(cap.compiler.zigzagEncode32(-1), 1 as u32);
    expect_eq(cap.compiler.zigzagDecode32(1), -1 as i32);
}

pub fn main()! {
    test("1. Varint/ZigZag/Fixed/Float/Double/String/Bytes Roundtrip", () => {
        testVarintZigZagFixedFloatAndStrings() catch unreachable;
    });
    test("2. Nested Submessage, Packed Repeated & Unknown Field skipField", () => {
        testNestedSubmessagePackedAndSkipField() catch unreachable;
    });
    test("3. Abnormal Boundaries (VarintOverflow, InvalidFieldNumber, InvalidWireType, TruncatedLengthDelimited)", () => {
        testAbnormalBoundaries() catch unreachable;
    });
    test("4. Recursive Submessage MaxDepthExceeded Protection", () => {
        testMaxDepthExceededProtection() catch unreachable;
    });
    test("5. cap.protobuf Dynamic Mounting & std.testing.allocator Zero-Leak", () => {
        testCapNamespaceDynamicMountingAndZeroLeak() catch unreachable;
    });
}
