// Origin: Pure-CP Stdlib Test Suite (lib/sys/byte_ring.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Power-of-Two Masked Batch Ring Byte & Frame Queue:
//   1. Cross-wraparound two-segment batch write & read to 100% full capacity
//   2. Atomic whole-block write (`writeAllAtomic`) preventing partial writes & tracking dropped_writes
//   3. Non-destructive `peekBytes`, zero-copy `skipBytes` across wraparound & `clear()`
//   4. 4-byte LE length-prefixed framing (`writeFrame` / `readFrame`) with wraparound & small-buffer retry
//   5. High-watermark, `fillRatioBps` & multi-cycle wraparound integrity stress
//   6. Non-power-of-two init, empty slice, empty ring & oversized frame defensive guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ByteRingStats,
    ByteRingQueue,
} from "cap:sys";

// ============================================================================
// Test 1: Cross-Wraparound Two-Segment Batch Write & Read
// ============================================================================

fn testWraparoundBatchReadWrite()! {
    let storage: [16]u8 = [0; 16];
    let ring = cap.sys.ByteRingQueue.init(storage[0..]) ?? return error.InitFailed;

    expect_eq(ring.isEmpty(), true);
    expect_eq(ring.available(), 16);

    // Write 12 bytes, then read 10 bytes -> head=10, tail=12, len=2
    const chunk1: []const u8 = "0123456789AB";
    expect_eq(ring.writeBytes(chunk1), 12);
    let tmp10: [10]u8 = [0; 10];
    expect_eq(ring.readBytes(tmp10[0..]), 10);
    expect_eq(ring.len(), 2);
    expect_eq(ring.available(), 14);

    // Write 14 bytes (wraps across index 16 -> 0..10), filling ring to 100% (16/16)
    const chunk2: []const u8 = "CDEFGHIJKLMNOP";
    expect_eq(ring.writeBytes(chunk2), 14);
    expect_eq(ring.len(), 16);
    expect_eq(ring.isFull(), true);
    expect_eq(ring.fillRatioBps(), 10000);

    // Extra write on full ring returns 0
    expect_eq(ring.writeBytes("X"), 0);

    // Read all 16 bytes -> must equal "ABCDEFGHIJKLMNOP"
    let out16: [16]u8 = [0; 16];
    expect_eq(ring.readBytes(out16[0..]), 16);
    const expected: []const u8 = "ABCDEFGHIJKLMNOP";
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        expect_eq(out16[i], expected[i]);
    }
    expect_eq(ring.isEmpty(), true);
}

// ============================================================================
// Test 2: Atomic Write (`writeAllAtomic`) Prevents Partial Writes
// ============================================================================

fn testWriteAllAtomic()! {
    let storage: [8]u8 = [0; 8];
    let ring = ByteRingQueue.init(storage[0..]) ?? return error.InitFailed;

    expect_eq(ring.writeAllAtomic("12345"), true);
    expect_eq(ring.available(), 3);

    // 4-byte atomic write fails with zero partial write
    expect_eq(ring.writeAllAtomic("6789"), false);
    expect_eq(ring.len(), 5);
    expect_eq(ring.stats().dropped_writes, 1);

    // 3-byte atomic write succeeds exactly
    expect_eq(ring.writeAllAtomic("678"), true);
    expect_eq(ring.isFull(), true);

    let out8: [8]u8 = [0; 8];
    expect_eq(ring.readBytes(out8[0..]), 8);
    expect_eq(out8[0], '1' as u8);
    expect_eq(out8[7], '8' as u8);
}

// ============================================================================
// Test 3: Non-Destructive `peekBytes`, `skipBytes` & `clear`
// ============================================================================

fn testPeekSkipAndClear()! {
    let storage: [8]u8 = [0; 8];
    let ring = ByteRingQueue.init(storage[0..]) ?? return error.InitFailed;

    // Advance cursors to force wraparound during peek/skip
    * = ring.writeBytes("123456");
    expect_eq(ring.skipBytes(5), 5); // head=5, tail=6 (1 byte '6' left)
    * = ring.writeBytes("ABCDEF");   // wraps around! unread = "6ABCDEF" (7 bytes)

    let p4: [4]u8 = [0; 4];
    expect_eq(ring.peekBytes(p4[0..]), 4);
    expect_eq(p4[0], '6' as u8);
    expect_eq(p4[1], 'A' as u8);
    expect_eq(p4[2], 'B' as u8);
    expect_eq(p4[3], 'C' as u8);
    expect_eq(ring.len(), 7); // peek did NOT advance head!

    expect_eq(ring.skipBytes(2), 2); // skips '6', 'A'
    let rem: [8]u8 = [0; 8];
    expect_eq(ring.readBytes(rem[0..]), 5);
    expect_eq(rem[0], 'B' as u8);
    expect_eq(rem[4], 'F' as u8);

    * = ring.writeBytes("XYZ");
    ring.clear();
    expect_eq(ring.isEmpty(), true);
    expect_eq(ring.len(), 0);
}

// ============================================================================
// Test 4: Length-Prefixed Framing (`writeFrame` / `readFrame`)
// ============================================================================

fn testLengthPrefixedFraming()! {
    let storage: [32]u8 = [0; 32];
    let ring = ByteRingQueue.init(storage[0..]) ?? return error.InitFailed;

    // Enqueue 3 frames including a 0-byte empty frame
    expect_eq(ring.writeFrame("hello"), true); // 4 + 5 = 9B
    expect_eq(ring.writeFrame(""), true);      // 4 + 0 = 4B
    expect_eq(ring.writeFrame("world!"), true); // 4 + 6 = 10B (total = 23B)

    // Oversized frame that doesn't fit in remaining 9B is rejected atomically
    expect_eq(ring.writeFrame("too-long"), false); // needs 12B > 9B
    expect_eq(ring.stats().dropped_writes, 1);

    // readFrame with undersized output buffer returns null WITHOUT consuming frame 1
    let tiny: [2]u8 = [0; 2];
    expect_eq(ring.readFrame(tiny[0..]) == null, true);
    expect_eq(ring.len(), 23);

    // Retry with adequate buffer -> reads exact frame 1 ("hello")
    let out: [16]u8 = [0; 16];
    const n1 = ring.readFrame(out[0..]) ?? return error.ReadFrameFailed;
    expect_eq(n1, 5);
    expect_eq(out[0], 'h' as u8);
    expect_eq(out[4], 'o' as u8);

    // Frame 2: empty 0-byte frame
    const n2 = ring.readFrame(out[0..]) ?? return error.ReadFrameFailed;
    expect_eq(n2, 0);

    // Write another frame that wraps around the 32-byte boundary before reading frame 3
    expect_eq(ring.writeFrame("wrap-around-msg"), true); // 4 + 15 = 19B (wraps!)

    const n3 = ring.readFrame(out[0..]) ?? return error.ReadFrameFailed;
    expect_eq(n3, 6);
    expect_eq(out[0], 'w' as u8);
    expect_eq(out[5], '!' as u8);

    const n4 = ring.readFrame(out[0..]) ?? return error.ReadFrameFailed;
    expect_eq(n4, 15);
    expect_eq(out[0], 'w' as u8);
    expect_eq(out[14], 'g' as u8);
    expect_eq(ring.isEmpty(), true);
}

// ============================================================================
// Test 5: High-Watermark, `fillRatioBps` & Multi-Cycle Wraparound Integrity
// ============================================================================

fn testHighWatermarkAndMultiCycleStress()! {
    let storage: [16]u8 = [0; 16];
    let ring = ByteRingQueue.init(storage[0..]) ?? return error.InitFailed;

    expect_eq(ring.fillRatioBps(), 0);
    * = ring.writeBytes("12345678");
    expect_eq(ring.fillRatioBps(), 5000); // 8 / 16 = 50.00%
    * = ring.skipBytes(8);

    // 40 cycles of writing & reading 7-byte frames (280 bytes across a 16-byte ring)
    let cycle: usize = 0;
    let frame_out: [12]u8 = [0; 12];
    while (cycle < 40) : (cycle += 1) {
        let payload: [7]u8 = [
            (cycle & 0xFF) as u8,
            ((cycle + 1) & 0xFF) as u8,
            ((cycle + 2) & 0xFF) as u8,
            ((cycle + 3) & 0xFF) as u8,
            ((cycle + 4) & 0xFF) as u8,
            ((cycle + 5) & 0xFF) as u8,
            ((cycle + 6) & 0xFF) as u8,
        ];
        expect_eq(ring.writeFrame(payload[0..]), true);
        const n = ring.readFrame(frame_out[0..]) ?? return error.StressReadFailed;
        expect_eq(n, 7);
        expect_eq(frame_out[0], payload[0]);
        expect_eq(frame_out[6], payload[6]);
    }

    const st = ring.stats();
    expect_eq(st.capacity, 16);
    expect_eq(st.used, 0);
    expect_eq(st.free, 16);
    expect_eq(st.high_watermark, 11); // 4-byte header + 7-byte payload
    expect_eq(st.total_written, 8 + 40 * 11);
    expect_eq(st.total_read, 8 + 40 * 11);
}

// ============================================================================
// Test 6: Defensive Boundary Guards
// ============================================================================

fn testDefensiveGuards()! {
    let b0: [1]u8 = [0; 1];
    let b3: [3]u8 = [0; 3];
    let b6: [6]u8 = [0; 6];
    let b10: [10]u8 = [0; 10];

    // Non-power-of-two or < 2 capacities rejected
    expect_eq(ByteRingQueue.init(b0[0..0]) == null, true);
    expect_eq(ByteRingQueue.init(b0[0..]) == null, true);
    expect_eq(ByteRingQueue.init(b3[0..]) == null, true);
    expect_eq(ByteRingQueue.init(b6[0..]) == null, true);
    expect_eq(ByteRingQueue.init(b10[0..]) == null, true);

    // Minimum valid power-of-two capacity = 2
    let b2: [2]u8 = [0; 2];
    let ring2 = ByteRingQueue.init(b2[0..]) ?? return error.InitFailed;

    // Empty slices & empty ring operations are safe no-ops
    let out: [4]u8 = [0; 4];
    expect_eq(ring2.writeBytes(b0[0..0]), 0);
    expect_eq(ring2.readBytes(out[0..0]), 0);
    expect_eq(ring2.peekBytes(out[0..0]), 0);
    expect_eq(ring2.readBytes(out[0..]), 0);
    expect_eq(ring2.skipBytes(10), 0);
    expect_eq(ring2.readFrame(out[0..]) == null, true);

    // Even a 0-byte frame needs 4 bytes > capacity 2 -> cleanly rejected
    expect_eq(ring2.writeFrame(""), false);
    expect_eq(ring2.stats().dropped_writes, 1);
}

// ============================================================================
// Test Runner Registration
// ============================================================================

test("sys/byte_ring: cross-wraparound two-segment batch read/write", testWraparoundBatchReadWrite);
test("sys/byte_ring: writeAllAtomic prevents partial writes & tracks drops", testWriteAllAtomic);
test("sys/byte_ring: non-destructive peekBytes, skipBytes & clear", testPeekSkipAndClear);
test("sys/byte_ring: 4B LE length-prefixed framing & small-buffer retry", testLengthPrefixedFraming);
test("sys/byte_ring: high-watermark, fillRatioBps & multi-cycle wraparound stress", testHighWatermarkAndMultiCycleStress);
test("sys/byte_ring: non-power-of-two init, empty slice & oversized frame guards", testDefensiveGuards);
