// Source: cp/cap/test/test_amqp_production_deep.cp
// Deep Production Test Suite for cap:amqp (AMQP 0-9-1 Binary Wire Framing, FieldTable & Stream Decoder)
// Covers:
//   1. Method (Basic.Publish), ContentHeader (content_type + FieldTable), Body & Heartbeat round-trip
//   2. FieldTable nested AmqpValue (boolean, i32, i64, f64, long_str, field_array, field_table, void_val) round-trip
//   3. AmqpStreamDecoder 1..5 byte arbitrary chunk half-packet & sticky-packet framing
//   4. Malformed & security guards: missing 0xCE (InvalidFrameEnd), FrameTooLarge, InvalidFrameType, Heartbeat non-0 channel (InvalidChannel), >32-level FieldTable (MaxDepthExceeded)
//   5. Dynamic lazy namespace mounting (`import cap from "cap"; cap.amqp`) & `std.testing.allocator` zero memory leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    PROTOCOL_HEADER,
    FRAME_END,
    AMQP_MAX_DEPTH,
    AmqpError,
    AmqpFrameType,
    AmqpField,
    AmqpValue,
    AmqpFrame,
    AmqpStreamDecoder,
    deinitFieldTable,
    getField,
    eqlFieldTable,
    encodeFieldTable,
    decodeFieldTable,
    encodeFrame,
    encodeMethodFrame,
    decodeMethodPayload,
    encodeContentHeader,
    decodeContentHeader,
    encodeHeartbeat,
} from "cap:net";
import cap from "cap";

test("1. Method (Basic.Publish), ContentHeader, Body & Heartbeat Frame Round-Trip") {
    const alloc = std.testing.allocator;

    expect_eq(PROTOCOL_HEADER.len, 8 as usize);
    expect_eq(std.mem.eql(u8, PROTOCOL_HEADER, "AMQP\x00\x00\x09\x01"), true);
    expect_eq(FRAME_END, 0xCE as u8);

    // Basic.Publish: class_id = 60, method_id = 40
    const publish_args = "\x00\x00\x06events\x07user.up\x00";
    const method_wire = encodeMethodFrame(alloc, 1, 60, 40, publish_args) catch unreachable;
    defer alloc.free(method_wire);

    let hdr_fields: [2]AmqpField = [
        AmqpField.init("x-retry", AmqpValue{ i32_val: 3 }),
        AmqpField.init("x-source", AmqpValue{ long_str: "gateway" }),
    ];
    const body_payload = "{\"ok\":true}";
    const header_wire = encodeContentHeader(alloc, 1, 60, body_payload.len as u64, "application/json", &hdr_fields) catch unreachable;
    defer alloc.free(header_wire);

    const body_wire = encodeFrame(alloc, .Body, 1, body_payload) catch unreachable;
    defer alloc.free(body_wire);

    const hb_wire = encodeHeartbeat(alloc) catch unreachable;
    defer alloc.free(hb_wire);
    expect_eq(hb_wire.len, 8 as usize);

    let decoder = AmqpStreamDecoder.init(alloc, 65536);
    defer decoder.deinit();

    decoder.feed(method_wire) catch unreachable;
    decoder.feed(header_wire) catch unreachable;
    decoder.feed(body_wire) catch unreachable;
    decoder.feed(hb_wire) catch unreachable;

    // Frame 1: Method
    if (f1 := decoder.nextFrame() catch unreachable) {
        defer f1.deinit(alloc);
        expect_eq(f1.frame_type == .Method, true);
        expect_eq(f1.channel, 1 as u16);
        const mp = decodeMethodPayload(f1.payload) catch unreachable;
        expect_eq(mp.class_id, 60 as u16);
        expect_eq(mp.method_id, 40 as u16);
        expect_eq(std.mem.eql(u8, mp.args, publish_args), true);
    } else {
        expect_eq(true, false);
    }

    // Frame 2: ContentHeader
    if (f2 := decoder.nextFrame() catch unreachable) {
        defer f2.deinit(alloc);
        expect_eq(f2.frame_type == .Header, true);
        expect_eq(f2.channel, 1 as u16);
        const ch = decodeContentHeader(alloc, f2.payload) catch unreachable;
        defer ch.deinit(alloc);
        expect_eq(ch.class_id, 60 as u16);
        expect_eq(ch.body_size, body_payload.len as u64);
        if (ct := ch.content_type) {
            expect_eq(std.mem.eql(u8, ct, "application/json"), true);
        } else {
            expect_eq(true, false);
        }
        expect_eq(eqlFieldTable(ch.headers, &hdr_fields), true);
    } else {
        expect_eq(true, false);
    }

    // Frame 3: Body
    if (f3 := decoder.nextFrame() catch unreachable) {
        defer f3.deinit(alloc);
        expect_eq(f3.frame_type == .Body, true);
        expect_eq(f3.channel, 1 as u16);
        expect_eq(std.mem.eql(u8, f3.payload, body_payload), true);
    } else {
        expect_eq(true, false);
    }

    // Frame 4: Heartbeat
    if (f4 := decoder.nextFrame() catch unreachable) {
        defer f4.deinit(alloc);
        expect_eq(f4.frame_type == .Heartbeat, true);
        expect_eq(f4.channel, 0 as u16);
        expect_eq(f4.payload.len, 0 as usize);
    } else {
        expect_eq(true, false);
    }

    expect_eq((decoder.nextFrame() catch unreachable) == null, true);
}

test("2. Nested FieldTable & AmqpValue All Types Encode/Decode Round-Trip") {
    const alloc = std.testing.allocator;

    let nested_tbl: [2]AmqpField = [
        AmqpField.init("region", AmqpValue{ long_str: "ap-southeast-2" }),
        AmqpField.init("active", AmqpValue{ boolean: true }),
    ];
    let arr_items: [3]AmqpValue = [
        AmqpValue{ i32_val: -42 },
        AmqpValue{ long_str: "tag-alpha" },
        AmqpValue{ field_table: &nested_tbl },
    ];
    let root_fields: [8]AmqpField = [
        AmqpField.init("flag", AmqpValue{ boolean: false }),
        AmqpField.init("count32", AmqpValue{ i32_val: 102400 }),
        AmqpField.init("count64", AmqpValue{ i64_val: -9007199254740993 }),
        AmqpField.init("ratio", AmqpValue{ f64_val: 3.141592653589793 }),
        AmqpField.init("msg", AmqpValue{ long_str: "hello amqp 0-9-1" }),
        AmqpField.init("tags", AmqpValue{ field_array: &arr_items }),
        AmqpField.init("meta", AmqpValue{ field_table: &nested_tbl }),
        AmqpField.init("empty", .void_val),
    ];

    const encoded = encodeFieldTable(alloc, &root_fields) catch unreachable;
    defer alloc.free(encoded);

    const decoded = decodeFieldTable(alloc, encoded) catch unreachable;
    defer deinitFieldTable(alloc, decoded);

    expect_eq(eqlFieldTable(&root_fields, decoded), true);

    if (meta_val := getField(decoded, "meta")) {
        if (reg := meta_val.get("region")) {
            expect_eq(reg.eql(AmqpValue{ long_str: "ap-southeast-2" }), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
    expect_eq(getField(decoded, "nonexistent") == null, true);
}

test("3. AmqpStreamDecoder 1..5 Byte Arbitrary Half-Packet & Sticky-Packet Framing") {
    const alloc = std.testing.allocator;

    const f1_wire = encodeMethodFrame(alloc, 3, 10, 10, "conn-start") catch unreachable;
    defer alloc.free(f1_wire);
    const f2_wire = encodeFrame(alloc, .Body, 3, "payload-chunk-1234567890") catch unreachable;
    defer alloc.free(f2_wire);
    const f3_wire = encodeHeartbeat(alloc) catch unreachable;
    defer alloc.free(f3_wire);

    let combined: std.ArrayListUnmanaged(u8) = .{};
    defer combined.deinit(alloc);
    combined.appendSlice(alloc, f1_wire) catch unreachable;
    combined.appendSlice(alloc, f2_wire) catch unreachable;
    combined.appendSlice(alloc, f3_wire) catch unreachable;

    let decoder = AmqpStreamDecoder.init(alloc, 4096);
    defer decoder.deinit();

    let decoded_count: usize = 0;
    let pos: usize = 0;
    let step: usize = 1;
    while (pos < combined.items.len) {
        let end = pos + step;
        if (end > combined.items.len) end = combined.items.len;
        decoder.feed(combined.items[pos..end]) catch unreachable;
        pos = end;
        step = (step % 5) + 1;

        while (true) {
            const maybe = decoder.nextFrame() catch unreachable;
            if (fr := maybe) {
                defer fr.deinit(alloc);
                if (decoded_count == 0) {
                    expect_eq(fr.frame_type == .Method, true);
                    expect_eq(fr.channel, 3 as u16);
                } else if (decoded_count == 1) {
                    expect_eq(fr.frame_type == .Body, true);
                    expect_eq(std.mem.eql(u8, fr.payload, "payload-chunk-1234567890"), true);
                } else if (decoded_count == 2) {
                    expect_eq(fr.frame_type == .Heartbeat, true);
                    expect_eq(fr.channel, 0 as u16);
                }
                decoded_count += 1;
            } else {
                break;
            }
        }
    }
    expect_eq(decoded_count, 3 as usize);
}

test("4. Malformed Frame & Security Guards (InvalidFrameEnd, FrameTooLarge, InvalidFrameType, InvalidChannel, MaxDepthExceeded)") {
    const alloc = std.testing.allocator;

    // 4.1 InvalidChannel on Heartbeat with non-zero channel
    let hit_invalid_channel = false;
    const dummy_bytes = encodeFrame(alloc, .Heartbeat, 2, "") catch |e| {
        if (e == err.InvalidChannel) hit_invalid_channel = true;
        ""
    };
    * = dummy_bytes;
    expect_eq(hit_invalid_channel, true);

    // 4.2 InvalidFrameEnd (corrupted 0xCE trailer)
    let bad_end = encodeFrame(alloc, .Body, 1, "abc") catch unreachable;
    defer alloc.free(bad_end);
    bad_end[bad_end.len - 1] = 0xCF;

    let dec1 = AmqpStreamDecoder.init(alloc, 1024);
    defer dec1.deinit();
    dec1.feed(bad_end) catch unreachable;
    let hit_bad_end = false;
    * = dec1.nextFrame() catch |e| {
        if (e == err.InvalidFrameEnd) hit_bad_end = true;
        null
    };
    expect_eq(hit_bad_end, true);

    // 4.3 FrameTooLarge
    const big_wire = encodeFrame(alloc, .Body, 1, "0123456789ABCDEF") catch unreachable;
    defer alloc.free(big_wire);
    let dec2 = AmqpStreamDecoder.init(alloc, 8);
    defer dec2.deinit();
    dec2.feed(big_wire) catch unreachable;
    let hit_too_large = false;
    * = dec2.nextFrame() catch |e| {
        if (e == err.FrameTooLarge) hit_too_large = true;
        null
    };
    expect_eq(hit_too_large, true);

    // 4.4 InvalidFrameType
    let bad_type = encodeFrame(alloc, .Body, 1, "xy") catch unreachable;
    defer alloc.free(bad_type);
    bad_type[0] = 99;
    let dec3 = AmqpStreamDecoder.init(alloc, 1024);
    defer dec3.deinit();
    dec3.feed(bad_type) catch unreachable;
    let hit_bad_type = false;
    * = dec3.nextFrame() catch |e| {
        if (e == err.InvalidFrameType) hit_bad_type = true;
        null
    };
    expect_eq(hit_bad_type, true);

    // 4.5 MaxDepthExceeded (> 32 nested FieldTables)
    // Innermost empty FieldTable: [0,0,0,0] (4 bytes)
    // Each wrapper FieldTable: [len:4 BE][shortstr "a": 1,'a'][tag 'F'][inner...] -> +7 bytes per level
    const levels: usize = AMQP_MAX_DEPTH + 1;
    let deep_buf: std.ArrayListUnmanaged(u8) = .{};
    defer deep_buf.deinit(alloc);

    let total_payload: usize = (4 as usize) + levels * (7 as usize);
    let l: usize = 0;
    while (l < levels) : (l += 1) {
        total_payload -= 4;
        deep_buf.append(alloc, ((total_payload >> 24) & 0xFF) as u8) catch unreachable;
        deep_buf.append(alloc, ((total_payload >> 16) & 0xFF) as u8) catch unreachable;
        deep_buf.append(alloc, ((total_payload >> 8) & 0xFF) as u8) catch unreachable;
        deep_buf.append(alloc, (total_payload & 0xFF) as u8) catch unreachable;
        deep_buf.append(alloc, 1) catch unreachable;
        deep_buf.append(alloc, 'a') catch unreachable;
        deep_buf.append(alloc, 'F') catch unreachable;
        total_payload -= 3;
    }
    deep_buf.appendSlice(alloc, "\x00\x00\x00\x00") catch unreachable;

    let empty_out: [0]AmqpField = [];
    let hit_max_depth = false;
    * = decodeFieldTable(alloc, deep_buf.items) catch |e| {
        if (e == err.MaxDepthExceeded) hit_max_depth = true;
        &empty_out
    };
    expect_eq(hit_max_depth, true);
}

test("5. Dynamic `cap.amqp` Namespace Mounting & Zero-Leak Round-Trip") {
    const alloc = std.testing.allocator;

    let headers: [1]cap.net.AmqpField = [
        cap.net.AmqpField.init("x-app", cap.net.AmqpValue{ long_str: "cp-amqp" }),
    ];
    const wire = cap.net.encodeContentHeader(alloc, 5, 60, 42, "text/plain", &headers) catch unreachable;
    defer alloc.free(wire);

    let decoder = cap.net.AmqpStreamDecoder.init(alloc, 8192);
    defer decoder.deinit();
    decoder.feed(wire) catch unreachable;

    if (fr := decoder.nextFrame() catch unreachable) {
        defer fr.deinit(alloc);
        expect_eq(fr.channel, 5 as u16);
        const ch = cap.net.decodeContentHeader(alloc, fr.payload) catch unreachable;
        defer ch.deinit(alloc);
        expect_eq(ch.body_size, 42 as u64);
        expect_eq(cap.net.eqlFieldTable(ch.headers, &headers), true);
    } else {
        expect_eq(true, false);
    }
}
