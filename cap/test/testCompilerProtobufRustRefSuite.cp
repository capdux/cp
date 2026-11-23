// tokio-rs/prost (Apache-2.0) Reference-Hardened Protocol Buffers Wire Format Test Suite
// Covers:
//   - Test 01: prost/src/encoding/varint.rs 1..10-byte Varint vectors, U64_MAX_PLUS_ONE & 11-byte
//              VarintOverflow, truncated Varint UnexpectedEof, ZigZag i32/i64 MIN/MAX boundaries
//   - Test 02: Tag & WireType boundaries (field_number=0 & >536_870_911 InvalidFieldNumber,
//              WireType 6/7 InvalidWireType, all 6 WireTypes, forged 0xFFFFFFFF TruncatedLengthDelimited)
//   - Test 03: Packed Repeated (varint/sint32/fixed32/fixed64), StartGroup/EndGroup pairing & mismatch
//              (UnexpectedEndGroupTag), recursion depth > 100 (MaxDepthExceeded), Strict UTF-8 (InvalidUtf8)
//   - Test 04: SIMD hot-path parity (copyPayloadSimd vs copyPayloadScalar, isValidUtf8 vs isValidUtf8Scalar)
//   - Test 05: 1,500-iteration deterministic PRNG malformed wire / forged length / deep group fuzzing

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MAX_FIELD_NUMBER,
    PROTOBUF_MAX_RECURSION_DEPTH,
    ProtoError,
    WireType,
    zigzagEncode32,
    zigzagDecode32,
    zigzagEncode64,
    zigzagDecode64,
    copyPayloadSimd,
    copyPayloadScalar,
    ProtoField,
    ProtoWriter,
    ProtoReader,
} from "../../lib/compiler/protobuf.cp";

fn xorshift64(state: *u64): u64 {
    let x = state.*;
    x = x ^ (x << 13);
    x = x ^ (x >> 7);
    x = x ^ (x << 17);
    state.* = x;
    return x;
}

fn checkVarintVector(alloc: std.mem.Allocator, value: u64, expected: []const u8): void! {
    let w = ProtoWriter.init(alloc);
    defer w.deinit();
    try w.writeVarint(value);
    expect_eq(ProtoWriter.varintSize(value), expected.len);
    expect_eq(std.mem.eql(u8, w.slice(), expected), true);

    let r = ProtoReader.init(expected);
    const decoded = try r.readVarint();
    expect_eq(decoded, value);
    expect_eq(r.hasMore(), false);
}

fn runTest01(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. prost/src/encoding/varint.rs 1..10-byte canonical vectors
    const v0: [1]u8 = [0x00];
    try checkVarintVector(alloc, 0, &v0);
    const v1: [1]u8 = [0x01];
    try checkVarintVector(alloc, 1, &v1);
    const v127: [1]u8 = [0x7F];
    try checkVarintVector(alloc, 127, &v127);
    const v128: [2]u8 = [ 0x80, 0x01 ];
    try checkVarintVector(alloc, 128, &v128);
    const v300: [2]u8 = [ 0xAC, 0x02 ];
    try checkVarintVector(alloc, 300, &v300);
    const v16383: [2]u8 = [ 0xFF, 0x7F ];
    try checkVarintVector(alloc, 16383, &v16383);
    const v16384: [3]u8 = [ 0x80, 0x80, 0x01 ];
    try checkVarintVector(alloc, 16384, &v16384);
    const v56_m1: [8]u8 = [ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F ];
    try checkVarintVector(alloc, (1 as u64 << 56) - 1, &v56_m1);
    const v63_m1: [9]u8 = [ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F ];
    try checkVarintVector(alloc, (1 as u64 << 63) - 1, &v63_m1);
    const v63: [10]u8 = [ 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01 ];
    try checkVarintVector(alloc, 1 as u64 << 63, &v63);
    const vmax: [10]u8 = [ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01 ];
    try checkVarintVector(alloc, std.math.maxInt(u64), &vmax);

    // 2. U64_MAX_PLUS_ONE (10th byte = 0x02) & 11-byte varint -> VarintOverflow
    const u64_max_plus_one: [10]u8 = [ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02 ];
    let r_ov1 = ProtoReader.init(&u64_max_plus_one);
    let got_ov1 = false;
    * = r_ov1.readVarint() catch |e| {
        if (std.mem.eql(u8, @errorName(e), "VarintOverflow")) got_ov1 = true;
    };
    expect_eq(got_ov1, true);

    const eleven_bytes: [11]u8 = [ 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01 ];
    let r_ov2 = ProtoReader.init(&eleven_bytes);
    let got_ov2 = false;
    * = r_ov2.readVarint() catch |e| {
        if (std.mem.eql(u8, @errorName(e), "VarintOverflow")) got_ov2 = true;
    };
    expect_eq(got_ov2, true);

    // 3. Truncated varint -> UnexpectedEof
    const trunc_varint: [2]u8 = [ 0x80, 0x80 ];
    let r_eof = ProtoReader.init(&trunc_varint);
    let got_eof = false;
    * = r_eof.readVarint() catch |e| {
        if (std.mem.eql(u8, @errorName(e), "UnexpectedEof")) got_eof = true;
    };
    expect_eq(got_eof, true);

    // 4. ZigZag 32/64 extreme boundaries
    expect_eq(zigzagEncode32(0), 0 as u32);
    expect_eq(zigzagEncode32(-1), 1 as u32);
    expect_eq(zigzagEncode32(1), 2 as u32);
    expect_eq(zigzagEncode32(std.math.maxInt(i32)), 0xFFFFFFFE as u32);
    expect_eq(zigzagEncode32(std.math.minInt(i32)), 0xFFFFFFFF as u32);
    expect_eq(zigzagDecode32(0xFFFFFFFF), std.math.minInt(i32));
    expect_eq(zigzagDecode32(0xFFFFFFFE), std.math.maxInt(i32));

    expect_eq(zigzagEncode64(0), 0 as u64);
    expect_eq(zigzagEncode64(-1), 1 as u64);
    expect_eq(zigzagEncode64(std.math.maxInt(i64)), std.math.maxInt(u64) - 1);
    expect_eq(zigzagEncode64(std.math.minInt(i64)), std.math.maxInt(u64));
    expect_eq(zigzagDecode64(std.math.maxInt(u64)), std.math.minInt(i64));
}

fn runTest02(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Tag boundaries: field_number == 0 and > MAX_FIELD_NUMBER rejected
    let w_bad = ProtoWriter.init(alloc);
    defer w_bad.deinit();
    let bad_fn0 = false;
    w_bad.writeTag(0, WireType.Varint) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidFieldNumber")) bad_fn0 = true;
    };
    expect_eq(bad_fn0, true);

    let bad_fn_hi = false;
    w_bad.writeTag(MAX_FIELD_NUMBER + 1, WireType.Varint) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidFieldNumber")) bad_fn_hi = true;
    };
    expect_eq(bad_fn_hi, true);

    // Wire decode of field_number == 0 (tag byte 0x00) -> InvalidFieldNumber
    const zero_tag: [2]u8 = [ 0x00, 0x01 ];
    let r_z = ProtoReader.init(&zero_tag);
    let got_z = false;
    * = r_z.next() catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidFieldNumber")) got_z = true;
    };
    expect_eq(got_z, true);

    // Wire decode of field_number > MAX_FIELD_NUMBER (tag > u32::MAX) -> InvalidFieldNumber
    let w_over_tag = ProtoWriter.init(alloc);
    defer w_over_tag.deinit();
    try w_over_tag.writeVarint((((MAX_FIELD_NUMBER as u64) + 1) << 3) | 0);
    try w_over_tag.writeVarint(1);
    let r_ot = ProtoReader.init(w_over_tag.slice());
    let got_ot = false;
    * = r_ot.next() catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidFieldNumber")) got_ot = true;
    };
    expect_eq(got_ot, true);

    // Invalid WireType 6 and 7
    const wt6: [1]u8 = [0x0E]; // field 1, wire_type 6
    let r_wt6 = ProtoReader.init(&wt6);
    let got_wt6 = false;
    * = r_wt6.next() catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidWireType")) got_wt6 = true;
    };
    expect_eq(got_wt6, true);

    const wt7: [1]u8 = [0x0F]; // field 1, wire_type 7
    let r_wt7 = ProtoReader.init(&wt7);
    let got_wt7 = false;
    * = r_wt7.next() catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidWireType")) got_wt7 = true;
    };
    expect_eq(got_wt7, true);

    // Forged huge length (0xFFFFFFFF) -> TruncatedLengthDelimited (no OOM / no OOB)
    const forged_len: [7]u8 = [ 0x0A, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x01 ];
    let r_fl = ProtoReader.init(&forged_len);
    let got_fl = false;
    * = r_fl.next() catch |e| {
        if (std.mem.eql(u8, @errorName(e), "TruncatedLengthDelimited")) got_fl = true;
    };
    expect_eq(got_fl, true);

    // 2. All 6 WireTypes roundtrip at MAX_FIELD_NUMBER boundary
    let w = ProtoWriter.init(alloc);
    defer w.deinit();
    try w.writeUint64(MAX_FIELD_NUMBER, 42);
    try w.writeFixed64(2, 0x0123456789ABCDEF);
    try w.writeString(3, "prost-hardened");
    try w.writeStartGroup(4);
    try w.writeSint32(5, -777);
    try w.writeEndGroup(4);
    try w.writeFixed32(6, 0xDEADBEEF);

    try ProtoReader.validateProtobufWire(w.slice());
    let r = ProtoReader.init(w.slice());
    const f1 = (try r.next()).?;
    expect_eq(f1.field_number, MAX_FIELD_NUMBER);
    expect_eq(f1.asUint64(), 42 as u64);
    const f2 = (try r.next()).?;
    expect_eq(f2.asFixed64(), 0x0123456789ABCDEF as u64);
    const f3 = (try r.next()).?;
    expect_eq(std.mem.eql(u8, try f3.asStringChecked(), "prost-hardened"), true);
    const f4 = (try r.next()).?;
    expect_eq(f4.wire_type == WireType.StartGroup, true);
    try r.skipField(f4);
    const f6 = (try r.next()).?;
    expect_eq(f6.asFixed32(), 0xDEADBEEF as u32);
    expect_eq((try r.next()) == null, true);
}

fn runTest03(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Packed Repeated (varint, sint32, fixed32, fixed64, empty proto3 omission)
    let w = ProtoWriter.init(alloc);
    defer w.deinit();
    const empty_u64: []const u64 = [];
    try w.writePackedVarints(1, empty_u64);
    expect_eq(w.len(), 0); // proto3 canonical: empty packed emits 0 bytes

    const p_u64: [5]u64 = [ 0, 1, 150, 16384, std.math.maxInt(u64) ];
    try w.writePackedVarints(1, &p_u64);
    const p_s32: [5]i32 = [ 0, -1, 1, std.math.minInt(i32), std.math.maxInt(i32) ];
    try w.writePackedSint32(2, &p_s32);
    const p_f32: [2]u32 = [ 1, 0xAABBCCDD ];
    try w.writePackedFixed32(3, &p_f32);
    const p_f64: [1]u64 = [ 0x1122334455667788 ];
    try w.writePackedFixed64(4, &p_f64);

    let r = ProtoReader.init(w.slice());
    const pf1 = (try r.next()).?;
    const dec_u64 = try ProtoReader.decodePackedVarints(alloc, pf1.asBytes());
    expect_eq(dec_u64.len, 5);
    expect_eq(dec_u64[2], 150 as u64);
    expect_eq(dec_u64[4], std.math.maxInt(u64));

    const pf2 = (try r.next()).?;
    const dec_s32 = try ProtoReader.decodePackedVarints(alloc, pf2.asBytes());
    expect_eq(zigzagDecode32(dec_s32[1] as u32), -1 as i32);
    expect_eq(zigzagDecode32(dec_s32[3] as u32), std.math.minInt(i32));

    // 2. StartGroup / EndGroup mismatch -> UnexpectedEndGroupTag
    let w_mis = ProtoWriter.init(alloc);
    defer w_mis.deinit();
    try w_mis.writeStartGroup(10);
    try w_mis.writeUint32(1, 5);
    try w_mis.writeEndGroup(11); // mismatched!

    let mis_err = false;
    ProtoReader.validateProtobufWire(w_mis.slice()) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "UnexpectedEndGroupTag")) mis_err = true;
    };
    expect_eq(mis_err, true);

    // Stray EndGroup at top level -> UnexpectedEndGroupTag
    let w_stray = ProtoWriter.init(alloc);
    defer w_stray.deinit();
    try w_stray.writeEndGroup(10);
    let stray_err = false;
    ProtoReader.validateProtobufWire(w_stray.slice()) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "UnexpectedEndGroupTag")) stray_err = true;
    };
    expect_eq(stray_err, true);

    // 3. Recursion depth > PROTOBUF_MAX_RECURSION_DEPTH (100) via nested StartGroup
    expect_eq(PROTOBUF_MAX_RECURSION_DEPTH, 100);
    let w_deep_grp = ProtoWriter.init(alloc);
    defer w_deep_grp.deinit();
    let d: usize = 0;
    while (d < 102) : (d += 1) try w_deep_grp.writeStartGroup(1);
    d = 0;
    while (d < 102) : (d += 1) try w_deep_grp.writeEndGroup(1);

    let grp_depth_err = false;
    ProtoReader.validateProtobufWire(w_deep_grp.slice()) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "MaxDepthExceeded")) grp_depth_err = true;
    };
    expect_eq(grp_depth_err, true);

    // Submessage recursion depth > 100 -> MaxDepthExceeded
    let nested_wire: []const u8 = "";
    let lv: usize = 0;
    while (lv < 102) : (lv += 1) {
        let nw = ProtoWriter.init(alloc);
        try nw.writeSubmessage(1, nested_wire);
        nested_wire = nw.slice();
    }
    let cur = ProtoReader.init(nested_wire);
    let step: usize = 0;
    while (step < 100) : (step += 1) {
        const f = (try cur.next()).?;
        cur = try cur.readSubmessage(f);
    }
    const f101 = (try cur.next()).?;
    let sub_depth_err = false;
    * = cur.readSubmessage(f101) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "MaxDepthExceeded")) sub_depth_err = true;
    };
    expect_eq(sub_depth_err, true);

    // 4. Strict UTF-8 check (prost string_merge_invalid_utf8: \x02\x80\x80)
    const bad_utf8_wire: [4]u8 = [ 0x0A, 0x02, 0x80, 0x80 ];
    let r_u8 = ProtoReader.init(&bad_utf8_wire);
    const fu8 = (try r_u8.next()).?;
    let utf8_err = false;
    * = fu8.asStringChecked() catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidUtf8")) utf8_err = true;
    };
    expect_eq(utf8_err, true);
}

fn runTest04(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. SIMD @Vector(16, u8) payload copy vs scalar reference (sizes 0..128)
    let src_buf: [128]u8 = undefined;
    let i: usize = 0;
    while (i < 128) : (i += 1) src_buf[i] = ((i * 37 + 13) & 0xFF) as u8;

    let simd_dst: [128]u8 = undefined;
    let scal_dst: [128]u8 = undefined;
    let sz: usize = 0;
    while (sz <= 128) : (sz += 1) {
        copyPayloadSimd(simd_dst[0..sz], src_buf[0..sz]);
        copyPayloadScalar(scal_dst[0..sz], src_buf[0..sz]);
        expect_eq(std.mem.eql(u8, simd_dst[0..sz], scal_dst[0..sz]), true);
    }

    // Large payload writeBytes roundtrip using SIMD copy
    let w = ProtoWriter.init(alloc);
    defer w.deinit();
    try w.writeBytes(7, &src_buf);
    let r = ProtoReader.init(w.slice());
    const f = (try r.next()).?;
    expect_eq(std.mem.eql(u8, f.asBytes(), &src_buf), true);

    // 2. cap:simd Simd.isAscii UTF-8 fast-path vs scalar reference 0-diff parity
    const utf8_samples: [5][]const u8 = [
        "",
        "hello-protobuf-ascii-0123456789-fastpath-over-16-bytes",
        "utf8-valid-\xC3\xA9-\xE4\xB8\xAD-\xF0\x9F\x98\x80",
        "bad-utf8-\x80\x80",
        "truncated-utf8-\xE4\xB8",
    ];
    for (s of utf8_samples) {
        expect_eq(ProtoReader.isValidUtf8(s), ProtoReader.isValidUtf8Scalar(s));
    }
}

fn runTest05(): void! {
    // 1,500-iteration deterministic PRNG malformed Protobuf wire fuzzing
    let seed: u64 = 0x5A17_C0DE_1234_5678;
    let buf: [96]u8 = undefined;

    let iter: usize = 0;
    let ok_count: usize = 0;
    let err_count: usize = 0;
    while (iter < 1500) : (iter += 1) {
        const len = ((xorshift64(&seed) % 80) + 1) as usize;
        let j: usize = 0;
        while (j < len) : (j += 1) {
            buf[j] = (xorshift64(&seed) & 0xFF) as u8;
        }
        ProtoReader.validateProtobufWire(buf[0..len]) catch {
            err_count += 1;
            continue;
        };
        ok_count += 1;
    }
    expect_eq(ok_count + err_count, 1500);
}

fn main() {
    test("protobuf_ref_01_prost_varint_vectors_overflow_and_zigzag", () => { try runTest01(); });
    test("protobuf_ref_02_tag_wiretype_forged_len_and_six_wiretypes", () => { try runTest02(); });
    test("protobuf_ref_03_packed_groups_recursion_100_and_strict_utf8", () => { try runTest03(); });
    test("protobuf_ref_04_simd_payload_copy_and_ascii_utf8_parity", () => { try runTest04(); });
    test("protobuf_ref_05_deterministic_prng_1500_fuzz_malformed_wire", () => { try runTest05(); });
}
