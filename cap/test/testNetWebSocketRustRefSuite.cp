// Reference-hardened test suite against snapview/tungstenite-rs (MIT/Apache-2.0) & RFC 6455 / Autobahn
// Covers:
//   1. RFC 6455 Section 1.3 computeAcceptKey vector & 125 / 126 / 65536 payload length boundaries
//   2. RFC 6455 / Autobahn strict header & close guards (RSV, reserved opcodes, fragmented control, non-minimal len, MSB=1, close codes, UTF-8)
//   3. WsStreamAssembler fragmentation, legal interleaved Ping/Pong control frames, illegal mid-fragment data & max_payload guard
//   4. 16-lane @Vector(16, u8) SIMD applyMaskSimd & applyMaskOffsetSimd 0-diff parity vs scalar (0..=257B + all 0..3 phase offsets)
//   5. Deterministic PRNG Fuzz (>= 1,600 malformed WebSocket frames & incremental byte-stream fuzz)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    WsOpcode,
    WsCloseCode,
    WsError,
    computeAcceptKey,
} from "../../lib/net/wsTypes.cp";
import {
    applyMaskSimd,
    applyMaskScalar,
    applyMaskOffsetSimd,
    applyMaskOffsetScalar,
    encodeFrame,
    encodeCloseFrame,
    decodeFrame,
} from "../../lib/net/wsCodec.cp";
import {
    WsMessage,
    WsStreamAssembler,
} from "../../lib/net/wsAssembler.cp";

fn nextRng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn decodeExpectErr(alloc: std.mem.Allocator, raw: []const u8, req_mask: bool, max_p: usize, expected: WsError): bool {
    const res = decodeFrame(alloc, raw, req_mask, max_p) catch (e) {
        return e == expected;
    };
    let f = res.frame;
    f.deinit(alloc);
    return false;
}

fn feedExpectErr(asmb: *WsStreamAssembler, raw: []const u8, expected: WsError): bool {
    asmb.feed(raw) catch (e) {
        return e == expected;
    };
    return false;
}

fn fuzzDecodeOne(alloc: std.mem.Allocator, raw: []const u8, req_mask: bool) {
    const res = decodeFrame(alloc, raw, req_mask, 4096) catch return;
    let f = res.frame;
    f.deinit(alloc);
}

fn main() {
    test("1. RFC 6455 Section 1.3 handshake key & 125 / 126 / 65536 length boundaries", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // RFC 6455 §1.3 official vector
        const accept = computeAcceptKey("dGhlIHNhbXBsZSBub25jZQ==");
        expect_eq(accept[0..28], "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");

        const mkey: [4]u8 = [0x37, 0xFA, 0x21, 0x3D];

        // 125-byte 7-bit length boundary
        let p125: [125]u8 = [0x41; 125];
        const f125 = encodeFrame(alloc, .Binary, p125[0..125], true, mkey) catch unreachable;
        defer alloc.free(f125);
        expect_eq(f125.len, (2 + 4 + 125) as usize);
        const d125 = decodeFrame(alloc, f125, true, 1024) catch unreachable;
        let fr125 = d125.frame;
        defer fr125.deinit(alloc);
        expect_eq(fr125.payload.len, 125 as usize);
        expect_eq(fr125.payload, p125[0..125]);

        // 126-byte 16-bit extended length boundary
        let p126: [126]u8 = [0x42; 126];
        const f126 = encodeFrame(alloc, .Binary, p126[0..126], true, mkey) catch unreachable;
        defer alloc.free(f126);
        expect_eq(f126.len, (4 + 4 + 126) as usize);
        const d126 = decodeFrame(alloc, f126, true, 1024) catch unreachable;
        let fr126 = d126.frame;
        defer fr126.deinit(alloc);
        expect_eq(fr126.payload.len, 126 as usize);
        expect_eq(fr126.payload, p126[0..126]);

        // 65536-byte 64-bit extended length boundary
        const p65536: []u8 = alloc.alloc(u8, 65536) catch unreachable;
        defer alloc.free(p65536);
        @memset(p65536, 0x5A);
        const f65536 = encodeFrame(alloc, .Binary, p65536, true, mkey) catch unreachable;
        defer alloc.free(f65536);
        expect_eq(f65536.len, (10 + 4 + 65536) as usize);
        const d65536 = decodeFrame(alloc, f65536, true, 131072) catch unreachable;
        let fr65536 = d65536.frame;
        defer fr65536.deinit(alloc);
        expect_eq(fr65536.payload.len, 65536 as usize);
        expect_eq(fr65536.payload[0], 0x5A as u8);
        expect_eq(fr65536.payload[65535], 0x5A as u8);
    });

    test("2. RFC 6455 / Autobahn strict header, non-minimal length, close code & UTF-8 guards", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // RSV1 / RSV2 / RSV3 non-zero -> ReservedBitsSet
        expect_eq(decodeExpectErr(alloc, "\xc1\x00", false, 1024, WsError.ReservedBitsSet), true);
        expect_eq(decodeExpectErr(alloc, "\xa1\x00", false, 1024, WsError.ReservedBitsSet), true);
        expect_eq(decodeExpectErr(alloc, "\x91\x00", false, 1024, WsError.ReservedBitsSet), true);

        // Reserved opcodes 3..7 and 11..15 -> InvalidOpcode
        expect_eq(decodeExpectErr(alloc, "\x83\x00", false, 1024, WsError.InvalidOpcode), true);
        expect_eq(decodeExpectErr(alloc, "\x8b\x00", false, 1024, WsError.InvalidOpcode), true);

        // Fragmented control frame (FIN=0, Ping) -> ControlFrameFragmented
        expect_eq(decodeExpectErr(alloc, "\x09\x00", false, 1024, WsError.ControlFrameFragmented), true);
        // Control frame > 125 bytes (len7=126, Ping) -> ControlFrameTooLarge
        expect_eq(decodeExpectErr(alloc, "\x89\x7e\x00\x7e", false, 1024, WsError.ControlFrameTooLarge), true);

        // Non-minimal length encoding: len7=126 with len16=125 -> NonMinimalLength
        expect_eq(decodeExpectErr(alloc, "\x82\x7e\x00\x7d", false, 1024, WsError.NonMinimalLength), true);
        // Non-minimal length encoding: len7=127 with len64=65535 -> NonMinimalLength
        expect_eq(decodeExpectErr(alloc, "\x82\x7f\x00\x00\x00\x00\x00\x00\xff\xff", false, 131072, WsError.NonMinimalLength), true);
        // 64-bit length MSB=1 -> PayloadTooLarge
        expect_eq(decodeExpectErr(alloc, "\x82\x7f\x80\x00\x00\x00\x00\x00\x00\x00", false, 131072, WsError.PayloadTooLarge), true);

        // Unmasked client frame when require_masked=true -> UnmaskedClientFrame
        expect_eq(decodeExpectErr(alloc, "\x81\x02hi", true, 1024, WsError.UnmaskedClientFrame), true);

        // Close frame payload len == 1 -> InvalidClosePayload
        expect_eq(decodeExpectErr(alloc, "\x88\x01\x03", false, 1024, WsError.InvalidClosePayload), true);

        // Close status code wire legality
        expect_eq(WsCloseCode.isValidWireCode(1000), true);
        expect_eq(WsCloseCode.isValidWireCode(1012), true);
        expect_eq(WsCloseCode.isValidWireCode(3000), true);
        expect_eq(WsCloseCode.isValidWireCode(4999), true);
        expect_eq(WsCloseCode.isValidWireCode(999), false);
        expect_eq(WsCloseCode.isValidWireCode(1004), false);
        expect_eq(WsCloseCode.isValidWireCode(1005), false);
        expect_eq(WsCloseCode.isValidWireCode(1006), false);
        expect_eq(WsCloseCode.isValidWireCode(1015), false);
        expect_eq(WsCloseCode.isValidWireCode(2999), false);
        expect_eq(WsCloseCode.isValidWireCode(5000), false);

        // Illegal wire close code 1005 (0x03ED) & invalid UTF-8 close reason
        expect_eq(decodeExpectErr(alloc, "\x88\x02\x03\xed", false, 1024, WsError.InvalidClosePayload), true);
        expect_eq(decodeExpectErr(alloc, "\x88\x04\x03\xe8\xff\xfe", false, 1024, WsError.InvalidUtf8), true);
        // Invalid UTF-8 in unfragmented Text frame
        expect_eq(decodeExpectErr(alloc, "\x81\x02\xff\xfe", false, 1024, WsError.InvalidUtf8), true);

        // Valid Close frame roundtrip
        const cf = encodeCloseFrame(alloc, 1000, "goodbye", null) catch unreachable;
        defer alloc.free(cf);
        const dc = decodeFrame(alloc, cf, false, 1024) catch unreachable;
        let frc = dc.frame;
        defer frc.deinit(alloc);
        expect_eq(frc.opcode == .Close, true);
        expect_eq(frc.payload.len, 9 as usize);
    });

    test("3. WsStreamAssembler fragmentation, interleaved Ping/Pong & protocol violation guards", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let asm_ok = WsStreamAssembler.init(alloc, false, 1024);
        defer asm_ok.deinit();

        // Fragment 1 (Text, FIN=0): first half of multi-byte UTF-8 "你" (E4 BD)
        const f1 = encodeFrame(alloc, .Text, "\xe4\xbd", false, null) catch unreachable;
        defer alloc.free(f1);
        // Interleaved Ping (FIN=1) mid-fragment — legal per RFC 6455 §5.4!
        const fp = encodeFrame(alloc, .Ping, "hb", true, null) catch unreachable;
        defer alloc.free(fp);
        // Fragment 2 (Continuation, FIN=1): remaining byte (A0) + "好" (E5 A5 BD)
        const f2 = encodeFrame(alloc, .Continuation, "\xa0\xe5\xa5\xbd", true, null) catch unreachable;
        defer alloc.free(f2);

        asm_ok.feed(f1) catch unreachable;
        asm_ok.feed(fp) catch unreachable;
        asm_ok.feed(f2) catch unreachable;

        expect_eq(asm_ok.pendingCount(), 2 as usize);
        let m1 = asm_ok.nextMessage().?;
        defer m1.deinit(alloc);
        expect_eq(m1.opcode == .Ping, true);
        expect_eq(m1.payload, "hb");

        let m2 = asm_ok.nextMessage().?;
        defer m2.deinit(alloc);
        expect_eq(m2.opcode == .Text, true);
        expect_eq(m2.payload, "你好");

        // Orphan Continuation frame -> UnexpectedContinuation
        const orphan = encodeFrame(alloc, .Continuation, "x", true, null) catch unreachable;
        defer alloc.free(orphan);
        expect_eq(feedExpectErr(&asm_ok, orphan, WsError.UnexpectedContinuation), true);

        // New data frame while fragment in progress -> FragmentedInProgress
        asm_ok.reset();
        asm_ok.feed(f1) catch unreachable;
        const bad_mid = encodeFrame(alloc, .Binary, "z", true, null) catch unreachable;
        defer alloc.free(bad_mid);
        expect_eq(feedExpectErr(&asm_ok, bad_mid, WsError.FragmentedInProgress), true);
    });

    test("4. 16-lane SIMD applyMaskSimd & applyMaskOffsetSimd 0-diff parity vs scalar (0..=257B)", () => {
        const key: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
        let b_simd: [260]u8 = undefined;
        let b_scal: [260]u8 = undefined;

        let len: usize = 0;
        while (len <= 257) : (len += 1) {
            let phase: usize = 0;
            while (phase < 4) : (phase += 1) {
                let i: usize = 0;
                while (i < len) : (i += 1) {
                    const v = ((i * 31 + phase * 7) & 0xFF) as u8;
                    b_simd[i] = v;
                    b_scal[i] = v;
                }
                if (phase == 0) {
                    applyMaskSimd(b_simd[0..len], key);
                    applyMaskScalar(b_scal[0..len], key);
                } else {
                    applyMaskOffsetSimd(b_simd[0..len], key, phase);
                    applyMaskOffsetScalar(b_scal[0..len], key, phase);
                }
                expect_eq(b_simd[0..len], b_scal[0..len]);
            }
        }
    });

    test("5. Deterministic PRNG Fuzz (>= 1,600 malformed WS frames, stream chunks & SIMD mask parity)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let seed: u64 = 0x5EED_6455_2026_0034;
        let buf: [96]u8 = undefined;
        let buf2: [96]u8 = undefined;

        // 5A. 1,000 malformed raw frame & incremental stream assembler fuzz iterations
        let asmb = WsStreamAssembler.init(alloc, false, 1024);
        defer asmb.deinit();

        let iter: usize = 0;
        while (iter < 1000) : (iter += 1) {
            const flen: usize = ((nextRng(&seed) % 80) + 1) as usize;
            let k: usize = 0;
            while (k < flen) : (k += 1) {
                buf[k] = (nextRng(&seed) & 0xFF) as u8;
            }
            const sample = buf[0..flen];
            fuzzDecodeOne(alloc, sample, (iter & 1) == 0);

            asmb.reset();
            asmb.feed(sample) catch {};
            while (msg_val := asmb.nextMessage()) {
                let m: WsMessage = msg_val;
                m.deinit(alloc);
            }
        }

        // 5B. 600 random key + length + offset SIMD masking parity checks
        let r_iter: usize = 0;
        while (r_iter < 600) : (r_iter += 1) {
            const mlen: usize = (nextRng(&seed) % 96) as usize;
            const key: [4]u8 = [
                (nextRng(&seed) & 0xFF) as u8,
                (nextRng(&seed) & 0xFF) as u8,
                (nextRng(&seed) & 0xFF) as u8,
                (nextRng(&seed) & 0xFF) as u8,
            ];
            const off: usize = (nextRng(&seed) % 16) as usize;
            let j: usize = 0;
            while (j < mlen) : (j += 1) {
                const v = (nextRng(&seed) & 0xFF) as u8;
                buf[j] = v;
                buf2[j] = v;
            }
            applyMaskOffsetSimd(buf[0..mlen], key, off);
            applyMaskOffsetScalar(buf2[0..mlen], key, off);
            expect_eq(buf[0..mlen], buf2[0..mlen]);
        }
    });
}
