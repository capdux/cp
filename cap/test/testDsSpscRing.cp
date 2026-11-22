// Origin: Pure-CP Stdlib Test Suite (lib/ds/spscRing.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc Power-of-Two Bitmask Batch SPSC Ring & Backpressure Queue:
//   1. Full/empty zero-wasted-slot boundary, peek/peekAt & multi-round wrap-around FIFO integrity
//   2. Batch zero-alloc `pushBatch` / `popBatch` / `skip` with partial-fill rejection accounting
//   3. Real-time telemetry overwrite mode (`pushOverwrite`) returning evicted oldest samples
//   4. Hysteresis high/low watermark backpressure state machine (deadband prevents oscillation)
//   5. Non-power-of-two capacity & invalid watermark rejection guards (`4..=256`)
//   6. 256-slot full-capacity stress, `clear()` & complete `stats()` telemetry snapshot
//   7. `cap.ds` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SPSC_MIN_CAPACITY,
    SPSC_MAX_CAPACITY,
    spscIsValidCapacity,
    SpscWatermarkConfig,
    SpscBackpressureState,
    SpscRingU64,
} from "cap:ds";

test("SpscRing: full/empty zero-wasted-slot boundary, peek & multi-round wrap-around FIFO") {
    let ring = SpscRingU64.init(4) ?? return;
    expect_eq(ring.capacity(), 4 as usize);
    expect_true(ring.isEmpty());
    expect_true(!ring.isFull());
    expect_true(ring.peek() == null);
    expect_true(ring.tryPop() == null);

    // Fill all 4 slots (zero wasted slots)
    expect_true(ring.tryPush(101));
    expect_true(ring.tryPush(102));
    expect_true(ring.tryPush(103));
    expect_true(ring.tryPush(104));
    expect_true(ring.isFull());
    expect_eq(ring.len(), 4 as usize);
    expect_eq(ring.available(), 0 as usize);

    // 5th push rejected
    expect_true(!ring.tryPush(105));
    expect_eq(ring.stats().total_rejected_full, 1 as u64);

    // Peek & peekAt
    expect_eq(ring.peek() ?? 0, 101 as u64);
    expect_eq(ring.peekAt(0) ?? 0, 101 as u64);
    expect_eq(ring.peekAt(3) ?? 0, 104 as u64);
    expect_true(ring.peekAt(4) == null);

    // Drain 2, then run 32 continuous wrap-around push/pop cycles verifying strict FIFO
    expect_eq(ring.tryPop() ?? 0, 101 as u64);
    expect_eq(ring.tryPop() ?? 0, 102 as u64);

    let next_push: u64 = 105;
    let next_expect: u64 = 103;
    let step: usize = 0;
    while (step < 32) : (step += 1) {
        expect_true(ring.tryPush(next_push));
        next_push += 1;
        expect_eq(ring.tryPop() ?? 0, next_expect);
        next_expect += 1;
    }

    // Drain final 2 items
    expect_eq(ring.tryPop() ?? 0, next_expect);
    expect_eq(ring.tryPop() ?? 0, next_expect + 1);
    expect_true(ring.isEmpty());
}

test("SpscRing: batch pushBatch / popBatch / skip with partial-fill accounting") {
    let ring = SpscRingU64.init(8) ?? return;

    const b1: [5]u64 = [1, 2, 3, 4, 5];
    expect_eq(ring.pushBatch(b1[0..]), 5 as usize);
    expect_eq(ring.len(), 5 as usize);

    // Push 6 more into 3 remaining slots -> 3 accepted, 3 rejected
    const b2: [6]u64 = [6, 7, 8, 9, 10, 11];
    expect_eq(ring.pushBatch(b2[0..]), 3 as usize);
    expect_true(ring.isFull());
    expect_eq(ring.stats().total_rejected_full, 3 as u64);

    // Pop 3 items
    let out1: [3]u64 = [0; 3];
    let out1_s: []u64 = out1[0..];
    expect_eq(ring.popBatch(out1_s), 3 as usize);
    expect_eq(out1[0], 1 as u64);
    expect_eq(out1[1], 2 as u64);
    expect_eq(out1[2], 3 as u64);

    // Skip 2 items (4, 5)
    expect_eq(ring.skip(2), 2 as usize);
    expect_eq(ring.len(), 3 as usize);

    // Pop remaining (6, 7, 8) into oversized buffer
    let out2: [8]u64 = [0; 8];
    let out2_s: []u64 = out2[0..];
    expect_eq(ring.popBatch(out2_s), 3 as usize);
    expect_eq(out2[0], 6 as u64);
    expect_eq(out2[1], 7 as u64);
    expect_eq(out2[2], 8 as u64);
    expect_true(ring.isEmpty());
}

test("SpscRing: real-time overwrite mode pushOverwrite returns evicted oldest sample") {
    let ring = SpscRingU64.init(4) ?? return;

    expect_true(ring.pushOverwrite(10) == null);
    expect_true(ring.pushOverwrite(20) == null);
    expect_true(ring.pushOverwrite(30) == null);
    expect_true(ring.pushOverwrite(40) == null);
    expect_true(ring.isFull());

    // Overwriting when full evicts oldest (10, then 20)
    expect_eq(ring.pushOverwrite(50) ?? 0, 10 as u64);
    expect_eq(ring.pushOverwrite(60) ?? 0, 20 as u64);
    expect_eq(ring.len(), 4 as usize);
    expect_eq(ring.stats().total_overwritten, 2 as u64);

    // Remaining window strictly holds newest 4 samples: [30, 40, 50, 60]
    let out: [4]u64 = [0; 4];
    let out_s: []u64 = out[0..];
    expect_eq(ring.popBatch(out_s), 4 as usize);
    expect_eq(out[0], 30 as u64);
    expect_eq(out[1], 40 as u64);
    expect_eq(out[2], 50 as u64);
    expect_eq(out[3], 60 as u64);
}

test("SpscRing: hysteresis high/low watermark backpressure controller") {
    // Capacity 8: high = 750 (6/8), low = 250 (2/8)
    let ring = SpscRingU64.initWithWatermarks(8, SpscWatermarkConfig{
        high_permille: 750,
        low_permille: 250,
    }) ?? return;

    // Push 5 items (625 permille < 750) -> still normal
    const five: [5]u64 = [1, 2, 3, 4, 5];
    * = ring.pushBatch(five[0..]);
    expect_eq(ring.fillPermille(), 625 as u16);
    expect_true(!ring.isAboveHighWatermark());
    expect_true(!ring.isBackpressured());

    // 6th item hits 750 permille -> enters throttled state!
    expect_true(ring.tryPush(6));
    expect_eq(ring.fillPermille(), 750 as u16);
    expect_true(ring.isAboveHighWatermark());
    expect_true(ring.isBackpressured());
    expect_eq(ring.stats().backpressure, SpscBackpressureState.throttled);

    // Pop 3 items (len drops to 3 -> 375 permille > 250): hysteresis keeps it throttled!
    expect_eq(ring.skip(3), 3 as usize);
    expect_eq(ring.fillPermille(), 375 as u16);
    expect_true(!ring.isBelowLowWatermark());
    expect_true(ring.isBackpressured());

    // Pop 1 more item (len drops to 2 -> 250 permille <= 250): clears backpressure!
    * = ring.tryPop();
    expect_eq(ring.fillPermille(), 250 as u16);
    expect_true(ring.isBelowLowWatermark());
    expect_true(!ring.isBackpressured());
    expect_eq(ring.stats().backpressure_transitions, 2 as u64);
}

test("SpscRing: non-power-of-two capacity & invalid watermark rejection guards") {
    expect_true(!spscIsValidCapacity(0));
    expect_true(!spscIsValidCapacity(1));
    expect_true(!spscIsValidCapacity(2));
    expect_true(!spscIsValidCapacity(3));
    expect_true(!spscIsValidCapacity(5));
    expect_true(!spscIsValidCapacity(6));
    expect_true(!spscIsValidCapacity(100));
    expect_true(!spscIsValidCapacity(255));
    expect_true(!spscIsValidCapacity(512));

    expect_true(SpscRingU64.init(0) == null);
    expect_true(SpscRingU64.init(3) == null);
    expect_true(SpscRingU64.init(6) == null);
    expect_true(SpscRingU64.init(300) == null);

    // Invalid watermarks
    expect_true(SpscRingU64.initWithWatermarks(16, SpscWatermarkConfig{ high_permille: 0, low_permille: 0 }) == null);
    expect_true(SpscRingU64.initWithWatermarks(16, SpscWatermarkConfig{ high_permille: 1001, low_permille: 100 }) == null);
    expect_true(SpscRingU64.initWithWatermarks(16, SpscWatermarkConfig{ high_permille: 500, low_permille: 600 }) == null);

    // All valid power-of-two capacities 4..=256 succeed
    let cap_val: usize = SPSC_MIN_CAPACITY;
    while (cap_val <= SPSC_MAX_CAPACITY) : (cap_val *= 2) {
        expect_true(spscIsValidCapacity(cap_val));
        expect_true(SpscRingU64.init(cap_val) != null);
    }
}

test("SpscRing: 256-slot full-capacity stress, clear() & telemetry stats snapshot") {
    let ring = SpscRingU64.init(SPSC_MAX_CAPACITY) ?? return;

    let i: u64 = 0;
    while (i < SPSC_MAX_CAPACITY as u64) : (i += 1) {
        expect_true(ring.tryPush(i * 3 + 1));
    }
    expect_true(ring.isFull());
    expect_eq(ring.fillPermille(), 1000 as u16);
    expect_true(ring.isBackpressured());

    // Clear resets len to 0 and clears backpressure while preserving cumulative telemetry
    ring.clear();
    expect_true(ring.isEmpty());
    expect_true(!ring.isBackpressured());

    const st = ring.stats();
    expect_eq(st.capacity, SPSC_MAX_CAPACITY);
    expect_eq(st.len, 0 as usize);
    expect_eq(st.available, SPSC_MAX_CAPACITY);
    expect_eq(st.peak_len, SPSC_MAX_CAPACITY);
    expect_eq(st.total_pushed, SPSC_MAX_CAPACITY as u64);
    expect_eq(st.backpressure_transitions, 2 as u64);
}

test("SpscRing: cap.ds namespace facade parity") {
    let ring = cap.ds.SpscRingU64.initWithWatermarks(16, cap.ds.SpscWatermarkConfig{
        high_permille: 500,
        low_permille: 125,
    }) ?? return;

    const payload: [8]u64 = [10, 20, 30, 40, 50, 60, 70, 80];
    expect_eq(ring.pushBatch(payload[0..]), 8 as usize);
    expect_true(ring.isBackpressured());

    let out: [8]u64 = [0; 8];
    let out_s: []u64 = out[0..];
    expect_eq(ring.popBatch(out_s), 8 as usize);
    expect_eq(out[0], 10 as u64);
    expect_eq(out[7], 80 as u64);
    expect_true(!ring.isBackpressured());
}
