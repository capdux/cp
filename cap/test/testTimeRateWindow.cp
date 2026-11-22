// Origin: Pure-CP Stdlib Test Suite (lib/time/rateWindow.cp) | Created: 2026-11-23
// Deep production regression tests for cap:time multi-tenant sliding window counter (`lib/time/rateWindow.cp`):
//   1. Cross-window Cloudflare hybrid interpolation prevents 2x fixed-window boundary bursts
//   2. 1ms-exact `retry_after_ms` in both same-window decay and next-window rollover scenarios
//   3. Weighted `cost > 1` consumption, tenant isolation, and per-key `setKeyQuota` / `clearKeyQuota`
//   4. Read-only `peek()` zero-side-effect guarantee + multi-window silence auto-zeroing & `reset_at_ms`
//   5. 128-slot capacity saturation with expired-first and LRU eviction of coldest tenants
//   6. Invalid config guards, oversized `cost > limit`, backwards clock clamp, and `purgeExpired()`
//   7. Cumulative zero-heap telemetry snapshot, `resetAll()`, and `cap.time` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    RATE_WINDOW_MAX_KEYS,
    RateWindowConfig,
    RateWindowLimiter,
} from "cap:time";

test("rateWindow: cross-window hybrid interpolation eliminates 2x boundary bursts") {
    let limiter = RateWindowLimiter.init(10, 1000) ?? return;
    const tenant: u64 = 0x1001;

    // Consume all 10 units near the end of window 0 (t = 900ms)
    const d0 = limiter.checkAndConsume(tenant, 10, 900);
    expect_true(d0.allowed);
    expect_eq(d0.remaining, @as(u64, 0));
    expect_eq(d0.effective_used, @as(u64, 10));
    expect_eq(d0.retry_after_ms, @as(u64, 0));

    // At window 1 boundary (t = 1000ms, elapsed = 0ms): prev weight = 100%, effective_used = 10 -> 2x burst blocked!
    const d1 = limiter.checkAndConsume(tenant, 1, 1000);
    expect_eq(d1.allowed, false);
    expect_eq(d1.remaining, @as(u64, 0));
    expect_eq(d1.effective_used, @as(u64, 10));
    expect_eq(d1.retry_after_ms, @as(u64, 100));

    // At t = 1200ms (elapsed = 200ms): prev weight = 80%, effective_used = 8 -> only 2 units available
    const d2 = limiter.checkAndConsume(tenant, 3, 1200);
    expect_eq(d2.allowed, false);
    expect_eq(d2.remaining, @as(u64, 2));
    expect_eq(d2.effective_used, @as(u64, 8));

    const d3 = limiter.checkAndConsume(tenant, 2, 1200);
    expect_true(d3.allowed);
    expect_eq(d3.remaining, @as(u64, 0));
    expect_eq(d3.effective_used, @as(u64, 10));

    // At t = 1500ms (elapsed = 500ms): prev=10*0.5=5 + curr=2 => effective_used=7, remaining=3
    const p = limiter.peek(tenant, 1500);
    expect_true(p.allowed);
    expect_eq(p.effective_used, @as(u64, 7));
    expect_eq(p.remaining, @as(u64, 3));
}

test("rateWindow: 1ms-exact retry_after_ms for same-window decay and next-window rollover") {
    let limiter = RateWindowLimiter.init(10, 1000) ?? return;

    // Scenario A: Same-window decay
    const t_a: u64 = 0x2001;
    * = limiter.checkAndConsume(t_a, 10, 800); // window 0 full
    // At t = 1200ms (prev=10, curr=0, elapsed=200ms), cost=3 needs prev*(800-dt) + 3000 <= 10000 => dt >= 100ms
    const rej_a = limiter.checkAndConsume(t_a, 3, 1200);
    expect_eq(rej_a.allowed, false);
    expect_eq(rej_a.retry_after_ms, @as(u64, 100));

    // 1ms before retry_after_ms -> still rejected
    const early_a = limiter.checkAndConsume(t_a, 3, 1299);
    expect_eq(early_a.allowed, false);
    // Exact retry_after_ms -> allowed!
    const ok_a = limiter.checkAndConsume(t_a, 3, 1300);
    expect_true(ok_a.allowed);
    expect_eq(ok_a.remaining, @as(u64, 0));

    // Scenario B: Next-window rollover (curr_count + cost > limit)
    const t_b: u64 = 0x2002;
    * = limiter.checkAndConsume(t_b, 10, 200); // window 0 full at t = 200ms
    // Need to wait 800ms to reach window 1 (t=1000), plus 100ms for prev=10 to decay to 9 => total 900ms (t=1100)
    const rej_b = limiter.checkAndConsume(t_b, 1, 200);
    expect_eq(rej_b.allowed, false);
    expect_eq(rej_b.retry_after_ms, @as(u64, 900));

    // 1ms before (t = 1099ms) -> strictly rejected
    const early_b = limiter.checkAndConsume(t_b, 1, 1099);
    expect_eq(early_b.allowed, false);
    // Exact retry_after_ms (t = 1100ms) -> allowed!
    const ok_b = limiter.checkAndConsume(t_b, 1, 1100);
    expect_true(ok_b.allowed);
    expect_eq(ok_b.effective_used, @as(u64, 10));
}

test("rateWindow: weighted cost > 1, tenant isolation, and per-key quota overrides") {
    let limiter = RateWindowLimiter.init(10, 1000) ?? return;
    const free_user: u64 = 0x3001;
    const vip_user: u64 = 0x3002;

    // Give VIP user 50 req / 2000ms
    expect_true(limiter.setKeyQuota(vip_user, 50, 2000));
    expect_eq(limiter.setKeyQuota(vip_user, 0, 2000), false);
    expect_eq(limiter.setKeyQuota(vip_user, 50, 0), false);

    // Free user hits default limit of 10
    const f1 = limiter.checkAndConsume(free_user, 6, 500);
    expect_true(f1.allowed);
    expect_eq(f1.remaining, @as(u64, 4));
    expect_eq(f1.limit, @as(u64, 10));

    const f2 = limiter.checkAndConsume(free_user, 5, 500);
    expect_eq(f2.allowed, false);
    expect_eq(f2.remaining, @as(u64, 4));

    // VIP user is completely isolated and enjoys 50 quota
    const v1 = limiter.checkAndConsume(vip_user, 35, 500);
    expect_true(v1.allowed);
    expect_eq(v1.remaining, @as(u64, 15));
    expect_eq(v1.limit, @as(u64, 50));

    // Clearing VIP override restores default quota (limit=10)
    expect_true(limiter.clearKeyQuota(vip_user));
    const v_after = limiter.peek(vip_user, 5000); // after silence, fresh default limit=10
    expect_eq(v_after.limit, @as(u64, 10));
    expect_eq(v_after.remaining, @as(u64, 10));
}

test("rateWindow: read-only peek zero-side-effects and multi-window silence auto-zeroing") {
    let limiter = RateWindowLimiter.init(20, 1000) ?? return;
    const key: u64 = 0x4001;

    // Peek on unknown key returns full quota and does not allocate a slot or increment requests
    const p0 = limiter.peek(key, 100);
    expect_true(p0.allowed);
    expect_eq(p0.remaining, @as(u64, 20));
    expect_eq(p0.effective_used, @as(u64, 0));
    expect_eq(limiter.activeCount(), @as(usize, 0));
    expect_eq(limiter.stats().total_requests, @as(u64, 0));

    // Consume 15 in window 0 (t = 400ms) -> reset_at_ms should be (0 + 2) * 1000 = 2000
    const d1 = limiter.checkAndConsume(key, 15, 400);
    expect_true(d1.allowed);
    expect_eq(d1.reset_at_ms, @as(i64, 2000));

    // Multiple peeks do not mutate counters or stats
    const p1 = limiter.peek(key, 400);
    const p2 = limiter.peek(key, 400);
    expect_eq(p1.remaining, @as(u64, 5));
    expect_eq(p2.remaining, @as(u64, 5));
    expect_eq(limiter.stats().total_requests, @as(u64, 1));

    // At t = reset_at_ms (2000ms, window 2 = window_index + 2): auto-zeroed back to full 20
    const p3 = limiter.peek(key, 2000);
    expect_true(p3.allowed);
    expect_eq(p3.effective_used, @as(u64, 0));
    expect_eq(p3.remaining, @as(u64, 20));

    const d2 = limiter.checkAndConsume(key, 20, 2000);
    expect_true(d2.allowed);
    expect_eq(d2.remaining, @as(u64, 0));
}

test("rateWindow: 128-slot saturation with expired-first and LRU eviction") {
    let limiter = RateWindowLimiter.init(5, 1000) ?? return;

    // Fill all 128 slots:
    //   key 1 at t=100 (cold, will expire by t=3000)
    //   key 2..128 at t=2500 (fresh in window 2)
    * = limiter.checkAndConsume(1, 5, 100);
    let k: u64 = 2;
    while (k <= @as(u64, RATE_WINDOW_MAX_KEYS)) {
        * = limiter.checkAndConsume(k, 4, 2500);
        k += 1;
    }
    expect_eq(limiter.activeCount(), RATE_WINDOW_MAX_KEYS);
    expect_eq(limiter.stats().total_evictions, @as(u64, 0));

    // Touch key 2 at t=2600 so key 3 becomes the oldest active LRU among fresh keys
    * = limiter.checkAndConsume(2, 1, 2600);

    // Insert key 9001 at t=3000: key 1 (window 0 vs curr window 3) is expired -> must be evicted first!
    * = limiter.checkAndConsume(9001, 2, 3000);
    expect_eq(limiter.activeCount(), RATE_WINDOW_MAX_KEYS);
    expect_eq(limiter.stats().total_evictions, @as(u64, 1));
    // Key 3 (fresh) must still retain its usage from window 2 (prev_count=4 at t=3000 => effective_used=4)
    expect_eq(limiter.peek(3, 3000).effective_used, @as(u64, 4));
    // Key 1 was evicted, so peeking it returns fresh remaining=5
    expect_eq(limiter.peek(1, 3000).remaining, @as(u64, 5));

    // Now all 128 slots are active at t=3000 (none expired). Insert key 9002 -> must evict LRU (key 3)!
    * = limiter.checkAndConsume(9002, 1, 3000);
    expect_eq(limiter.stats().total_evictions, @as(u64, 2));
    // Key 3 was evicted (returns fresh remaining=5), while touched key 2 is preserved!
    expect_eq(limiter.peek(3, 3000).remaining, @as(u64, 5));
    expect_eq(limiter.peek(2, 3000).effective_used, @as(u64, 5));
}

test("rateWindow: invalid configs, oversized cost, clock rollback clamp, and purgeExpired") {
    expect_true(RateWindowConfig.init(0, 1000) == null);
    expect_true(RateWindowConfig.init(10, 0) == null);
    expect_true(RateWindowLimiter.init(0, 1000) == null);
    expect_true(RateWindowLimiter.init(10, 0) == null);

    let limiter = RateWindowLimiter.init(10, 1000) ?? return;
    const key: u64 = 0x6001;

    // Single request cost > limit is rejected with retry_after_ms = window_ms
    const big = limiter.checkAndConsume(key, 11, 500);
    expect_eq(big.allowed, false);
    expect_eq(big.retry_after_ms, @as(u64, 1000));

    // Normal consumption at t = 1500ms
    const ok1 = limiter.checkAndConsume(key, 7, 1500);
    expect_true(ok1.allowed);
    expect_eq(ok1.remaining, @as(u64, 3));

    // Backwards clock jump (t = 1200ms < 1500ms) is clamped to 1500ms, preserving state
    const ok2 = limiter.checkAndConsume(key, 3, 1200);
    expect_true(ok2.allowed);
    expect_eq(ok2.remaining, @as(u64, 0));

    // Add a second key with custom quota, then purgeExpired at t = 5000ms:
    // key (no custom quota) is purged, custom key is preserved
    const custom_k: u64 = 0x6002;
    expect_true(limiter.setKeyQuota(custom_k, 5, 1000));
    * = limiter.checkAndConsume(custom_k, 1, 1500);
    expect_eq(limiter.activeCount(), @as(usize, 2));

    const purged = limiter.purgeExpired(5000);
    expect_eq(purged, @as(usize, 1));
    expect_eq(limiter.activeCount(), @as(usize, 1));
    expect_true(limiter.removeKey(custom_k));
    expect_eq(limiter.activeCount(), @as(usize, 0));
}

test("rateWindow: cumulative telemetry snapshot, resetAll, and cap.time facade parity") {
    const cfg = cap.time.RateWindowConfig.init(4, 1000) ?? return;
    let limiter = cap.time.RateWindowLimiter.initWithConfig(cfg) ?? return;

    * = limiter.checkAndConsume(77, 2, 100); // allowed
    * = limiter.checkAndConsume(77, 2, 200); // allowed
    * = limiter.checkAndConsume(77, 1, 300); // rejected
    * = limiter.checkAndConsume(88, 1, 300); // allowed

    const st = limiter.stats();
    expect_eq(st.capacity, RATE_WINDOW_MAX_KEYS);
    expect_eq(st.active_keys, @as(usize, 2));
    expect_eq(st.total_requests, @as(u64, 4));
    expect_eq(st.total_allowed, @as(u64, 3));
    expect_eq(st.total_rejected, @as(u64, 1));
    expect_eq(st.total_evictions, @as(u64, 0));

    limiter.resetAll();
    const st2 = limiter.stats();
    expect_eq(st2.active_keys, @as(usize, 0));
    expect_eq(st2.total_requests, @as(u64, 0));
    expect_eq(st2.total_allowed, @as(u64, 0));
    expect_eq(st2.total_rejected, @as(u64, 0));
}
