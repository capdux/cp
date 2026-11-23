// Reference-hardened test suite against hyperium/tonic (MIT) & gRPC over HTTP/2 Spec
// Covers:
//   1. 5-byte LPM framing: single frame, coalesced multi-frame, 1-byte incremental stream reassembly, compressed flag >= 2 & > 4MiB rejection
//   2. grpc-timeout strict parsing (all 6 units H/M/S/m/u/n, >8 digits / leading space / sign / bad unit rejection) & formatting
//   3. All 17 GrpcStatus bidirectional mappings, -bin key detection, strict Base64 bin headers & percent-encoded grpc-message
//   4. 16-lane @Vector(16, u8) SIMD payload copy, ASCII metadata precheck & percent-encode scan 0-diff parity vs scalar
//   5. Deterministic PRNG Fuzz (>= 1,600 malformed gRPC LPM frames / timeout strings / metadata & random SIMD parity)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    GrpcStatus,
    statusName,
    statusFromCode,
    statusFromName,
    parseServiceMethodPath,
    formatServiceMethodPath,
} from "../../lib/net/grpcStatus.cp";
import {
    isBinHeaderKey,
    simdGrpcIsValidAsciiMetadata,
    simdGrpcIsValidAsciiMetadataScalar,
    simdGrpcNeedsPercentEncode,
    simdGrpcNeedsPercentEncodeScalar,
    parseGrpcTimeoutNs,
    formatGrpcTimeout,
    percentEncodeGrpcMessage,
    percentDecodeGrpcMessage,
    encodeBinHeader,
    decodeBinHeader,
} from "../../lib/net/grpcHeaders.cp";
import {
    GRPC_DEFAULT_MAX_DECODING_MESSAGE_SIZE,
    simdGrpcCopyPayload,
    simdGrpcCopyPayloadScalar,
    encodeGrpcFrame,
    decodeGrpcFrame,
    GrpcStreamFramer,
} from "../../lib/net/grpcFrame.cp";

fn nextRng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn decodeFrameFails(alloc: std.mem.Allocator, raw: []const u8, max_b: usize): bool {
    const f = decodeGrpcFrame(alloc, raw, max_b) catch return true;
    f.deinit(alloc);
    return false;
}

fn parseTimeoutFails(raw: []const u8): bool {
    * = parseGrpcTimeoutNs(raw) catch return true;
    return false;
}

fn percentDecodeFails(alloc: std.mem.Allocator, raw: []const u8): bool {
    const d = percentDecodeGrpcMessage(alloc, raw) catch return true;
    alloc.free(d);
    return false;
}

fn binDecodeFails(alloc: std.mem.Allocator, raw: []const u8): bool {
    const d = decodeBinHeader(alloc, raw) catch return true;
    alloc.free(d);
    return false;
}

fn fuzzFramerOne(framer: *GrpcStreamFramer, alloc: std.mem.Allocator, raw: []const u8) {
    framer.reset();
    framer.push(alloc, raw) catch return;
    const maybe_f = framer.nextFrame(alloc) catch return;
    if (f := maybe_f) {
        f.deinit(alloc);
    }
}

fn fuzzDecodeOneFrame(alloc: std.mem.Allocator, raw: []const u8) {
    const f = decodeGrpcFrame(alloc, raw, 4096) catch return;
    f.deinit(alloc);
}

fn main() {
    test("1. 5-byte LPM framing, coalesced/byte-by-byte stream & flag/size limits", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(GRPC_DEFAULT_MAX_DECODING_MESSAGE_SIZE, (4 * 1024 * 1024) as usize);

        const p1 = "hello_grpc_payload_over_16_bytes_0123456789";
        const p2 = "second_frame";
        const f1 = encodeGrpcFrame(alloc, false, p1) catch unreachable;
        defer alloc.free(f1);
        const f2 = encodeGrpcFrame(alloc, true, p2) catch unreachable;
        defer alloc.free(f2);

        const dec1 = decodeGrpcFrame(alloc, f1, GRPC_DEFAULT_MAX_DECODING_MESSAGE_SIZE) catch unreachable;
        defer dec1.deinit(alloc);
        expect_eq(dec1.compressed, false);
        expect_eq(dec1.payload, p1);

        // Coalesced + 1-byte incremental streaming via GrpcStreamFramer
        let framer = GrpcStreamFramer.init(GRPC_DEFAULT_MAX_DECODING_MESSAGE_SIZE);
        defer framer.deinit(alloc);

        let i: usize = 0;
        while (i < f1.len) : (i += 1) {
            framer.push(alloc, f1[i .. i + 1]) catch unreachable;
        }
        framer.push(alloc, f2) catch unreachable;

        const out1 = (framer.nextFrame(alloc) catch unreachable).?;
        defer out1.deinit(alloc);
        expect_eq(out1.compressed, false);
        expect_eq(out1.payload, p1);

        const out2 = (framer.nextFrame(alloc) catch unreachable).?;
        defer out2.deinit(alloc);
        expect_eq(out2.compressed, true);
        expect_eq(out2.payload, p2);
        expect_eq(framer.bufferedLen(), 0 as usize);

        // Invalid compressed flag >= 2 rejected
        expect_eq(decodeFrameFails(alloc, "\x02\x00\x00\x00\x00", 1024), true);
        expect_eq(decodeFrameFails(alloc, "\xff\x00\x00\x00\x00", 1024), true);
        // Oversized frame > 4MiB (0x00400001) rejected
        expect_eq(decodeFrameFails(alloc, "\x00\x00\x40\x00\x01", GRPC_DEFAULT_MAX_DECODING_MESSAGE_SIZE), true);
        // Truncated header / payload rejected
        expect_eq(decodeFrameFails(alloc, "\x00\x00\x00", 1024), true);
        expect_eq(decodeFrameFails(alloc, "\x00\x00\x00\x00\x05hi", 1024), true);
    });

    test("2. grpc-timeout strict parsing (all 6 units) & overflow/malformed rejection", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        expect_eq(parseGrpcTimeoutNs("1H") catch unreachable, 3_600_000_000_000 as u64);
        expect_eq(parseGrpcTimeoutNs("2M") catch unreachable, 120_000_000_000 as u64);
        expect_eq(parseGrpcTimeoutNs("5S") catch unreachable, 5_000_000_000 as u64);
        expect_eq(parseGrpcTimeoutNs("250m") catch unreachable, 250_000_000 as u64);
        expect_eq(parseGrpcTimeoutNs("500u") catch unreachable, 500_000 as u64);
        expect_eq(parseGrpcTimeoutNs("99999999n") catch unreachable, 99_999_999 as u64);

        const fmt_ms = formatGrpcTimeout(alloc, 250_000_000) catch unreachable;
        defer alloc.free(fmt_ms);
        expect_eq(fmt_ms, "250000u");

        // Reject: >8 digits, leading space, sign, unknown unit, empty
        expect_eq(parseTimeoutFails("123456789S"), true);
        expect_eq(parseTimeoutFails(" 5S"), true);
        expect_eq(parseTimeoutFails("-5S"), true);
        expect_eq(parseTimeoutFails("+5S"), true);
        expect_eq(parseTimeoutFails("10X"), true);
        expect_eq(parseTimeoutFails("S"), true);
        expect_eq(parseTimeoutFails(""), true);
    });

    test("3. All 17 GrpcStatus bidirectional mappings, -bin metadata & percent-encoded grpc-message", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // All 17 status codes 0..=16 roundtrip
        let c: u32 = 0;
        while (c <= 16) : (c += 1) {
            const st = statusFromCode(c).?;
            const nm = statusName(st);
            expect_eq(statusFromName(nm).? == st, true);
        }
        expect_eq(statusFromCode(17) == null, true);
        expect_eq(statusFromName("NO_SUCH_STATUS") == null, true);

        // Service/method path parsing & formatting
        const sp = parseServiceMethodPath("/helloworld.Greeter/SayHello") catch unreachable;
        expect_eq(sp.package_service, "helloworld.Greeter");
        expect_eq(sp.method, "SayHello");
        const fp = formatServiceMethodPath(alloc, "helloworld.Greeter", "SayHello") catch unreachable;
        defer alloc.free(fp);
        expect_eq(fp, "/helloworld.Greeter/SayHello");

        // -bin key detection & Base64 roundtrip + invalid base64 rejection
        expect_eq(isBinHeaderKey("trace-bin"), true);
        expect_eq(isBinHeaderKey("-bin"), false);
        expect_eq(isBinHeaderKey("trace-id"), false);

        const raw_bin = "\x00\x01\xfe\xffgrpc";
        const b64 = encodeBinHeader(alloc, raw_bin) catch unreachable;
        defer alloc.free(b64);
        const dec_bin = decodeBinHeader(alloc, b64) catch unreachable;
        defer alloc.free(dec_bin);
        expect_eq(dec_bin, raw_bin);
        expect_eq(binDecodeFails(alloc, "!!!not_base64!!!"), true);

        // grpc-message percent encoding & decoding + invalid %ZZ rejection
        const msg = "bad input: 100% failed\n中文";
        const enc_msg = percentEncodeGrpcMessage(alloc, msg) catch unreachable;
        defer alloc.free(enc_msg);
        const dec_msg = percentDecodeGrpcMessage(alloc, enc_msg) catch unreachable;
        defer alloc.free(dec_msg);
        expect_eq(dec_msg, msg);

        // Safe ASCII fast-path (>16 bytes)
        const ascii_msg = "service temporarily unavailable 0123456789";
        const enc_ascii = percentEncodeGrpcMessage(alloc, ascii_msg) catch unreachable;
        defer alloc.free(enc_ascii);
        expect_eq(enc_ascii, ascii_msg);

        expect_eq(percentDecodeFails(alloc, "bad%ZZescape"), true);
        expect_eq(percentDecodeFails(alloc, "truncated%2"), true);
    });

    test("4. 16-lane SIMD payload copy & metadata precheck 0-diff parity vs scalar", () => {
        const valid_hdr = "grpc-encoding-gzip-deflate-identity-0123456789";
        const bad_hdr = "grpc-encoding-gzip\r\ninjected-header";
        const pct_hdr = "error-message-100%-failure-0123456789";

        expect_eq(simdGrpcIsValidAsciiMetadata(valid_hdr), true);
        expect_eq(simdGrpcIsValidAsciiMetadata(valid_hdr), simdGrpcIsValidAsciiMetadataScalar(valid_hdr));
        expect_eq(simdGrpcIsValidAsciiMetadata(bad_hdr), false);
        expect_eq(simdGrpcIsValidAsciiMetadata(bad_hdr), simdGrpcIsValidAsciiMetadataScalar(bad_hdr));

        expect_eq(simdGrpcNeedsPercentEncode(valid_hdr), false);
        expect_eq(simdGrpcNeedsPercentEncode(valid_hdr), simdGrpcNeedsPercentEncodeScalar(valid_hdr));
        expect_eq(simdGrpcNeedsPercentEncode(pct_hdr), true);
        expect_eq(simdGrpcNeedsPercentEncode(pct_hdr), simdGrpcNeedsPercentEncodeScalar(pct_hdr));

        let src: [80]u8 = undefined;
        let d_simd: [80]u8 = undefined;
        let d_scal: [80]u8 = undefined;
        let k: usize = 0;
        while (k < 80) : (k += 1) {
            src[k] = ((k * 29 + 5) & 0xFF) as u8;
        }
        let len: usize = 0;
        while (len <= 80) : (len += 1) {
            simdGrpcCopyPayload(d_simd[0..len], src[0..len]);
            simdGrpcCopyPayloadScalar(d_scal[0..len], src[0..len]);
            expect_eq(d_simd[0..len], d_scal[0..len]);
        }
    });

    test("5. Deterministic PRNG Fuzz (>= 1,600 malformed gRPC LPM/timeout/metadata & SIMD parity)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let seed: u64 = 0x5EED_689C_2026_0034;
        let buf: [80]u8 = undefined;
        let d1: [80]u8 = undefined;
        let d2: [80]u8 = undefined;

        // 5A. 1,000 malformed gRPC LPM frames, timeout strings, percent/base64 metadata fuzz
        let framer = GrpcStreamFramer.init(1024);
        defer framer.deinit(alloc);

        let iter: usize = 0;
        while (iter < 1000) : (iter += 1) {
            const flen: usize = ((nextRng(&seed) % 72) + 1) as usize;
            let k: usize = 0;
            while (k < flen) : (k += 1) {
                buf[k] = (nextRng(&seed) & 0xFF) as u8;
            }
            const sample = buf[0..flen];
            fuzzDecodeOneFrame(alloc, sample);
            * = parseGrpcTimeoutNs(sample) catch {};
            * = percentDecodeFails(alloc, sample);
            * = binDecodeFails(alloc, sample);

            fuzzFramerOne(&framer, alloc, sample);
        }

        // 5B. 600 random byte-slice SIMD payload copy & metadata precheck parity checks
        let r_iter: usize = 0;
        while (r_iter < 600) : (r_iter += 1) {
            const rlen: usize = (nextRng(&seed) % 72) as usize;
            const ascii_mode = (r_iter & 1) == 0;
            let j: usize = 0;
            while (j < rlen) : (j += 1) {
                buf[j] = if (ascii_mode) ((0x20 + (nextRng(&seed) % 95)) as u8) else ((nextRng(&seed) & 0xFF) as u8);
            }
            const slice = buf[0..rlen];
            expect_eq(simdGrpcIsValidAsciiMetadata(slice), simdGrpcIsValidAsciiMetadataScalar(slice));
            expect_eq(simdGrpcNeedsPercentEncode(slice), simdGrpcNeedsPercentEncodeScalar(slice));
            simdGrpcCopyPayload(d1[0..rlen], slice);
            simdGrpcCopyPayloadScalar(d2[0..rlen], slice);
            expect_eq(d1[0..rlen], d2[0..rlen]);
        }
    });
}
