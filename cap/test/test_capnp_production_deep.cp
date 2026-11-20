// Source: cp/cap/test/test_capnp_production_deep.cp (Deep integration tests for cap:capnp)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CapnpError,
    ElementSize,
    TraversalLimit,
    MessageBuilder,
    MessageReader,
    encodeStructPointer,
    encodeFarPointer,
    writeU32LE,
    writeU64LE,
} from "cap:capnp";

test("capnp multi-segment framing even/odd padding roundtrip", () => {
    const alloc = std.testing.allocator;

    // 2 segments (even -> 4B header padding: 4 + 2*4 + 4 = 16 bytes header)
    let mb_even = MessageBuilder.init(alloc);
    defer mb_even.deinit();
    let root_even = try mb_even.initRoot(1, 0);
    root_even.writeU32(0, 0x1234_5678);
    const seg1 = try mb_even.addSegment(3);
    expect_eq(seg1, @as(u32, 1));

    const bytes_even = try mb_even.toBytes(alloc);
    defer alloc.free(bytes_even);

    // Header must be 16 bytes + (2 words + 3 words)*8 = 16 + 40 = 56 bytes
    expect_eq(bytes_even.len, @as(usize, 56));

    let mr_even = try MessageReader.initDefault(alloc, bytes_even);
    defer mr_even.deinit();
    expect_eq(mr_even.segmentCount(), @as(usize, 2));
    expect_eq(mr_even.segmentWords(0), @as(usize, 2));
    expect_eq(mr_even.segmentWords(1), @as(usize, 3));
    const r_even = try mr_even.getRoot();
    expect_eq(r_even.readU32(0), @as(u32, 0x1234_5678));

    // 3 segments (odd -> no padding: 4 + 3*4 = 16 bytes header)
    let mb_odd = MessageBuilder.init(alloc);
    defer mb_odd.deinit();
    let root_odd = try mb_odd.initRoot(1, 0);
    root_odd.writeU64(0, 0xCAFE_BABE_DEAD_BEEF);
    * = try mb_odd.addSegment(2);
    * = try mb_odd.addSegment(4);

    const bytes_odd = try mb_odd.toBytes(alloc);
    defer alloc.free(bytes_odd);
    expect_eq(bytes_odd.len, @as(usize, 16 + (2 + 2 + 4) * 8));

    let mr_odd = try MessageReader.initDefault(alloc, bytes_odd);
    defer mr_odd.deinit();
    expect_eq(mr_odd.segmentCount(), @as(usize, 3));
    const r_odd = try mr_odd.getRoot();
    expect_eq(r_odd.readU64(0), @as(u64, 0xCAFE_BABE_DEAD_BEEF));
});

test("capnp struct scalars, XOR defaults, Text/Data and nested pointers", () => {
    const alloc = std.testing.allocator;
    let mb = MessageBuilder.init(alloc);
    defer mb.deinit();

    let root = try mb.initRoot(3, 3);
    root.writeBool(0, true);
    root.writeBool(7, true);
    root.writeU8(1, 0xAB);
    root.writeU16(1, 0xBEEF);
    root.writeU32OrDefault(1, 42, 0xFFFF_0000);
    root.writeI32(2, -123456);
    root.writeU64OrDefault(2, 999999, 0x5555_5555_5555_5555);

    try root.writeText(0, "capnp-cp-rocks");
    try root.writeData(1, "\x01\x02\x03\x04");
    let child = try root.initStruct(2, 1, 1);
    child.writeU32(0, 777);
    try child.writeText(0, "nested-child");

    const bytes = try mb.toBytes(alloc);
    defer alloc.free(bytes);

    let mr = try MessageReader.initDefault(alloc, bytes);
    defer mr.deinit();
    const r = try mr.getRoot();

    expect_eq(r.readBool(0), true);
    expect_eq(r.readBool(1), false);
    expect_eq(r.readBool(7), true);
    expect_eq(r.readU8(1), @as(u8, 0xAB));
    expect_eq(r.readU16(1), @as(u16, 0xBEEF));
    expect_eq(r.readU32OrDefault(1, 0xFFFF_0000), @as(u32, 42));
    expect_eq(r.readI32(2), @as(i32, -123456));
    expect_eq(r.readU64OrDefault(2, 0x5555_5555_5555_5555), @as(u64, 999999));

    const txt = try r.readText(0);
    expect_eq(std.mem.eql(u8, txt, "capnp-cp-rocks"), true);
    const bin = try r.readData(1);
    expect_eq(bin.len, @as(usize, 4));
    expect_eq(bin[3], @as(u8, 4));

    const c_opt = try r.readStruct(2);
    expect_eq(c_opt != null, true);
    const c = c_opt.?;
    expect_eq(c.readU32(0), @as(u32, 777));
    expect_eq(std.mem.eql(u8, try c.readText(0), "nested-child"), true);
});

test("capnp lists Bit, FourBytes, Pointer, and Composite roundtrip", () => {
    const alloc = std.testing.allocator;
    let mb = MessageBuilder.init(alloc);
    defer mb.deinit();

    let root = try mb.initRoot(0, 4);

    // 0: Bit list
    let bits = try root.initList(0, .Bit, 10);
    try bits.setBool(0, true);
    try bits.setBool(3, true);
    try bits.setBool(9, true);

    // 1: FourBytes list
    let u32s = try root.initList(1, .FourBytes, 3);
    try u32s.setU32(0, 100);
    try u32s.setU32(1, 200);
    try u32s.setU32(2, 300);

    // 2: Pointer (Text) list
    let texts = try root.initList(2, .Pointer, 2);
    try texts.setText(0, "alpha");
    try texts.setText(1, "beta");

    // 3: Composite struct list
    let comp = try root.initCompositeList(3, 2, 1, 1);
    let e0 = try comp.getStruct(0);
    e0.writeU32(0, 11);
    try e0.writeText(0, "first");
    let e1 = try comp.getStruct(1);
    e1.writeU32(0, 22);
    try e1.writeText(0, "second");

    const bytes = try mb.toBytes(alloc);
    defer alloc.free(bytes);

    let mr = try MessageReader.initDefault(alloc, bytes);
    defer mr.deinit();
    const r = try mr.getRoot();

    const r_bits = (try r.readList(0)).?;
    expect_eq(r_bits.len(), @as(u32, 10));
    expect_eq(try r_bits.getBool(0), true);
    expect_eq(try r_bits.getBool(1), false);
    expect_eq(try r_bits.getBool(3), true);
    expect_eq(try r_bits.getBool(9), true);

    const r_u32s = (try r.readList(1)).?;
    expect_eq(try r_u32s.getU32(0), @as(u32, 100));
    expect_eq(try r_u32s.getU32(1), @as(u32, 200));
    expect_eq(try r_u32s.getU32(2), @as(u32, 300));

    const r_texts = (try r.readList(2)).?;
    expect_eq(std.mem.eql(u8, try r_texts.getText(0), "alpha"), true);
    expect_eq(std.mem.eql(u8, try r_texts.getText(1), "beta"), true);

    const r_comp = (try r.readList(3)).?;
    expect_eq(r_comp.len(), @as(u32, 2));
    const re0 = try r_comp.getStruct(0);
    expect_eq(re0.readU32(0), @as(u32, 11));
    expect_eq(std.mem.eql(u8, try re0.readText(0), "first"), true);
    const re1 = try r_comp.getStruct(1);
    expect_eq(re1.readU32(0), @as(u32, 22));
    expect_eq(std.mem.eql(u8, try re1.readText(0), "second"), true);
});

test("capnp Far Pointer cross-segment, cycle TraversalLimit, and out-of-bounds segment", () => {
    const alloc = std.testing.allocator;

    // 1) Far Pointer cross-segment jump
    let mb = MessageBuilder.init(alloc);
    defer mb.deinit();
    let root = try mb.initRoot(1, 1);
    root.writeU32(0, 10);
    const seg1 = try mb.addSegment(0);
    let far_child = try root.initStructInSegment(0, seg1, 1, 0);
    far_child.writeU32(0, 9876);

    const bytes = try mb.toBytes(alloc);
    defer alloc.free(bytes);

    let mr = try MessageReader.initDefault(alloc, bytes);
    defer mr.deinit();
    const r = try mr.getRoot();
    expect_eq(r.readU32(0), @as(u32, 10));
    const fc = (try r.readStruct(0)).?;
    expect_eq(fc.readU32(0), @as(u32, 9876));

    // 2) Self-referential cycle pointer -> TraversalLimitExceeded / NestingLimitExceeded
    // Frame: 1 seg, 2 words:
    // word 0: root struct at word 1 (offset = 0, data = 0, ptr = 1)
    // word 1: struct pointer with offset = -1 (points back to word 1 itself!), data = 0, ptr = 1
    let cycle_buf: [24]u8 = undefined;
    @memset(&cycle_buf, 0);
    writeU32LE(&cycle_buf, 0, 0); // 1 segment
    writeU32LE(&cycle_buf, 4, 2); // 2 words
    writeU64LE(&cycle_buf, 8, encodeStructPointer(0, 0, 1));
    writeU64LE(&cycle_buf, 16, encodeStructPointer(-1, 0, 1));

    let mr_cycle = try MessageReader.init(alloc, &cycle_buf, TraversalLimit.init(5, 64));
    defer mr_cycle.deinit();
    let cur = try mr_cycle.getRoot();
    let hit_limit = false;
    let step: usize = 0;
    while (step < 10) {
        const next_res = cur.readStruct(0) catch |e| {
            if (e == CapnpError.TraversalLimitExceeded or e == CapnpError.NestingLimitExceeded) {
                hit_limit = true;
            }
            break;
        };
        if (next_res == null) {
            break;
        }
        cur = next_res.?;
        step += 1;
    }
    expect_eq(hit_limit, true);

    // 3) Out-of-bounds Far pointer segment -> InvalidFarPointer
    let bad_far: [24]u8 = undefined;
    @memset(&bad_far, 0);
    writeU32LE(&bad_far, 0, 0); // 1 segment
    writeU32LE(&bad_far, 4, 2);
    writeU64LE(&bad_far, 8, encodeStructPointer(0, 0, 1));
    writeU64LE(&bad_far, 16, encodeFarPointer(false, 0, 99)); // segment 99 does not exist

    let mr_bad = try MessageReader.initDefault(alloc, &bad_far);
    defer mr_bad.deinit();
    const r_bad = try mr_bad.getRoot();
    let got_bad_far = false;
    while (true) {
        * = r_bad.readStruct(0) catch |e| {
            if (e == CapnpError.InvalidFarPointer) {
                got_bad_far = true;
            }
            break;
        };
        break;
    }
    expect_eq(got_bad_far, true);
});

test("cap.capnp dynamic namespace mount and zero-leak check", () => {
    const alloc = std.testing.allocator;
    let mb = cap.capnp.MessageBuilder.init(alloc);
    defer mb.deinit();

    let root = try mb.initRoot(1, 1);
    root.writeU64(0, 0x1122_3344_5566_7788);
    try root.writeText(0, "via-cap-namespace");

    const bytes = try mb.toBytes(alloc);
    defer alloc.free(bytes);

    let mr = try cap.capnp.MessageReader.initDefault(alloc, bytes);
    defer mr.deinit();
    const r = try mr.getRoot();
    expect_eq(r.readU64(0), @as(u64, 0x1122_3344_5566_7788));
    expect_eq(std.mem.eql(u8, try r.readText(0), "via-cap-namespace"), true);
});
