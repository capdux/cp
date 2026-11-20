// Source: cp/cap/test/test_http3_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:http3 & cap.http3

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    QPACK_MAX_TABLE_CAPACITY,
    MAX_FIELD_SECTION_SIZE,
    QPACK_BLOCKED_STREAMS,
    Http3Error,
    H3StreamType,
    H3FrameType,
    H3Setting,
    H3PushPromiseFrame,
    H3UnknownFrame,
    H3Frame,
    H3Header,
    parseStreamType,
    encodeFrame,
    decodeFrames,
    decodeFramesWithMaxSize,
    freeFrames,
    encodeQpackHeaders,
    decodeQpackHeaders,
} from "cap:http3";

fn appendSlice(list: *std.ArrayList(u8), alloc: std.mem.Allocator, data: []const u8): !void {
    list.appendSlice(alloc, data) catch return Http3Error.OutOfMemory;
}

fn runTestFramesAndGrease(): !void {
    const alloc = std.testing.allocator;

    // Stream type validation
    expect_eq(parseStreamType(0x00)?, H3StreamType.Control);
    expect_eq(parseStreamType(0x02)?, H3StreamType.QpackEncoder);
    expect_eq(parseStreamType(0x03)?, H3StreamType.QpackDecoder);
    let bad_stream = false;
    * = parseStreamType(0x99) catch |e| {
        if (e == Http3Error.InvalidStreamType) bad_stream = true;
    };
    expect_eq(bad_stream, true);

    let wire: std.ArrayList(u8) = .empty;
    defer wire.deinit(alloc);

    const settings_list: [3]H3Setting = [
        H3Setting { id: QPACK_MAX_TABLE_CAPACITY, value: 4096 },
        H3Setting { id: MAX_FIELD_SECTION_SIZE, value: 65536 },
        H3Setting { id: QPACK_BLOCKED_STREAMS, value: 16 },
    ];

    const f_settings = encodeFrame(alloc, H3Frame { settings: &settings_list })?;
    defer alloc.free(f_settings);
    appendSlice(&wire, alloc, f_settings)?;

    // GREASE unknown frame type (0x1f * 2 + 0x21 = 0x5f)
    const f_grease = encodeFrame(alloc, H3Frame {
        unknown: H3UnknownFrame { frame_type: 0x5f, payload: "grease-data" },
    })?;
    defer alloc.free(f_grease);
    appendSlice(&wire, alloc, f_grease)?;

    const f_headers = encodeFrame(alloc, H3Frame { headers: "\x00\x00\xd1\xd7" })?;
    defer alloc.free(f_headers);
    appendSlice(&wire, alloc, f_headers)?;

    const f_data = encodeFrame(alloc, H3Frame { data: "hello-http3-body" })?;
    defer alloc.free(f_data);
    appendSlice(&wire, alloc, f_data)?;

    const f_cancel = encodeFrame(alloc, H3Frame { cancel_push: 7 })?;
    defer alloc.free(f_cancel);
    appendSlice(&wire, alloc, f_cancel)?;

    const f_goaway = encodeFrame(alloc, H3Frame { goaway: 100 })?;
    defer alloc.free(f_goaway);
    appendSlice(&wire, alloc, f_goaway)?;

    const f_max_push = encodeFrame(alloc, H3Frame { max_push_id: 255 })?;
    defer alloc.free(f_max_push);
    appendSlice(&wire, alloc, f_max_push)?;

    const decoded = decodeFrames(alloc, wire.items)?;
    defer freeFrames(alloc, decoded);

    expect_eq(decoded.len, 7 as usize);

    switch (decoded[0]) {
        .settings => |s| {
            expect_eq(s.len, 3 as usize);
            expect_eq(s[0].id, QPACK_MAX_TABLE_CAPACITY);
            expect_eq(s[0].value, 4096 as u64);
            expect_eq(s[1].id, MAX_FIELD_SECTION_SIZE);
            expect_eq(s[1].value, 65536 as u64);
            expect_eq(s[2].id, QPACK_BLOCKED_STREAMS);
            expect_eq(s[2].value, 16 as u64);
        },
        else => unreachable,
    }
    switch (decoded[1]) {
        .unknown => |unk| {
            expect_eq(unk.frame_type, 0x5f as u64);
            expect_eq(std.mem.eql(u8, unk.payload, "grease-data"), true);
        },
        else => unreachable,
    }
    switch (decoded[2]) {
        .headers => |h| {
            expect_eq(std.mem.eql(u8, h, "\x00\x00\xd1\xd7"), true);
        },
        else => unreachable,
    }
    switch (decoded[3]) {
        .data => |d| {
            expect_eq(std.mem.eql(u8, d, "hello-http3-body"), true);
        },
        else => unreachable,
    }
    switch (decoded[4]) {
        .cancel_push => |pid| {
            expect_eq(pid, 7 as u64);
        },
        else => unreachable,
    }
    switch (decoded[5]) {
        .goaway => |gid| {
            expect_eq(gid, 100 as u64);
        },
        else => unreachable,
    }
    switch (decoded[6]) {
        .max_push_id => |mpid| {
            expect_eq(mpid, 255 as u64);
        },
        else => unreachable,
    }
}

fn runTestQpackStaticAndLiterals(): !void {
    const alloc = std.testing.allocator;

    const req_headers: [8]H3Header = [
        H3Header { name: ":method", value: "GET" },              // Static exact index 17
        H3Header { name: ":scheme", value: "https" },            // Static exact index 23
        H3Header { name: ":authority", value: "api.example.com" }, // Static name ref index 0
        H3Header { name: ":path", value: "/v1/chat/completions" }, // Static name ref index 1
        H3Header { name: "accept-encoding", value: "gzip, deflate, br" }, // Static exact index 31
        H3Header { name: "content-type", value: "application/json" }, // Static exact index 46
        H3Header { name: "user-agent", value: "cap-http3/1.0" }, // Static name ref index 95
        H3Header { name: "x-custom-trace-id", value: "trace-99887766" }, // Literal name + value
    ];

    const block = encodeQpackHeaders(alloc, &req_headers)?;
    defer alloc.free(block);

    // Check 2-byte prefix (Required Insert Count = 0, Delta Base = 0)
    expect_eq(block[0], 0x00 as u8);
    expect_eq(block[1], 0x00 as u8);

    const decoded = decodeQpackHeaders(alloc, block)?;
    defer alloc.free(decoded);

    expect_eq(decoded.len, req_headers.len);
    let i: usize = 0;
    while (i < req_headers.len) {
        expect_eq(std.mem.eql(u8, decoded[i].name, req_headers[i].name), true);
        expect_eq(std.mem.eql(u8, decoded[i].value, req_headers[i].value), true);
        i += 1;
    }
}

fn runTestSettingsGuards(): !void {
    const alloc = std.testing.allocator;

    // 1. HTTP/2 reserved setting 0x02 (ENABLE_PUSH) & 0x03 (MAX_CONCURRENT_STREAMS) on encode
    const reserved_02: [1]H3Setting = [
        H3Setting { id: 0x02, value: 1 },
    ];
    let res_err_enc = false;
    * = encodeFrame(alloc, H3Frame { settings: &reserved_02 }) catch |e| {
        if (e == Http3Error.ReservedSetting) res_err_enc = true;
    };
    expect_eq(res_err_enc, true);

    // 2. HTTP/2 reserved setting 0x03 on wire decode: Type=0x04, Len=0x02, Id=0x03, Val=0x0a
    const wire_reserved: [4]u8 = [ 0x04, 0x02, 0x03, 0x0a ];
    let res_err_dec = false;
    * = decodeFrames(alloc, &wire_reserved) catch |e| {
        if (e == Http3Error.ReservedSetting) res_err_dec = true;
    };
    expect_eq(res_err_dec, true);

    // 3. Duplicate setting ID on encode & wire decode
    const dup_settings: [2]H3Setting = [
        H3Setting { id: MAX_FIELD_SECTION_SIZE, value: 1024 },
        H3Setting { id: MAX_FIELD_SECTION_SIZE, value: 2048 },
    ];
    let dup_err_enc = false;
    * = encodeFrame(alloc, H3Frame { settings: &dup_settings }) catch |e| {
        if (e == Http3Error.DuplicateSetting) dup_err_enc = true;
    };
    expect_eq(dup_err_enc, true);

    const wire_dup: [6]u8 = [ 0x04, 0x04, 0x06, 0x01, 0x06, 0x02 ];
    let dup_err_dec = false;
    * = decodeFrames(alloc, &wire_dup) catch |e| {
        if (e == Http3Error.DuplicateSetting) dup_err_dec = true;
    };
    expect_eq(dup_err_dec, true);
}

fn runTestFrameAndQpackErrorGuards(): !void {
    const alloc = std.testing.allocator;

    // 1. FrameTooLarge
    const large_frame = encodeFrame(alloc, H3Frame { data: "0123456789abcdef" })?;
    defer alloc.free(large_frame);
    let too_large = false;
    * = decodeFramesWithMaxSize(alloc, large_frame, 8) catch |e| {
        if (e == Http3Error.FrameTooLarge) too_large = true;
    };
    expect_eq(too_large, true);

    // 2. Truncated payload -> UnexpectedEof
    let trunc_err = false;
    * = decodeFrames(alloc, large_frame[0 .. large_frame.len - 3]) catch |e| {
        if (e == Http3Error.UnexpectedEof) trunc_err = true;
    };
    expect_eq(trunc_err, true);

    // 3. Illegal QPACK static index (>= 99):
    // Prefix: 0x00, 0x00; Indexed Field Line static index 100: 0xFF (63+0xC0) followed by 37 (0x25)
    const bad_qpack_idx: [4]u8 = [ 0x00, 0x00, 0xFF, 0x25 ];
    let qpack_oob = false;
    * = decodeQpackHeaders(alloc, &bad_qpack_idx) catch |e| {
        if (e == Http3Error.QpackDecompressionFailed) qpack_oob = true;
    };
    expect_eq(qpack_oob, true);

    // 4. Non-zero Required Insert Count -> QpackDecompressionFailed
    const bad_qpack_ric: [3]u8 = [ 0x01, 0x00, 0xD1 ];
    let qpack_ric_err = false;
    * = decodeQpackHeaders(alloc, &bad_qpack_ric) catch |e| {
        if (e == Http3Error.QpackDecompressionFailed) qpack_ric_err = true;
    };
    expect_eq(qpack_ric_err, true);
}

fn runTestDynamicCapMount(): !void {
    const alloc = std.testing.allocator;

    const push_wire = cap.http3.encodeFrame(alloc, cap.http3.H3Frame {
        push_promise: cap.http3.H3PushPromiseFrame {
            push_id: 42,
            header_block: "\x00\x00\xd1",
        },
    })?;
    defer alloc.free(push_wire);

    const frames = cap.http3.decodeFrames(alloc, push_wire)?;
    defer cap.http3.freeFrames(alloc, frames);

    expect_eq(frames.len, 1 as usize);
    switch (frames[0]) {
        .push_promise => |pp| {
            expect_eq(pp.push_id, 42 as u64);
            expect_eq(std.mem.eql(u8, pp.header_block, "\x00\x00\xd1"), true);
        },
        else => unreachable,
    }

    const hdrs: [2]cap.http3.H3Header = [
        cap.http3.H3Header { name: ":status", value: "200" },
        cap.http3.H3Header { name: "server", value: "cap-h3" },
    ];
    const encoded_hdrs = cap.http3.encodeQpackHeaders(alloc, &hdrs)?;
    defer alloc.free(encoded_hdrs);

    const decoded_hdrs = cap.http3.decodeQpackHeaders(alloc, encoded_hdrs)?;
    defer alloc.free(decoded_hdrs);
    expect_eq(decoded_hdrs.len, 2 as usize);
    expect_eq(std.mem.eql(u8, decoded_hdrs[0].name, ":status"), true);
    expect_eq(std.mem.eql(u8, decoded_hdrs[0].value, "200"), true);
    expect_eq(std.mem.eql(u8, decoded_hdrs[1].name, "server"), true);
    expect_eq(std.mem.eql(u8, decoded_hdrs[1].value, "cap-h3"), true);
}

test("1. HTTP/3 frames (SETTINGS/GOAWAY/MAX_PUSH_ID/CANCEL_PUSH/DATA/HEADERS) & GREASE unknown frame") {
    runTestFramesAndGrease() catch |e| {
        std.debug.print("frames test failed: {}\n", .{e});
        unreachable;
    };
}

test("2. RFC 9204 QPACK static table exact match, name ref & literal headers round-trip") {
    runTestQpackStaticAndLiterals() catch |e| {
        std.debug.print("qpack test failed: {}\n", .{e});
        unreachable;
    };
}

test("3. HTTP/2 reserved settings (0x02/0x03) & duplicate setting interception") {
    runTestSettingsGuards() catch |e| {
        std.debug.print("settings guard test failed: {}\n", .{e});
        unreachable;
    };
}

test("4. FrameTooLarge, UnexpectedEof truncation & QpackDecompressionFailed guards") {
    runTestFrameAndQpackErrorGuards() catch |e| {
        std.debug.print("error guard test failed: {}\n", .{e});
        unreachable;
    };
}

test("5. cap.http3 dynamic lazy-mount & std.testing.allocator zero-leak") {
    runTestDynamicCapMount() catch |e| {
        std.debug.print("cap.http3 test failed: {}\n", .{e});
        unreachable;
    };
}
