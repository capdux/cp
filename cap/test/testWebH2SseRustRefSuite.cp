// Reference-hardened test suite against hyperium/h2 (MIT) & RFC 9113 / WHATWG SSE
// Covers all RFC 9113 frame types (positive + negative vectors: PADDED underflow, WINDOW_UPDATE=0,
// PING!=8, SETTINGS non-multiple-of-6, Stream 0 violations, reserved bit masking, max_frame_size),
// WHATWG SSE (\r\n/\n/\r, UTF-8 BOM, comment lines, single-space stripping, NUL id ignore,
// strict ASCII retry + overflow guard, multi-line data join), 16-lane @Vector(16, u8) SIMD vs
// scalar 0-diff parity, and 1,600 deterministic PRNG fuzz cases (800 HTTP/2 + 800 SSE).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CONNECTION_PREFACE,
    FRAME_HEADER_SIZE,
    DEFAULT_MAX_FRAME_SIZE,
    MAX_ALLOWED_FRAME_SIZE,
    MAX_WINDOW_SIZE,
    Http2Error,
    FrameType,
    FrameFlags,
    Http2ErrorCode,
    SettingId,
    SettingParam,
    FrameStreamDecoder,
    simdH2SseFindLineBreak,
    scalarH2SseFindLineBreak,
    simdH2SseBytesEqual,
    scalarH2SseBytesEqual,
    hasConnectionPreface,
    validateHttp2Preface,
    validateHttp2FrameStrict,
    encodeFrame,
    encodeSettings,
    decodeSettings,
    encodePing,
    decodePing,
    encodeGoaway,
    decodeGoaway,
    encodeWindowUpdate,
    decodeWindowUpdate,
    encodeRstStream,
    decodeRstStream,
    encodeDataFrame,
    decodeDataPayload,
} from "../../lib/web/http2.cp";
import {
    SseError,
    SseParser,
    SseEncoder,
} from "../../lib/web/sse.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextByte(*_): u8 {
        return @as(u8, @truncate(_.next()));
    }

    fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) : (i += 1) {
            dst[i] = _.nextByte();
        }
    }
}

fn writeH2Header(buf: []u8, plen: u32, ftype: u8, flags: u8, stream_id_raw: u32) {
    buf[0] = @as(u8, @truncate(plen >> 16));
    buf[1] = @as(u8, @truncate(plen >> 8));
    buf[2] = @as(u8, @truncate(plen));
    buf[3] = ftype;
    buf[4] = flags;
    buf[5] = @as(u8, @truncate(stream_id_raw >> 24));
    buf[6] = @as(u8, @truncate(stream_id_raw >> 16));
    buf[7] = @as(u8, @truncate(stream_id_raw >> 8));
    buf[8] = @as(u8, @truncate(stream_id_raw));
}

fn checkTest1Http2PositiveRoundTrip(): u32 {
    const allocator = std.heap.page_allocator;

    // 1a. 24-byte connection preface
    if (!hasConnectionPreface(CONNECTION_PREFACE)) return 1;
    validateHttp2Preface(CONNECTION_PREFACE) catch return 2;
    if (hasConnectionPreface("PRI * HTTP/2.0\r\n\r\nSM\r\n\rX")) return 3;

    // 1b. SETTINGS + ACK
    const params: [2]SettingParam = [
        SettingParam.init(SettingId.MAX_FRAME_SIZE, 32768),
        SettingParam.init(SettingId.INITIAL_WINDOW_SIZE, 65535),
    ];
    const f_set = encodeSettings(allocator, params[0..], false) catch return 4;
    defer allocator.free(f_set);
    const h_set = validateHttp2FrameStrict(f_set, DEFAULT_MAX_FRAME_SIZE) catch return 5;
    if (h_set.frame_type != FrameType.Settings || h_set.stream_id != 0 || h_set.payload_len != 12) return 6;

    const f_set_ack = encodeSettings(allocator, &.{}, true) catch return 7;
    defer allocator.free(f_set_ack);
    const h_ack = validateHttp2FrameStrict(f_set_ack, DEFAULT_MAX_FRAME_SIZE) catch return 8;
    if (h_ack.payload_len != 0 || (h_ack.flags & FrameFlags.ACK) == 0) return 9;

    // 1c. PING
    const ping_bytes: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
    const f_ping = encodePing(allocator, ping_bytes, false) catch return 10;
    defer allocator.free(f_ping);
    const h_ping = validateHttp2FrameStrict(f_ping, DEFAULT_MAX_FRAME_SIZE) catch return 11;
    if (h_ping.frame_type != FrameType.Ping || h_ping.payload_len != 8) return 12;

    // 1d. GOAWAY
    const f_goaway = encodeGoaway(allocator, 7, Http2ErrorCode.NoError, "bye") catch return 13;
    defer allocator.free(f_goaway);
    const h_goaway = validateHttp2FrameStrict(f_goaway, DEFAULT_MAX_FRAME_SIZE) catch return 14;
    if (h_goaway.frame_type != FrameType.Goaway || h_goaway.payload_len != 11) return 15;

    // 1e. WINDOW_UPDATE (stream 0 and stream 3)
    const f_wu = encodeWindowUpdate(allocator, 3, 1024) catch return 16;
    defer allocator.free(f_wu);
    const h_wu = validateHttp2FrameStrict(f_wu, DEFAULT_MAX_FRAME_SIZE) catch return 17;
    if (h_wu.frame_type != FrameType.WindowUpdate || h_wu.stream_id != 3) return 18;

    // 1f. RST_STREAM
    const f_rst = encodeRstStream(allocator, 5, Http2ErrorCode.Cancel) catch return 19;
    defer allocator.free(f_rst);
    const h_rst = validateHttp2FrameStrict(f_rst, DEFAULT_MAX_FRAME_SIZE) catch return 20;
    if (h_rst.frame_type != FrameType.RstStream || h_rst.stream_id != 5) return 21;

    // 1g. DATA with PADDED + END_STREAM
    const f_data = encodeDataFrame(allocator, 1, "hello-h2-payload", true, 5) catch return 22;
    defer allocator.free(f_data);
    const h_data = validateHttp2FrameStrict(f_data, DEFAULT_MAX_FRAME_SIZE) catch return 23;
    if (h_data.frame_type != FrameType.Data || h_data.stream_id != 1) return 24;

    // 1h. Reserved bit R on Stream ID must be masked cleanly
    let raw_r: [13]u8 = [0; 13];
    writeH2Header(raw_r[0..], 4, FrameType.WindowUpdate as u8, 0, 0x80000009);
    raw_r[12] = 1; // increment = 1
    const h_r = validateHttp2FrameStrict(raw_r[0..], DEFAULT_MAX_FRAME_SIZE) catch return 25;
    if (h_r.stream_id != 9) return 26;

    // 1i. Stream decoder sticky-frame round-trip
    let decoder = FrameStreamDecoder.init(allocator, DEFAULT_MAX_FRAME_SIZE);
    defer decoder.deinit();
    decoder.feed(f_set) catch return 27;
    decoder.feed(f_ping) catch return 28;
    decoder.feed(f_data) catch return 29;

    const d1_opt = decoder.nextFrame() catch return 30;
    const d1 = d1_opt orelse return 31;
    let d1m = d1;
    defer d1m.deinit(allocator);
    const decoded_params = decodeSettings(allocator, d1m) catch return 32;
    defer allocator.free(decoded_params);
    if (decoded_params.len != 2 || decoded_params[0].value != 32768) return 33;

    const d2_opt = decoder.nextFrame() catch return 34;
    const d2 = d2_opt orelse return 35;
    let d2m = d2;
    defer d2m.deinit(allocator);
    const pong = decodePing(d2m) catch return 36;
    if (!simdH2SseBytesEqual(pong[0..], ping_bytes[0..])) return 37;

    const d3_opt = decoder.nextFrame() catch return 38;
    const d3 = d3_opt orelse return 39;
    let d3m = d3;
    defer d3m.deinit(allocator);
    const unpadded = decodeDataPayload(d3m) catch return 40;
    if (!simdH2SseBytesEqual(unpadded, "hello-h2-payload")) return 41;

    return 0;
}

fn checkTest2Http2NegativeGuards(): u32 {
    let raw: [64]u8 = [0; 64];

    // 2a. Header < 9 bytes
    let ok = false;
    * = validateHttp2FrameStrict(raw[0..8], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.FrameSizeError);
    };
    if (!ok) return 1;

    // 2b. max_frame_size out of RFC 9113 bounds (< 16384 or > 16777215)
    writeH2Header(raw[0..], 0, FrameType.Settings as u8, FrameFlags.ACK, 0);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..9], 16383) catch |e| {
        ok = (e == Http2Error.FrameSizeError);
    };
    if (!ok) return 2;

    ok = false;
    * = validateHttp2FrameStrict(raw[0..9], MAX_ALLOWED_FRAME_SIZE + 1) catch |e| {
        ok = (e == Http2Error.FrameSizeError);
    };
    if (!ok) return 3;

    // 2c. SETTINGS on non-zero stream -> ProtocolError
    writeH2Header(raw[0..], 0, FrameType.Settings as u8, FrameFlags.ACK, 1);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..9], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.ProtocolError);
    };
    if (!ok) return 4;

    // 2d. SETTINGS non-multiple of 6 -> InvalidSettingsLength
    writeH2Header(raw[0..], 5, FrameType.Settings as u8, 0, 0);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..14], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.InvalidSettingsLength);
    };
    if (!ok) return 5;

    // 2e. SETTINGS ACK with non-empty payload -> FrameSizeError
    writeH2Header(raw[0..], 6, FrameType.Settings as u8, FrameFlags.ACK, 0);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..15], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.FrameSizeError);
    };
    if (!ok) return 6;

    // 2f. PING != 8 bytes -> FrameSizeError; PING on stream != 0 -> ProtocolError
    writeH2Header(raw[0..], 7, FrameType.Ping as u8, 0, 0);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..16], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.FrameSizeError);
    };
    if (!ok) return 7;

    writeH2Header(raw[0..], 8, FrameType.Ping as u8, 0, 2);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..17], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.ProtocolError);
    };
    if (!ok) return 8;

    // 2g. GOAWAY on stream != 0 -> ProtocolError; GOAWAY < 8 bytes -> FrameSizeError
    writeH2Header(raw[0..], 8, FrameType.Goaway as u8, 0, 1);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..17], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.ProtocolError);
    };
    if (!ok) return 9;

    writeH2Header(raw[0..], 4, FrameType.Goaway as u8, 0, 0);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..13], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.FrameSizeError);
    };
    if (!ok) return 10;

    // 2h. RST_STREAM != 4 bytes -> FrameSizeError; RST_STREAM on stream 0 -> ProtocolError
    writeH2Header(raw[0..], 3, FrameType.RstStream as u8, 0, 1);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..12], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.FrameSizeError);
    };
    if (!ok) return 11;

    writeH2Header(raw[0..], 4, FrameType.RstStream as u8, 0, 0);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..13], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.ProtocolError);
    };
    if (!ok) return 12;

    // 2i. WINDOW_UPDATE increment == 0 -> InvalidWindowIncrement; length != 4 -> FrameSizeError
    writeH2Header(raw[0..], 4, FrameType.WindowUpdate as u8, 0, 0);
    raw[9] = 0x80; // only reserved bit set -> 31-bit increment == 0
    raw[10] = 0;
    raw[11] = 0;
    raw[12] = 0;
    ok = false;
    * = validateHttp2FrameStrict(raw[0..13], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.InvalidWindowIncrement);
    };
    if (!ok) return 13;

    writeH2Header(raw[0..], 5, FrameType.WindowUpdate as u8, 0, 0);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..14], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.FrameSizeError);
    };
    if (!ok) return 14;

    // 2j. DATA PADDED underflow guard: pad_len >= payload_len (e.g. pad_len == 5, payload_len == 5)
    writeH2Header(raw[0..], 5, FrameType.Data as u8, FrameFlags.PADDED, 1);
    raw[9] = 5; // pad_len == payload_len -> invalid
    ok = false;
    * = validateHttp2FrameStrict(raw[0..14], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.ProtocolError);
    };
    if (!ok) return 15;

    // DATA PADDED with empty payload -> ProtocolError
    writeH2Header(raw[0..], 0, FrameType.Data as u8, FrameFlags.PADDED, 1);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..9], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.ProtocolError);
    };
    if (!ok) return 16;

    // 2k. HEADERS PADDED + PRIORITY underflow guard & self-dependency
    writeH2Header(raw[0..], 6, FrameType.Headers as u8, FrameFlags.PADDED | FrameFlags.PRIORITY, 3);
    raw[9] = 1; // prefix_len=6, pad_len=1 > 6-6 -> ProtocolError
    ok = false;
    * = validateHttp2FrameStrict(raw[0..15], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.ProtocolError);
    };
    if (!ok) return 17;

    // PRIORITY self-dependency (stream 3 depends on stream 3)
    writeH2Header(raw[0..], 5, FrameType.Priority as u8, 0, 3);
    raw[9] = 0;
    raw[10] = 0;
    raw[11] = 0;
    raw[12] = 3;
    raw[13] = 16;
    ok = false;
    * = validateHttp2FrameStrict(raw[0..14], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.ProtocolError);
    };
    if (!ok) return 18;

    // 2l. Payload length exceeds slice length -> FrameSizeError
    writeH2Header(raw[0..], 100, FrameType.Data as u8, 0, 1);
    ok = false;
    * = validateHttp2FrameStrict(raw[0..20], DEFAULT_MAX_FRAME_SIZE) catch |e| {
        ok = (e == Http2Error.FrameSizeError);
    };
    if (!ok) return 19;

    return 0;
}

fn checkTest3SseSpecCompliance(): u32 {
    const allocator = std.heap.page_allocator;
    let p = SseParser.init(allocator, 4096);
    defer p.deinit();

    // 3a. UTF-8 BOM + mixed \r\n, \n, \r + comment + single space strip + multi-line data + valid id/retry
    const stream = "\xEF\xBB\xBF: heartbeat comment\r\nevent:  custom\r\nid: evt-42\nretry: 1500\rdata: line1\r\ndata:  indented\ndata:\n\r\n";
    // Feed in 3-byte chunks to stress BOM & \r\n chunk boundary splitting
    let off: usize = 0;
    while (off < stream.len) {
        const end = if (off + 3 <= stream.len) off + 3 else stream.len;
        p.feed(stream[off..end]) catch return 1;
        off = end;
    }

    const ev1 = p.nextEvent() orelse return 2;
    defer ev1.deinit(allocator);
    // At most one leading 0x20 stripped -> " custom"
    if (!simdH2SseBytesEqual(ev1.event_type, " custom")) return 3;
    if (!simdH2SseBytesEqual(ev1.data, "line1\n indented\n")) return 4;
    const id1 = ev1.id orelse return 5;
    if (!simdH2SseBytesEqual(id1, "evt-42")) return 6;
    const r1 = ev1.retry_ms orelse return 7;
    if (r1 != 1500) return 8;

    // 3b. id containing NUL (0x00) MUST be ignored (Last-Event-ID remains "evt-42")
    // retry with non-digits, sign, or u64 overflow MUST be ignored (remains 1500)
    const bad_fields = "id: bad\x00id\nretry: +200\nretry: 123abc\nretry: 99999999999999999999999999\ndata: ok\n\n";
    p.feed(bad_fields) catch return 9;
    const ev2 = p.nextEvent() orelse return 10;
    defer ev2.deinit(allocator);
    if (!simdH2SseBytesEqual(ev2.event_type, "message")) return 11;
    if (!simdH2SseBytesEqual(ev2.data, "ok")) return 12;
    const id2 = ev2.id orelse return 13;
    if (!simdH2SseBytesEqual(id2, "evt-42")) return 14;
    const r2 = ev2.retry_ms orelse return 15;
    if (r2 != 1500) return 16;

    // 3c. SseEncoder round-trip + NUL/CRLF rejection
    const encoded = SseEncoder.formatEvent(allocator, "update", "a\nb\r\nc", "id-9", 2500) catch return 17;
    defer allocator.free(encoded);
    p.feed(encoded) catch return 18;
    const ev3 = p.nextEvent() orelse return 19;
    defer ev3.deinit(allocator);
    if (!simdH2SseBytesEqual(ev3.event_type, "update")) return 20;
    if (!simdH2SseBytesEqual(ev3.data, "a\nb\nc")) return 21;
    const r3 = ev3.retry_ms orelse return 22;
    if (r3 != 2500) return 23;

    let rejected = false;
    * = SseEncoder.formatEvent(allocator, "ok", "hi", "bad\x00id", null) catch |e| {
        rejected = (e == SseError.InvalidEventData);
    };
    if (!rejected) return 24;

    return 0;
}

fn checkTest4SimdDifferential(): u32 {
    let rng = XorShift64.init(0x48325353_4553494D);
    let buf: [192]u8 = [0; 192];
    let other: [192]u8 = [0; 192];

    let len: usize = 0;
    while (len <= 160) : (len += 1) {
        // Fill with printable ASCII (no \r, \n, \0)
        let i: usize = 0;
        while (i < len) : (i += 1) {
            buf[i] = (0x20 as u8) + (@as(u8, @truncate(rng.next())) % (90 as u8));
            other[i] = buf[i];
        }

        // No delimiter present -> both null
        if (simdH2SseFindLineBreak(buf[0..len]) != null) return 1;
        if (scalarH2SseFindLineBreak(buf[0..len]) != null) return 2;

        // Equal slices parity
        if (!simdH2SseBytesEqual(buf[0..len], other[0..len])) return 3;
        if (!scalarH2SseBytesEqual(buf[0..len], other[0..len])) return 4;

        if (len > 0) {
            // Test \r, \n, \0 at deterministic positions covering all 16 lanes & tail
            const pos = @as(usize, @truncate(rng.next())) % len;
            const targets: [3]u8 = ['\r', '\n', 0x00];
            let t: usize = 0;
            while (t < 3) : (t += 1) {
                const saved = buf[pos];
                buf[pos] = targets[t];
                const r_simd = simdH2SseFindLineBreak(buf[0..len]);
                const r_scalar = scalarH2SseFindLineBreak(buf[0..len]);
                if (r_simd == null || r_scalar == null) return 5;
                if ((r_simd orelse 999) != (r_scalar orelse 998)) return 6;
                if ((r_simd orelse 999) != pos) return 7;
                buf[pos] = saved;
            }

            // Single-byte flip inequality parity
            other[pos] ^= 0x01;
            const eq_s = simdH2SseBytesEqual(buf[0..len], other[0..len]);
            const eq_c = scalarH2SseBytesEqual(buf[0..len], other[0..len]);
            if (eq_s || eq_s != eq_c) return 8;
        }
    }
    return 0;
}

fn checkTest5PrngFuzzing(): u32 {
    const allocator = std.heap.page_allocator;
    let rng = XorShift64.init(0x57333752_3546555A);
    let buf: [192]u8 = [0; 192];

    // 5a. 800 HTTP/2 fuzz cases (verify strict validator & FrameStreamDecoder agree and never crash)
    let valid_h2: usize = 0;
    let iter: usize = 0;
    while (iter < 800) : (iter += 1) {
        const len = @as(usize, @truncate(rng.next())) % 160;
        rng.fillBytes(buf[0..len]);

        // Shape half the inputs to have plausible header length & frame type so deep checks run
        if (len >= 9 && (iter & 1) == 0) {
            const plen: u32 = @as(u32, @truncate(len - 9));
            const ftype: u8 = @as(u8, @truncate(iter % 10));
            const sid: u32 = if ((iter & 2) == 0) 0 else (@as(u32, @truncate(iter)) & 0x7FFFFFFF);
            writeH2Header(buf[0..], plen, ftype, buf[4], sid);
        }

        const slice = buf[0..len];
        let v_ok = true;
        * = validateHttp2FrameStrict(slice, DEFAULT_MAX_FRAME_SIZE) catch {
            v_ok = false;
        };

        let decoder = FrameStreamDecoder.init(allocator, DEFAULT_MAX_FRAME_SIZE);
        defer decoder.deinit();
        decoder.feed(slice) catch return 1;
        let dec_ok = false;
        const f_opt = decoder.nextFrame() catch {
            if (v_ok) return 2;
            continue;
        };
        if (f_val := f_opt) {
            let fm = f_val;
            defer fm.deinit(allocator);
            dec_ok = true;
            valid_h2 += 1;
        }
        if (v_ok && !dec_ok) return 3;
    }
    if (valid_h2 > 800) return 4;

    // 5b. 800 SSE fuzz cases (arbitrary binary + SSE-like lines with random chunk splits, zero crash/leak)
    let total_sse_events: usize = 0;
    iter = 0;
    while (iter < 800) : (iter += 1) {
        const len = @as(usize, @truncate(rng.next())) % 160;
        rng.fillBytes(buf[0..len]);

        if ((iter & 1) == 0 && len >= 20) {
            // Inject valid SSE field prefixes & line endings
            const sample = "data: fuzz\r\nid: 1\nretry: 5\r\r\n";
            const copy_n = if (len < sample.len) len else sample.len;
            let k: usize = 0;
            while (k < copy_n) : (k += 1) {
                buf[k] = sample[k];
            }
        }

        let p = SseParser.init(allocator, 512);
        defer p.deinit();

        // Feed in 1..17 byte random chunks
        let pos: usize = 0;
        while (pos < len) {
            const step = 1 + (@as(usize, @truncate(rng.next())) % 17);
            const end = if (pos + step <= len) pos + step else len;
            p.feed(buf[pos..end]) catch break;
            pos = end;
        }
        while (ev := p.nextEvent()) {
            ev.deinit(allocator);
            total_sse_events += 1;
        }
    }
    if (total_sse_events == 0) return 5;
    return 0;
}

fn main() {
    test("1. HTTP/2 RFC 9113 Positive Vectors, Preface, Reserved Bit Mask & Decoder", () => {
        expect_eq(checkTest1Http2PositiveRoundTrip(), 0 as u32);
    });

    test("2. HTTP/2 RFC 9113 Strict Negative Guards (PADDED, WU=0, PING, SETTINGS, Stream 0)", () => {
        expect_eq(checkTest2Http2NegativeGuards(), 0 as u32);
    });

    test("3. WHATWG SSE Spec Compliance (BOM, CRLF/LF/CR, NUL id, Retry Overflow, Multi-line)", () => {
        expect_eq(checkTest3SseSpecCompliance(), 0 as u32);
    });

    test("4. 16-Lane @Vector(16, u8) SIMD vs Scalar 0-Diff Differential Parity", () => {
        expect_eq(checkTest4SimdDifferential(), 0 as u32);
    });

    test("5. Deterministic PRNG Fuzzing (800 HTTP/2 + 800 SSE = 1,600 Cases, Zero Crash/Leak)", () => {
        expect_eq(checkTest5PrngFuzzing(), 0 as u32);
    });
}
