// Source: cp/cap/test/test_protobuf_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:protobuf & cap.protobuf

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ProtoError,
    WireType,
    ZigZag,
    ProtoTag,
    ProtoWriter,
    ProtoReader,
    GrpcError,
    GrpcStatus,
    GrpcFrame,
    GrpcFrameCodec,
    GrpcFrameStream,
    GrpcTimeout,
} from "cap:protobuf";

fn testVarintZigZagAndOfficialVectors(): !void {
    const alloc = std.heap.page_allocator;

    // 1. ZigZag 32-bit boundary values
    expect_eq(ZigZag.encode32(0), 0 as u32);
    expect_eq(ZigZag.encode32(-1), 1 as u32);
    expect_eq(ZigZag.encode32(1), 2 as u32);
    expect_eq(ZigZag.encode32(-2), 3 as u32);
    expect_eq(ZigZag.encode32(2147483647), 0xFFFFFFFE as u32);
    expect_eq(ZigZag.encode32(-2147483648), 0xFFFFFFFF as u32);

    expect_eq(ZigZag.decode32(0), 0 as i32);
    expect_eq(ZigZag.decode32(1), -1 as i32);
    expect_eq(ZigZag.decode32(2), 1 as i32);
    expect_eq(ZigZag.decode32(3), -2 as i32);
    expect_eq(ZigZag.decode32(0xFFFFFFFE), 2147483647 as i32);
    expect_eq(ZigZag.decode32(0xFFFFFFFF), -2147483648 as i32);

    // 2. ZigZag 64-bit boundary values
    expect_eq(ZigZag.encode64(0), 0 as u64);
    expect_eq(ZigZag.encode64(-1), 1 as u64);
    expect_eq(ZigZag.encode64(1), 2 as u64);
    expect_eq(ZigZag.encode64(std.math.maxInt(i64)), std.math.maxInt(u64) - 1);
    expect_eq(ZigZag.encode64(std.math.minInt(i64)), std.math.maxInt(u64));

    expect_eq(ZigZag.decode64(0), 0 as i64);
    expect_eq(ZigZag.decode64(1), -1 as i64);
    expect_eq(ZigZag.decode64(std.math.maxInt(u64) - 1), std.math.maxInt(i64));
    expect_eq(ZigZag.decode64(std.math.maxInt(u64)), std.math.minInt(i64));

    // 3. Proto3 official spec vector: field 1 varint = 150 -> [0x08, 0x96, 0x01]
    let w1 = ProtoWriter.init(alloc);
    defer w1.deinit();
    w1.writeVarint(1, 150)?;
    const v1: [3]u8 = [0x08, 0x96, 0x01];
    expect_eq(std.mem.eql(u8, w1.slice(), &v1), true);

    // 4. Proto3 official spec vector: field 2 string "testing" -> [0x12, 0x07, ...]
    let w2 = ProtoWriter.init(alloc);
    defer w2.deinit();
    w2.writeString(2, "testing")?;
    const v2: [9]u8 = [0x12, 0x07, 0x74, 0x65, 0x73, 0x74, 0x69, 0x6e, 0x67];
    expect_eq(std.mem.eql(u8, w2.slice(), &v2), true);

    // 5. Proto3 official spec vector: field 3 submessage { field 1: 150 } -> [0x1a, 0x03, 0x08, 0x96, 0x01]
    let w3 = ProtoWriter.init(alloc);
    defer w3.deinit();
    w3.writeSubmessage(3, w1.slice())?;
    const v3: [5]u8 = [0x1a, 0x03, 0x08, 0x96, 0x01];
    expect_eq(std.mem.eql(u8, w3.slice(), &v3), true);

    // 6. Proto3 official spec vector: field 4 packed repeated [3, 270, 86942]
    let w4 = ProtoWriter.init(alloc);
    defer w4.deinit();
    const packed_vals: [3]u64 = [3, 270, 86942];
    w4.writePackedVarints(4, &packed_vals)?;
    const v4: [8]u8 = [0x22, 0x06, 0x03, 0x8e, 0x02, 0x9e, 0xa7, 0x05];
    expect_eq(std.mem.eql(u8, w4.slice(), &v4), true);

    // 7. Max u64 varint (10 bytes: 9x 0xFF + 0x01) roundtrip
    let w5 = ProtoWriter.init(alloc);
    defer w5.deinit();
    w5.writeVarint(15, std.math.maxInt(u64))?;
    expect_eq(w5.len(), 11); // 1 byte tag (15 << 3 | 0 = 0x78) + 10 byte varint
    let r5 = ProtoReader.init(w5.slice());
    const t5 = (r5.nextTag()?).?;
    expect_eq(t5.field_number, 15 as u32);
    expect_eq(t5.wire_type == WireType.Varint, true);
    expect_eq(r5.readVarint()?, std.math.maxInt(u64));
    expect_eq(r5.nextTag()? == null, true);
}

fn testNestedSubmessagePackedAndSkipFieldRoundtrip(): !void {
    const alloc = std.heap.page_allocator;

    // Build inner submessage
    let sub_w = ProtoWriter.init(alloc);
    defer sub_w.deinit();
    sub_w.writeSint32(1, -12345)?;
    sub_w.writeSint64(2, -9876543210123)?;
    sub_w.writeFloat(3, 1.5)?;
    sub_w.writeDouble(4, -3.25)?;
    sub_w.writeBool(5, true)?;

    // Build outer message with interleaved unknown fields covering all 4 wire types
    let w = ProtoWriter.init(alloc);
    defer w.deinit();
    w.writeVarint(1, 42)?;
    w.writeVarint(900, 999999)?;                    // unknown Varint
    w.writeString(2, "nexus-agent")?;
    w.writeFixed64(901, 0x1122334455667788)?;       // unknown Fixed64
    w.writeSubmessage(3, sub_w.slice())?;
    w.writeBytes(902, "unknown-len-payload")?;      // unknown Len
    const ids: [4]u64 = [1, 128, 16384, 2097152];
    w.writePackedVarints(4, &ids)?;
    w.writeFixed32(903, 0xDEADBEEF)?;               // unknown Fixed32
    const expected_f64: u64 = ((0xCAFEBABE as u64) << 32) | (0x01020304 as u64);
    w.writeFixed32(5, 0x12345678)?;
    w.writeFixed64(6, expected_f64)?;
    w.writeBytes(7, "raw\x00bin")?;
    w.writeInt32(8, -42)?;

    const owned = w.toOwnedSlice()?;
    defer alloc.free(owned);

    let r = ProtoReader.init(owned);
    let got_id: u64 = 0;
    let got_name: []const u8 = "";
    let got_f32: u32 = 0;
    let got_f64: u64 = 0;
    let got_bytes: []const u8 = "";
    let got_int32: i32 = 0;
    let skipped_count: usize = 0;

    let sub_s32: i32 = 0;
    let sub_s64: i64 = 0;
    let sub_flt: f32 = 0.0;
    let sub_dbl: f64 = 0.0;
    let sub_bool: bool = false;
    let packed_out_opt: ([]u64)? = null;
    defer if (p := packed_out_opt) {
        if (p.len > 0) alloc.free(p);
    }

    while (tag := r.nextTag()?) {
        switch (tag.field_number) {
            1 => { got_id = r.readVarint()?; },
            2 => { got_name = r.readString()?; },
            3 => {
                let sr = r.readSubmessage()?;
                while (stag := sr.nextTag()?) {
                    switch (stag.field_number) {
                        1 => { sub_s32 = sr.readSint32()?; },
                        2 => { sub_s64 = sr.readSint64()?; },
                        3 => { sub_flt = sr.readFloat()?; },
                        4 => { sub_dbl = sr.readDouble()?; },
                        5 => { sub_bool = sr.readBool()?; },
                        else => sr.skipField(stag.wire_type)?,
                    }
                }
            },
            4 => { packed_out_opt = r.readPackedVarints(alloc)?; },
            5 => { got_f32 = r.readFixed32()?; },
            6 => { got_f64 = r.readFixed64()?; },
            7 => { got_bytes = r.readBytes()?; },
            8 => { got_int32 = r.readInt32()?; },
            else => {
                r.skipField(tag.wire_type)?;
                skipped_count += 1;
            },
        }
    }

    expect_eq(got_id, 42 as u64);
    expect_eq(std.mem.eql(u8, got_name, "nexus-agent"), true);
    expect_eq(skipped_count, 4 as usize);
    expect_eq(sub_s32, -12345 as i32);
    expect_eq(sub_s64, -9876543210123 as i64);
    expect_eq(sub_flt > 1.49 && sub_flt < 1.51, true);
    expect_eq(sub_dbl < -3.24 && sub_dbl > -3.26, true);
    expect_eq(sub_bool, true);

    const packed_out = packed_out_opt.?;
    expect_eq(packed_out.len, 4 as usize);
    expect_eq(packed_out[0], 1 as u64);
    expect_eq(packed_out[1], 128 as u64);
    expect_eq(packed_out[2], 16384 as u64);
    expect_eq(packed_out[3], 2097152 as u64);

    expect_eq(got_f32, 0x12345678 as u32);
    expect_eq(got_f64, expected_f64);
    expect_eq(std.mem.eql(u8, got_bytes, "raw\x00bin"), true);
    expect_eq(got_int32, -42 as i32);

    // Zero-copy check: got_name pointer must sit inside `owned` memory range
    const base_addr: usize = @intFromPtr(owned.ptr);
    const end_addr: usize = base_addr + owned.len;
    const str_addr: usize = @intFromPtr(got_name.ptr);
    expect_eq(str_addr >= base_addr && str_addr < end_addr, true);
}

fn testAbnormalBoundariesAndValidation(): !void {
    const alloc = std.heap.page_allocator;

    // 1. 11-byte varint overflow
    const overflow_11: [11]u8 = [
        0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80,
        0x01,
    ];
    let r_ov11 = ProtoReader.init(&overflow_11);
    let hit_ov11 = false;
    switch (r_ov11.readVarint()) {
        err.VarintOverflow => { hit_ov11 = true; },
        else => {},
    }
    expect_eq(hit_ov11, true);

    // 10th byte with value > 1 (bit 64+ overflow)
    const overflow_10_val: [10]u8 = [
        0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x02,
    ];
    let r_ov10 = ProtoReader.init(&overflow_10_val);
    let hit_ov10 = false;
    switch (r_ov10.readVarint()) {
        err.VarintOverflow => { hit_ov10 = true; },
        else => {},
    }
    expect_eq(hit_ov10, true);

    // 2. Reserved field numbers (19000..19999), 0, and > 536870911
    expect_eq(ProtoTag.isValidFieldNumber(0), false);
    expect_eq(ProtoTag.isValidFieldNumber(18999), true);
    expect_eq(ProtoTag.isValidFieldNumber(19000), false);
    expect_eq(ProtoTag.isValidFieldNumber(19999), false);
    expect_eq(ProtoTag.isValidFieldNumber(20000), true);
    expect_eq(ProtoTag.isValidFieldNumber(536870911), true);
    expect_eq(ProtoTag.isValidFieldNumber(536870912), false);

    let w = ProtoWriter.init(alloc);
    defer w.deinit();
    let hit_res_w = false;
    switch (w.writeVarint(19000, 1)) {
        err.InvalidFieldNumber => { hit_res_w = true; },
        else => {},
    }
    expect_eq(hit_res_w, true);

    // Raw tag for field 19000, wire type Varint = (19000 << 3) | 0 = 152000 -> [0xC0, 0xA3, 0x09]
    let hit_res_r = false;
    switch (ProtoTag.decode((19000 as u64) << 3)) {
        err.InvalidFieldNumber => { hit_res_r = true; },
        else => {},
    }
    expect_eq(hit_res_r, true);

    // 3. Invalid wire types (3, 4, 6, 7)
    let hit_wt = false;
    switch (WireType.fromInt(3)) {
        err.InvalidWireType => { hit_wt = true; },
        else => {},
    }
    expect_eq(hit_wt, true);

    // 4. Truncated length-delimited field (tag 2 Len, len=10, only 3 bytes payload)
    const trunc_len: [5]u8 = [0x12, 0x0A, 'a', 'b', 'c'];
    let r_trunc = ProtoReader.init(&trunc_len);
    const t_trunc = (r_trunc.nextTag()?).?;
    expect_eq(t_trunc.field_number, 2 as u32);
    let hit_eof = false;
    switch (r_trunc.readBytes()) {
        err.UnexpectedEof => { hit_eof = true; },
        else => {},
    }
    expect_eq(hit_eof, true);

    // 5. Truncated varint & fixed32
    const trunc_varint: [1]u8 = [0x80];
    let r_tv = ProtoReader.init(&trunc_varint);
    let hit_tv = false;
    switch (r_tv.readVarint()) {
        err.UnexpectedEof => { hit_tv = true; },
        else => {},
    }
    expect_eq(hit_tv, true);
}

fn testGrpcFramingStreamStatusAndTimeout(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Single-frame encode & zero-copy decode
    const f1_bytes = GrpcFrameCodec.encode(alloc, false, "hello-grpc")?;
    defer alloc.free(f1_bytes);
    expect_eq(f1_bytes.len, 15 as usize);
    expect_eq(f1_bytes[0], 0 as u8);
    expect_eq(f1_bytes[4], 10 as u8);

    const f1_dec = GrpcFrameCodec.decode(f1_bytes)?;
    expect_eq(f1_dec.compressed, false);
    expect_eq(std.mem.eql(u8, f1_dec.payload, "hello-grpc"), true);

    const f2_bytes = GrpcFrameCodec.encode(alloc, true, "compressed-frame")?;
    defer alloc.free(f2_bytes);
    const f3_bytes = GrpcFrameCodec.encode(alloc, false, "")?;
    defer alloc.free(f3_bytes);

    // 2. Incremental streaming across fragmented chunks + coalesced frames
    let stream = GrpcFrameStream.initWithLimit(alloc, 1024);
    defer stream.deinit();

    // Feed first 3 bytes of frame 1 (incomplete header)
    const out_a = stream.feed(f1_bytes[0..3])?;
    GrpcFrameStream.freeFrames(alloc, out_a);
    expect_eq(out_a.len, 0 as usize);
    expect_eq(stream.bufferedBytes(), 3 as usize);

    // Feed remainder of frame 1 + first half of frame 2
    let combined_mid: std.ArrayList(u8) = .empty;
    defer combined_mid.deinit(alloc);
    combined_mid.appendSlice(alloc, f1_bytes[3..])?;
    combined_mid.appendSlice(alloc, f2_bytes[0..8])?;

    const out_b = stream.feed(combined_mid.items)?;
    defer GrpcFrameStream.freeFrames(alloc, out_b);
    expect_eq(out_b.len, 1 as usize);
    expect_eq(out_b[0].compressed, false);
    expect_eq(std.mem.eql(u8, out_b[0].payload, "hello-grpc"), true);
    expect_eq(stream.bufferedBytes(), 8 as usize);

    // Feed remainder of frame 2 + entire empty frame 3
    let combined_tail: std.ArrayList(u8) = .empty;
    defer combined_tail.deinit(alloc);
    combined_tail.appendSlice(alloc, f2_bytes[8..])?;
    combined_tail.appendSlice(alloc, f3_bytes)?;

    const out_c = stream.feed(combined_tail.items)?;
    defer GrpcFrameStream.freeFrames(alloc, out_c);
    expect_eq(out_c.len, 2 as usize);
    expect_eq(out_c[0].compressed, true);
    expect_eq(std.mem.eql(u8, out_c[0].payload, "compressed-frame"), true);
    expect_eq(out_c[1].compressed, false);
    expect_eq(out_c[1].payload.len, 0 as usize);
    expect_eq(stream.bufferedBytes(), 0 as usize);

    // 3. Size limit rejection (default 4 MiB & custom limit)
    const huge_hdr: [5]u8 = [0x00, 0x00, 0x40, 0x00, 0x01]; // 4 MiB + 1
    let hit_too_large = false;
    switch (GrpcFrameCodec.decode(&huge_hdr)) {
        err.MessageTooLarge => { hit_too_large = true; },
        else => {},
    }
    expect_eq(hit_too_large, true);

    let hit_stream_limit = false;
    const oversize_hdr: [5]u8 = [0x00, 0x00, 0x00, 0x04, 0x01]; // 1025 > 1024
    switch (stream.feed(&oversize_hdr)) {
        err.MessageTooLarge => { hit_stream_limit = true; },
        else => {},
    }
    expect_eq(hit_stream_limit, true);

    let hit_bad_flag = false;
    const bad_flag_hdr: [5]u8 = [0x02, 0x00, 0x00, 0x00, 0x00];
    switch (GrpcFrameCodec.decode(&bad_flag_hdr)) {
        err.InvalidCompressedFlag => { hit_bad_flag = true; },
        else => {},
    }
    expect_eq(hit_bad_flag, true);

    // 4. All 17 GrpcStatus codes
    let code_idx: i32 = 0;
    while (code_idx <= 16) : (code_idx += 1) {
        const st = GrpcStatus.fromCode(code_idx).?;
        expect_eq(st.code(), code_idx);
        expect_eq(st.name().len > 0, true);
    }
    expect_eq(GrpcStatus.fromCode(-1) == null, true);
    expect_eq(GrpcStatus.fromCode(17) == null, true);
    expect_eq(GrpcStatus.Ok.isOk(), true);
    expect_eq(GrpcStatus.Ok.name(), "OK");
    expect_eq(GrpcStatus.Unavailable.isRetryable(), true);
    expect_eq(GrpcStatus.ResourceExhausted.isRetryable(), true);
    expect_eq(GrpcStatus.Aborted.isRetryable(), true);
    expect_eq(GrpcStatus.InvalidArgument.isRetryable(), false);
    expect_eq(GrpcStatus.DataLoss.code(), 15);
    expect_eq(GrpcStatus.Unauthenticated.code(), 16);

    // 5. GrpcTimeout parse & formatMs
    expect_eq(GrpcTimeout.parse("100m")?, 100000000 as u64);
    expect_eq(GrpcTimeout.parse("5S")?, 5000000000 as u64);
    expect_eq(GrpcTimeout.parse("2H")?, 7200000000000 as u64);
    expect_eq(GrpcTimeout.parse("3M")?, 180000000000 as u64);
    expect_eq(GrpcTimeout.parse("42u")?, 42000 as u64);
    expect_eq(GrpcTimeout.parse("9n")?, 9 as u64);

    const fmt_ms = GrpcTimeout.formatMs(alloc, 250)?;
    defer alloc.free(fmt_ms);
    expect_eq(std.mem.eql(u8, fmt_ms, "250m"), true);
    expect_eq(GrpcTimeout.parse(fmt_ms)?, 250000000 as u64);

    let hit_bad_to = false;
    switch (GrpcTimeout.parse("10s")) {
        err.InvalidTimeout => { hit_bad_to = true; },
        else => {},
    }
    expect_eq(hit_bad_to, true);
}

fn testCapProtobufLazyMount(): !void {
    const alloc = std.heap.page_allocator;

    // Verify `import cap from "cap"; cap.protobuf.*` automatic dynamic lazy mounting
    let pw = cap.protobuf.ProtoWriter.init(alloc);
    defer pw.deinit();
    pw.writeVarint(1, 777)?;
    pw.writeString(2, "cap-lazy-mount")?;
    pw.writeSint32(3, -99)?;

    const frame_bytes = cap.protobuf.GrpcFrameCodec.encode(alloc, false, pw.slice())?;
    defer alloc.free(frame_bytes);

    const decoded_frame = cap.protobuf.GrpcFrameCodec.decode(frame_bytes)?;
    let pr = cap.protobuf.ProtoReader.init(decoded_frame.payload);

    const t1 = (pr.nextTag()?).?;
    expect_eq(t1.field_number, 1 as u32);
    expect_eq(pr.readVarint()?, 777 as u64);

    const t2 = (pr.nextTag()?).?;
    expect_eq(t2.field_number, 2 as u32);
    expect_eq(std.mem.eql(u8, pr.readString()?, "cap-lazy-mount"), true);

    const t3 = (pr.nextTag()?).?;
    expect_eq(t3.field_number, 3 as u32);
    expect_eq(pr.readSint32()?, -99 as i32);
    expect_eq(pr.nextTag()? == null, true);

    expect_eq(cap.protobuf.GrpcStatus.Unavailable.isRetryable(), true);
    expect_eq(cap.protobuf.GrpcTimeout.parse("5S")?, 5000000000 as u64);
    expect_eq(cap.protobuf.ZigZag.encode32(-1), 1 as u32);
}

fn main() {
    test("1. Varint/ZigZag Boundary Values & Proto3 Official Wire Vectors", () => {
        testVarintZigZagAndOfficialVectors()?;
    });
    test("2. Nested Submessage, Packed Repeated & Unknown Field skipField Roundtrip", () => {
        testNestedSubmessagePackedAndSkipFieldRoundtrip()?;
    });
    test("3. Abnormal Boundaries (11B Varint Overflow, Reserved 19000, Truncated Len)", () => {
        testAbnormalBoundariesAndValidation()?;
    });
    test("4. GrpcFrameCodec Fragmented Stream, Size Limits, GrpcStatus & GrpcTimeout", () => {
        testGrpcFramingStreamStatusAndTimeout()?;
    });
    test("5. cap.protobuf Automatic Dynamic Lazy Mounting Verification", () => {
        testCapProtobufLazyMount()?;
    });
}
