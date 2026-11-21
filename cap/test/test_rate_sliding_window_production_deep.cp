import {
    SlidingWindowCounter,
    SlidingGcraLimiter,
} from "cap:rate";
import { test, expect } from "cap:test";

test("SlidingWindowCounter smooth boundary transition and rollback safety", () => {
    let sw = SlidingWindowCounter.init(10, 1000, 1000);
    // Consume 8 in window [1000, 2000)
    expect(sw.tryAcquire(8, 1200));
    expect(sw.remaining(1200) == 2);
    expect(!sw.tryAcquire(3, 1500));

    // At t=2500 (halfway through next window), prev_count=8 has 50% overlap -> weighted=4
    expect(sw.estimatedCount(2500) == 4);
    expect(sw.remaining(2500) == 6);
    expect(sw.tryAcquire(6, 2500));
    expect(!sw.tryAcquire(1, 2500));

    // Clock rollback must not grant free quota
    expect(!sw.tryAcquire(1, 2000));

    // Jump 2+ windows ahead -> previous window fully expired
    expect(sw.remaining(5000) == 10);
});

test("SlidingGcraLimiter burst tolerance and exact retry-after ms", () => {
    // 1 req / 100ms, burst capacity 3
    let gcra = SlidingGcraLimiter.init(100, 3, 1000);
    expect(gcra.checkAndAcquire(1000) == 0); // tat=1100
    expect(gcra.checkAndAcquire(1000) == 0); // tat=1200
    expect(gcra.checkAndAcquire(1000) == 0); // tat=1300
    // 4th immediate request rejected; allow_at = 1300 - 200 = 1100 -> wait 100ms
    expect(gcra.checkAndAcquire(1000) == 100);
    expect(gcra.checkAndAcquire(1050) == 50);
    expect(gcra.checkAndAcquire(1100) == 0);
});
