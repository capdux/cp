// Source: cp/cap/test/test_grpc_wire_production_deep.cp (Deep production tests for cap:grpc_wire)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    GrpcStatus,
    GrpcError,
    statusName,
    statusFromCode,
    parseServiceMethodPath,
    formatServiceMethodPath,
    parseGrpcTimeoutNs,
    formatGrpcTimeout,
    percentEncodeGrpcMessage,
    percentDecodeGrpcMessage,
    encodeBinHeader,
    decodeBinHeader,
    DEFAULT_MAX_MESSAGE_BYTES,
    encodeGrpcFrame,
    decodeGrpcFrame,
    GrpcStreamFramer,
} from "cap:net";

test("Length-Prefixed frame codec and GrpcStreamFramer arbitrary 1..7 byte chunking") {
    const alloc = std.testing.allocator;

    const f1 = try encodeGrpcFrame(alloc, false, "hello");
    defer alloc.free(f1);
    const f2 = try encodeGrpcFrame(alloc, true, "");
    defer alloc.free(f2);
    const f3 = try encodeGrpcFrame(alloc, false, "streaming-payload-0123456789abcdef");
    defer alloc.free(f3);

    // Single frame decode check
    let single = try decodeGrpcFrame(alloc, f1, DEFAULT_MAX_MESSAGE_BYTES);
    defer single.deinit(alloc);
    expect_eq(single.compressed, false);
    expect_eq(std.mem.eql(u8, single.payload, "hello"), true);

    // Concatenate 3 frames into one stream
    let stream = std.ArrayList(u8).empty;
    defer stream.deinit(alloc);
    try stream.appendSlice(alloc, f1);
    try stream.appendSlice(alloc, f2);
    try stream.appendSlice(alloc, f3);

    let framer = GrpcStreamFramer.init(DEFAULT_MAX_MESSAGE_BYTES);
    defer framer.deinit(alloc);

    // Feed arbitrary 1..7 byte chunks
    let pos: usize = 0;
    let step: usize = 1;
    while (pos < stream.items.len) {
        const end = @min(pos + step, stream.items.len);
        try framer.push(alloc, stream.items[pos..end]);
        pos = end;
        step = if (step >= 7) 1 else step + 1;
    }

    let out1 = (try framer.nextFrame(alloc)).?;
    defer out1.deinit(alloc);
    expect_eq(out1.compressed, false);
    expect_eq(std.mem.eql(u8, out1.payload, "hello"), true);

    let out2 = (try framer.nextFrame(alloc)).?;
    defer out2.deinit(alloc);
    expect_eq(out2.compressed, true);
    expect_eq(out2.payload.len, 0);

    let out3 = (try framer.nextFrame(alloc)).?;
    defer out3.deinit(alloc);
    expect_eq(out3.compressed, false);
    expect_eq(std.mem.eql(u8, out3.payload, "streaming-payload-0123456789abcdef"), true);

    const out4 = try framer.nextFrame(alloc);
    expect_eq(out4 == null, true);
    expect_eq(framer.bufferedLen(), 0);
}

test("grpc-timeout six units H/M/S/m/u/n and invalid/9-digit/overflow rejection") {
    const alloc = std.testing.allocator;

    expect_eq(try parseGrpcTimeoutNs("2H"), 2 * 3_600_000_000_000);
    expect_eq(try parseGrpcTimeoutNs("5M"), 5 * 60_000_000_000);
    expect_eq(try parseGrpcTimeoutNs("10S"), 10 * 1_000_000_000);
    expect_eq(try parseGrpcTimeoutNs("250m"), 250 * 1_000_000);
    expect_eq(try parseGrpcTimeoutNs("500u"), 500 * 1_000);
    expect_eq(try parseGrpcTimeoutNs("999n"), 999);
    expect_eq(try parseGrpcTimeoutNs("99999999n"), 99_999_999);

    const s1 = try formatGrpcTimeout(alloc, 500);
    defer alloc.free(s1);
    expect_eq(std.mem.eql(u8, s1, "500n"), true);

    const s2 = try formatGrpcTimeout(alloc, 150_000_000);
    defer alloc.free(s2);
    expect_eq(std.mem.eql(u8, s2, "150000u"), true);

    const s3 = try formatGrpcTimeout(alloc, 7_200_000_000_000);
    defer alloc.free(s3);
    expect_eq(std.mem.eql(u8, s3, "7200S"), true);

    // 9-digit rejected
    expect_eq(std.meta.isError(parseGrpcTimeoutNs("100000000n")), true);
    // Invalid unit rejected
    expect_eq(std.meta.isError(parseGrpcTimeoutNs("10X")), true);
    // Empty/non-digit rejected
    expect_eq(std.meta.isError(parseGrpcTimeoutNs("S")), true);
    expect_eq(std.meta.isError(parseGrpcTimeoutNs("12aS")), true);
    // Overflow u64 rejected (99999999 * 3_600_000_000_000 > u64 max)
    expect_eq(std.meta.isError(parseGrpcTimeoutNs("99999999H")), true);
}

test("GrpcStatus 0..16, parseServiceMethodPath, and percent-encoded grpc-message") {
    const alloc = std.testing.allocator;

    let code: u32 = 0;
    while (code <= 16) {
        const st = statusFromCode(code).?;
        expect_eq(@intFromEnum(st), code);
        expect_eq(statusName(st).len > 0, true);
        code += 1;
    }
    expect_eq(statusFromCode(17) == null, true);
    expect_eq(std.mem.eql(u8, statusName(GrpcStatus.UNAUTHENTICATED), "UNAUTHENTICATED"), true);

    const err_obj = GrpcError{
        .status = GrpcStatus.INVALID_ARGUMENT,
        .message = "bad request",
    };
    expect_eq(err_obj.status, GrpcStatus.INVALID_ARGUMENT);

    const parsed = try parseServiceMethodPath("/helloworld.Greeter/SayHello");
    expect_eq(std.mem.eql(u8, parsed.package_service, "helloworld.Greeter"), true);
    expect_eq(std.mem.eql(u8, parsed.method, "SayHello"), true);

    const formatted = try formatServiceMethodPath(alloc, "pkg.Svc", "DoWork");
    defer alloc.free(formatted);
    expect_eq(std.mem.eql(u8, formatted, "/pkg.Svc/DoWork"), true);

    // Invalid paths
    expect_eq(std.meta.isError(parseServiceMethodPath("pkg.Svc/Method")), true);
    expect_eq(std.meta.isError(parseServiceMethodPath("/pkg.Svc/Method/Extra")), true);
    expect_eq(std.meta.isError(parseServiceMethodPath("//Method")), true);
    expect_eq(std.meta.isError(parseServiceMethodPath("/pkg.Svc/")), true);

    // Percent-encode & decode with UTF-8 Chinese and '%'
    const raw_msg = "100% error: 参数非法\nline2";
    const enc = try percentEncodeGrpcMessage(alloc, raw_msg);
    defer alloc.free(enc);
    expect_eq(std.mem.indexOf(u8, enc, "%25") != null, true);
    expect_eq(std.mem.indexOf(u8, enc, "%0A") != null, true);

    const decoded_msg = try percentDecodeGrpcMessage(alloc, enc);
    defer alloc.free(decoded_msg);
    expect_eq(std.mem.eql(u8, decoded_msg, raw_msg), true);

    // Invalid percent encoding
    expect_eq(std.meta.isError(percentDecodeGrpcMessage(alloc, "bad%2")), true);
    expect_eq(std.meta.isError(percentDecodeGrpcMessage(alloc, "bad%ZZ")), true);
}

test("max_message_bytes guard, invalid compressed_flag > 1, and -bin base64 validation") {
    const alloc = std.testing.allocator;

    // Frame exceeding max_message_bytes
    const big_frame = try encodeGrpcFrame(alloc, false, "0123456789");
    defer alloc.free(big_frame);
    expect_eq(std.meta.isError(decodeGrpcFrame(alloc, big_frame, 5)), true);

    let framer = GrpcStreamFramer.init(5);
    defer framer.deinit(alloc);
    try framer.push(alloc, big_frame);
    expect_eq(std.meta.isError(framer.nextFrame(alloc)), true);

    // Invalid compressed flag (> 1)
    let bad_flag_frame = try encodeGrpcFrame(alloc, false, "abc");
    defer alloc.free(bad_flag_frame);
    bad_flag_frame[0] = 2;
    expect_eq(std.meta.isError(decodeGrpcFrame(alloc, bad_flag_frame, 64)), true);

    // -bin Base64 encode/decode (both padded and unpadded)
    const raw_bin = "\x00\xFF\x10\xAB\xCD";
    const b64 = try encodeBinHeader(alloc, raw_bin);
    defer alloc.free(b64);
    const decoded_bin = try decodeBinHeader(alloc, b64);
    defer alloc.free(decoded_bin);
    expect_eq(std.mem.eql(u8, decoded_bin, raw_bin), true);

    // Unpadded base64 ("AQID" -> {1, 2, 3}, "AQI" -> {1, 2})
    const decoded_unpadded = try decodeBinHeader(alloc, "AQI");
    defer alloc.free(decoded_unpadded);
    expect_eq(decoded_unpadded.len, 2);
    expect_eq(decoded_unpadded[0], 1);
    expect_eq(decoded_unpadded[1], 2);

    // Malformed base64
    expect_eq(std.meta.isError(decodeBinHeader(alloc, "!!!invalid!!!")), true);
    expect_eq(std.meta.isError(decodeBinHeader(alloc, "A")), true);
}

test("cap.grpc_wire dynamic mount and zero-leak verification") {
    const alloc = std.testing.allocator;

    const frame_bytes = try cap.net.encodeGrpcFrame(alloc, false, "mounted-ok");
    defer alloc.free(frame_bytes);

    let frame = try cap.net.decodeGrpcFrame(alloc, frame_bytes, cap.net.DEFAULT_MAX_MESSAGE_BYTES);
    defer frame.deinit(alloc);
    expect_eq(std.mem.eql(u8, frame.payload, "mounted-ok"), true);

    const ns = try cap.net.parseGrpcTimeoutNs("42m");
    expect_eq(ns, 42_000_000);
}
