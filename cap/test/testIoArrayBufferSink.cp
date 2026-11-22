// Origin: Pure-CP Stdlib Test Suite (lib/io/arrayBufferSink.cp) | Created: 2026-11-22
// Production Deep Verification Suite for `Bun.ArrayBufferSink` Zero-Alloc Incremental Writer:
//   1. Incremental write / writeByte / peek / end() & post-end SinkEnded rejection + start() reuse
//   2. Streaming mode (`stream: true`): periodic flush() drains & resets buffer; non-stream flush preserves buffer
//   3. highWaterMark backpressure threshold (`shouldFlush`) & clamping
//   4. writeLatin1 ISO-8859-1 -> UTF-8 transcoding (ASCII + 0x80..0xFF 2-byte sequences)
//   5. writeUtf16 UTF-16 -> UTF-8 transcoding (BMP 1/2/3-byte, surrogate pair 4-byte emoji, lone surrogate -> U+FFFD)
//   6. BufferOverflow rollback atomicity & OutputTooSmall protection

import { test, expect_eq, expect_true } from "cap:test";
import {
    ARRAY_BUFFER_SINK_CAP,
    ArrayBufferSinkOptions,
    ArrayBufferSink,
} from "../../lib/io/arrayBufferSink.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Test 1: Incremental writes, end() terminal guard & start() reset reuse
// ============================================================================
test("arrayBufferSink: incremental write, end() terminal guard & start() reuse", () => {
    let sink = ArrayBufferSink.init();
    expect_eq(sink.bufferedLen(), 0);
    expect_eq(sink.capacity(), ARRAY_BUFFER_SINK_CAP);
    expect_true(!sink.isEnded());

    const w1 = sink.write("hel") catch 0;
    const w2 = sink.writeByte('l') catch 0;
    const w3 = sink.write("o") catch 0;
    expect_eq(w1, 3);
    expect_eq(w2, 1);
    expect_eq(w3, 1);
    expect_true(strEq(sink.peek(), "hello"));

    let out: [32]u8 = [0; 32];
    const n = sink.end(out[0..]) catch 0;
    expect_eq(n, 5);
    expect_true(strEq(out[0..n], "hello"));
    expect_true(sink.isEnded());
    expect_eq(sink.bufferedLen(), 0);

    // Subsequent write after end() must fail with SinkEnded
    let ended_err = false;
    * = sink.write("!") catch brk: {
        ended_err = true;
        break :brk 0;
    };
    expect_true(ended_err);

    // start() resets state and allows reuse
    sink.start(ArrayBufferSinkOptions {});
    expect_true(!sink.isEnded());
    const w4 = sink.write("again") catch 0;
    expect_eq(w4, 5);
    expect_true(strEq(sink.peek(), "again"));
});

// ============================================================================
// Test 2: Streaming flush (`stream: true`) vs non-streaming flush
// ============================================================================
test("arrayBufferSink: streaming flush drains chunk while non-stream flush preserves buffer", () => {
    let sink = ArrayBufferSink.init();
    sink.start(ArrayBufferSinkOptions { stream: true, as_uint8array: true, high_water_mark: 16 });
    expect_true(sink.isStreaming());

    * = sink.write("hel") catch 0;
    let chunk: [32]u8 = [0; 32];
    if (r1 := sink.flush(chunk[0..]) catch null) {
        expect_true(r1.is_stream_chunk);
        expect_true(r1.as_uint8array);
        expect_eq(r1.bytes_flushed, 3);
        expect_true(strEq(chunk[0..3], "hel"));
    } else {
        expect_true(false);
    }
    expect_eq(sink.bufferedLen(), 0);

    * = sink.write("lo") catch 0;
    if (r2 := sink.flush(chunk[0..]) catch null) {
        expect_eq(r2.bytes_flushed, 2);
        expect_true(strEq(chunk[0..2], "lo"));
    } else {
        expect_true(false);
    }

    // Non-streaming mode: flush reports buffered count without draining
    sink.start(ArrayBufferSinkOptions { stream: false });
    * = sink.write("keep") catch 0;
    if (r3 := sink.flush(chunk[0..]) catch null) {
        expect_true(!r3.is_stream_chunk);
        expect_eq(r3.bytes_flushed, 4);
    } else {
        expect_true(false);
    }
    expect_eq(sink.bufferedLen(), 4);
});

// ============================================================================
// Test 3: highWaterMark backpressure threshold & clamping
// ============================================================================
test("arrayBufferSink: highWaterMark backpressure trigger and clamping", () => {
    let sink = ArrayBufferSink.init();
    sink.start(ArrayBufferSinkOptions { high_water_mark: 8, stream: true });
    expect_eq(sink.highWaterMark(), 8);
    expect_true(!sink.shouldFlush());

    * = sink.write("1234567") catch 0;
    expect_true(!sink.shouldFlush());

    * = sink.writeByte('8') catch 0;
    expect_true(sink.shouldFlush());

    // Oversized highWaterMark clamps to ARRAY_BUFFER_SINK_CAP
    sink.start(ArrayBufferSinkOptions { high_water_mark: 999999 });
    expect_eq(sink.highWaterMark(), ARRAY_BUFFER_SINK_CAP);
});

// ============================================================================
// Test 4: writeLatin1 (ISO-8859-1 -> UTF-8)
// ============================================================================
test("arrayBufferSink: writeLatin1 transcodes ASCII and high-byte Latin-1 to UTF-8", () => {
    let sink = ArrayBufferSink.init();
    // "caf" + 0xE9 ('é' -> UTF-8 C3 A9)
    const latin1: [4]u8 = ['c', 'a', 'f', 0xE9];
    const written = sink.writeLatin1(latin1[0..]) catch 0;
    expect_eq(written, 5);

    const p = sink.peek();
    expect_eq(p.len, 5);
    expect_eq(p[0], 'c');
    expect_eq(p[1], 'a');
    expect_eq(p[2], 'f');
    expect_eq(p[3], 0xC3);
    expect_eq(p[4], 0xA9);
});

// ============================================================================
// Test 5: writeUtf16 (BMP, surrogate pair emoji, lone surrogate -> U+FFFD)
// ============================================================================
test("arrayBufferSink: writeUtf16 handles BMP, surrogate pairs, and lone surrogates", () => {
    let sink = ArrayBufferSink.init();
    // 'A' (0x0041), '中' (0x4E2D -> E4 B8 AD), U+1F600 (D83D DE00 -> F0 9F 98 80), lone high 0xD800 (-> EF BF BD)
    const units: [5]u16 = [0x0041, 0x4E2D, 0xD83D, 0xDE00, 0xD800];
    const written = sink.writeUtf16(units[0..]) catch 0;
    expect_eq(written, 1 + 3 + 4 + 3);

    const p = sink.peek();
    expect_eq(p[0], 'A');
    expect_eq(p[1], 0xE4);
    expect_eq(p[2], 0xB8);
    expect_eq(p[3], 0xAD);
    expect_eq(p[4], 0xF0);
    expect_eq(p[5], 0x9F);
    expect_eq(p[6], 0x98);
    expect_eq(p[7], 0x80);
    expect_eq(p[8], 0xEF);
    expect_eq(p[9], 0xBF);
    expect_eq(p[10], 0xBD);
});

// ============================================================================
// Test 6: BufferOverflow rollback & OutputTooSmall safety guards
// ============================================================================
test("arrayBufferSink: BufferOverflow atomicity and OutputTooSmall guard", () => {
    let sink = ArrayBufferSink.init();
    sink.start(ArrayBufferSinkOptions { stream: true });

    let block: [4096]u8 = ['x'; 4096];
    * = sink.write(block[0..]) catch 0;
    * = sink.write(block[0..]) catch 0;
    expect_eq(sink.remainingCap(), 0);

    // Next write must fail with BufferOverflow without corrupting len
    let overflowed = false;
    * = sink.write("z") catch brk: {
        overflowed = true;
        break :brk 0;
    };
    expect_true(overflowed);
    expect_eq(sink.bufferedLen(), ARRAY_BUFFER_SINK_CAP);

    // Tiny output buffer on flush/end fails with OutputTooSmall and preserves buffered data
    let tiny: [4]u8 = [0; 4];
    let flush_too_small = false;
    * = sink.flush(tiny[0..]) catch brk: {
        flush_too_small = true;
        break :brk null;
    };
    expect_true(flush_too_small);
    expect_eq(sink.bufferedLen(), ARRAY_BUFFER_SINK_CAP);
});
