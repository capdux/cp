// Origin: Pure-CP Stdlib Test Suite (lib/time/timing_wheel.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Hierarchical Hashed Timing Wheel:
//   1. Level 0 exact short-delay firing, same-bucket multi-timer firing & next_expiry_tick
//   2. Level 1 (>=64) & Level 2 (>=4096) cross-level cascade demotion & exact deadline firing
//   3. O(1) handle cancellation & slot-reuse stale-generation ABA protection
//   4. Periodic timer multi-round auto-rescheduling (L0 & L1 intervals) & mid-flight cancel
//   5. Clock regression guard (target_tick < current_tick no-op)
//   6. Full 256-timer capacity, 0-delay immediate firing & >262,143 max-span rejection
//   7. Full lifecycle statistics snapshot conservation check

import { test, expect_eq } from "cap:test";
import {
    TW_SLOTS_PER_LEVEL,
    TW_LEVELS,
    TW_MAX_TIMERS,
    TW_INVALID_IDX,
    TimerHandle,
    WheelTimerEntry,
    TimingWheelStats,
    HierarchicalTimingWheel,
} from "cap:time";

// ============================================================================
// Test 1: Level 0 Short-Delay Firing, Same-Bucket Multi-Timers & next_expiry_tick
// ============================================================================
test("timing_wheel: Level 0 exact firing, same-bucket timers & next_expiry_tick", () => {
    let tw = HierarchicalTimingWheel.init(100);
    expect_eq(tw.next_expiry_tick() == null, true);

    expect_eq(tw.schedule(5, 1001) != null, true);
    expect_eq(tw.schedule(5, 1002) != null, true); // same bucket & deadline (105)
    expect_eq(tw.schedule(12, 1003) != null, true);
    expect_eq(tw.next_expiry_tick() ?? 0, 105);

    let fired: [8]u64 = [0; 8];
    let fired_s: []u64 = fired[0..];

    // Advance to 104 -> nothing fires yet
    expect_eq(tw.advance_to(104, fired_s), 0);
    expect_eq(tw.stats().active_timers, 3);

    // Advance to 105 -> both 1001 and 1002 fire
    const n1 = tw.advance_to(105, fired_s);
    expect_eq(n1, 2);
    const got_both = (fired[0] == 1001 && fired[1] == 1002) || (fired[0] == 1002 && fired[1] == 1001);
    expect_eq(got_both, true);
    expect_eq(tw.next_expiry_tick() ?? 0, 112);

    // Advance to 112 -> 1003 fires
    const n2 = tw.advance_to(112, fired_s);
    expect_eq(n2, 1);
    expect_eq(fired[0], 1003);
    expect_eq(tw.next_expiry_tick() == null, true);
});

// ============================================================================
// Test 2: Level 1 (>=64) & Level 2 (>=4096) Cross-Level Cascade Firing
// ============================================================================
test("timing_wheel: Level 1 and Level 2 cascade demotion fires at exact deadlines", () => {
    // Non-aligned start tick 63 to exercise immediate wrap boundary (63 -> 64)
    let tw = HierarchicalTimingWheel.init(63);

    expect_eq(tw.schedule(64, 11) != null, true);     // deadline = 127 (Level 1)
    expect_eq(tw.schedule(65, 12) != null, true);     // deadline = 128 (Level 1)
    expect_eq(tw.schedule(4096, 21) != null, true);   // deadline = 4159 (Level 2)
    expect_eq(tw.schedule(20_000, 22) != null, true); // deadline = 20063 (Level 2)

    let fired: [8]u64 = [0; 8];
    let fired_s: []u64 = fired[0..];

    // Before 127 -> 0 fired
    expect_eq(tw.advance_to(126, fired_s), 0);
    // Exact 127 -> tag 11
    expect_eq(tw.advance_to(127, fired_s), 1);
    expect_eq(fired[0], 11);
    // Exact 128 -> tag 12
    expect_eq(tw.advance_to(128, fired_s), 1);
    expect_eq(fired[0], 12);

    // Before 4159 -> 0 fired
    expect_eq(tw.advance_to(4158, fired_s), 0);
    // Exact 4159 -> tag 21
    expect_eq(tw.advance_to(4159, fired_s), 1);
    expect_eq(fired[0], 21);

    // Before 20063 -> 0 fired
    expect_eq(tw.advance_to(20_062, fired_s), 0);
    // Exact 20063 -> tag 22
    expect_eq(tw.advance_to(20_063, fired_s), 1);
    expect_eq(fired[0], 22);

    const st = tw.stats();
    expect_eq(st.active_timers, 0);
    expect_eq(st.total_fired, 4);
    expect_eq(st.total_cascades > 0, true);
});

// ============================================================================
// Test 3: O(1) Cancel & Generation-Checked ABA Protection on Slot Reuse
// ============================================================================
test("timing_wheel: O(1) cancel & stale generation ABA protection after slot reuse", () => {
    let tw = HierarchicalTimingWheel.init(0);

    const h1 = tw.schedule(20, 111) ?? return;
    expect_eq(tw.cancel(h1), true);
    // Double cancel returns false
    expect_eq(tw.cancel(h1), false);

    // Next schedule reuses the exact same freed slot_idx with incremented generation
    const h2 = tw.schedule(20, 222) ?? return;
    expect_eq(h2.slot_idx, h1.slot_idx);
    expect_eq(h2.generation != h1.generation, true);

    // Stale handle h1 must NOT cancel the reused slot h2!
    expect_eq(tw.cancel(h1), false);
    expect_eq(tw.stats().active_timers, 1);

    // h2 fires cleanly at tick 20
    let fired: [4]u64 = [0; 4];
    let fired_s: []u64 = fired[0..];
    expect_eq(tw.advance_to(20, fired_s), 1);
    expect_eq(fired[0], 222);

    // Invalid sentinel handle rejected
    expect_eq(tw.cancel(TimerHandle{}), false);
});

// ============================================================================
// Test 4: Periodic Timer Multi-Round Auto-Rescheduling & Mid-Flight Cancel
// ============================================================================
test("timing_wheel: periodic timer reschedules across L0/L1 and stops on cancel", () => {
    let tw = HierarchicalTimingWheel.init(0);

    // Short periodic timer: first @10, then every 15 ticks (10, 25, 40, 55)
    const hp = tw.schedule_periodic(10, 15, 777) ?? return;
    // Cross-level periodic timer: first @80, then every 80 ticks (80, 160)
    const hp_l1 = tw.schedule_periodic(80, 80, 888) ?? return;

    let fired: [16]u64 = [0; 16];
    let fired_s: []u64 = fired[0..];

    const n1 = tw.advance_to(55, fired_s);
    expect_eq(n1, 4);
    expect_eq(fired[0], 777);
    expect_eq(fired[1], 777);
    expect_eq(fired[2], 777);
    expect_eq(fired[3], 777);

    // Cancel short periodic timer before tick 70
    expect_eq(tw.cancel(hp), true);

    // Advance to 165 -> only hp_l1 fires twice (@80, @160)
    const n2 = tw.advance_to(165, fired_s);
    expect_eq(n2, 2);
    expect_eq(fired[0], 888);
    expect_eq(fired[1], 888);

    expect_eq(tw.cancel(hp_l1), true);
    expect_eq(tw.stats().active_timers, 0);
});

// ============================================================================
// Test 5: Clock Regression Guard (target_tick < current_tick No-Op)
// ============================================================================
test("timing_wheel: clock regression target_tick < current_tick is a safe no-op", () => {
    let tw = HierarchicalTimingWheel.init(500);
    expect_eq(tw.schedule(10, 42) != null, true);

    let fired: [4]u64 = [0; 4];
    let fired_s: []u64 = fired[0..];
    expect_eq(tw.advance_to(400, fired_s), 0);
    expect_eq(tw.stats().current_tick, 500);
    expect_eq(tw.stats().active_timers, 1);

    // Normal advance to 510 still fires tag 42
    expect_eq(tw.advance_to(510, fired_s), 1);
    expect_eq(fired[0], 42);
});

// ============================================================================
// Test 6: Full 256-Timer Capacity, 0-Delay Immediate Firing & Span Guards
// ============================================================================
test("timing_wheel: 0-delay immediate firing, 256-slot cap & max-span rejection", () => {
    expect_eq(TW_SLOTS_PER_LEVEL, 64);
    expect_eq(TW_LEVELS, 3);
    expect_eq(TW_MAX_TIMERS, 256);
    expect_eq(TW_INVALID_IDX, 0xFFFF);

    let tw = HierarchicalTimingWheel.init(10);

    // Out-of-span delay & zero periodic interval rejected
    expect_eq(tw.schedule(262_144, 1) == null, true);
    expect_eq(tw.schedule_periodic(10, 0, 1) == null, true);
    expect_eq(tw.schedule_periodic(10, 262_144, 1) == null, true);

    // 0-delay timer fires immediately on advance_to(current_tick)
    expect_eq(tw.schedule(0, 999) != null, true);
    let fired: [4]u64 = [0; 4];
    let fired_s: []u64 = fired[0..];
    expect_eq(tw.advance_to(10, fired_s), 1);
    expect_eq(fired[0], 999);

    // Fill all 256 slots
    let i: u64 = 0;
    while (i < 256) : (i += 1) {
        expect_eq(tw.schedule(5 + (i % 60), i + 1) != null, true);
    }
    expect_eq(tw.stats().active_timers, 256);
    // 257th rejected cleanly
    expect_eq(tw.schedule(5, 9999) == null, true);

    // Advance past all 256 timers -> all slots reclaimed
    let big_out: [256]u64 = [0; 256];
    let big_s: []u64 = big_out[0..];
    expect_eq(tw.advance_to(100, big_s), 256);
    expect_eq(tw.stats().active_timers, 0);

    // Slot pool fully reusable
    expect_eq(tw.schedule(1, 555) != null, true);
    expect_eq(tw.stats().active_timers, 1);
});

// ============================================================================
// Test 7: Statistics Snapshot Conservation
// ============================================================================
test("timing_wheel: stats snapshot accurately tracks scheduled, fired, canceled & cascades", () => {
    let tw = HierarchicalTimingWheel.init(0);
    const h1 = tw.schedule(5, 1) ?? return;
    const h2 = tw.schedule(70, 2) ?? return;
    const h3 = tw.schedule(100, 3) ?? return;
    * = h1;
    * = h2;

    expect_eq(tw.cancel(h3), true);

    let fired: [4]u64 = [0; 4];
    let fired_s: []u64 = fired[0..];
    expect_eq(tw.advance_to(75, fired_s), 2);

    const st = tw.stats();
    expect_eq(st.current_tick, 75);
    expect_eq(st.active_timers, 0);
    expect_eq(st.total_scheduled, 3);
    expect_eq(st.total_fired, 2);
    expect_eq(st.total_canceled, 1);
    expect_eq(st.total_cascades >= 1, true);
});
