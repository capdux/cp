import { test, expect_eq } from "cap:test";
import { TokenBucket, FixedWindowLimiter } from "cap:token_bucket";

test("1. TokenBucket burst consumption, sub-second zero-loss refill, and capacity cap", () => {
    // 容量 10，每秒补充 5 个令牌（即每 200ms 补充 1 个）
    let b = TokenBucket.init(10, 5, 1000);
    expect_eq(b.available(1000), u64(10));

    // 突发消费全部 10 个
    expect_eq(b.tryConsume(10, 1000), true);
    expect_eq(b.available(1000), u64(0));
    expect_eq(b.tryConsume(1, 1000), false);

    // 亚秒级增量累加：100ms + 100ms = 200ms -> 精确补足 1 个令牌（零舍入丢失）
    expect_eq(b.available(1100), u64(0));
    expect_eq(b.available(1200), u64(1));
    expect_eq(b.tryConsume(1, 1200), true);

    // 长时间空闲后封顶至 capacity=10
    expect_eq(b.available(100_000), u64(10));
});

test("2. TokenBucket clock rollback protection and overflow safety", () => {
    let b = TokenBucket.initWithTokens(20, 5, 10, 5000);
    expect_eq(b.available(5000), u64(5));

    // 时钟回拨（4000 < 5000）绝不产生负数或异常补充
    expect_eq(b.available(4000), u64(5));
    expect_eq(b.tryConsume(3, 3000), true);
    expect_eq(b.available(4999), u64(2));

    // 极大时间跨度绝不乘法溢出，安全封顶至 capacity
    expect_eq(b.available(0x7FFF_FFFF_FFFF_FFFF), u64(20));
});

test("3. TokenBucket timeUntilReadyMs exact math and impossible requests return null", () => {
    // 容量 10，初始 2，每秒补充 3 个（每 ms 补 3 毫令牌）
    let b = TokenBucket.initWithTokens(10, 2, 3, 0);

    // 已有 2 个 -> 立即就绪
    expect_eq(b.timeUntilReadyMs(0, 0) ?? unreachable, u64(0));
    expect_eq(b.timeUntilReadyMs(2, 0) ?? unreachable, u64(0));

    // 需要 3 个 -> 缺 1000 毫令牌 -> ceil(1000 / 3) = 334ms
    expect_eq(b.timeUntilReadyMs(3, 0) ?? unreachable, u64(334));
    expect_eq(b.tryConsume(3, 333), false);
    expect_eq(b.tryConsume(3, 334), true);

    // 超容量永远不可满足 -> null
    expect_eq(b.timeUntilReadyMs(11, 334) == null, true);

    // refill_per_sec == 0 且令牌不足 -> null；令牌足够 -> 0
    let static_b = TokenBucket.initWithTokens(5, 2, 0, 100);
    expect_eq(static_b.timeUntilReadyMs(2, 100) ?? unreachable, u64(0));
    expect_eq(static_b.timeUntilReadyMs(3, 100) == null, true);
});

test("4. FixedWindowLimiter allow, window rollover, remaining, and resetAfterMs", () => {
    let lim = FixedWindowLimiter.init(3, 1000, 10_000);

    expect_eq(lim.remaining(10_000), u64(3));
    expect_eq(lim.resetAfterMs(10_000), u64(1000));
    expect_eq(lim.resetAfterMs(10_250), u64(750));

    expect_eq(lim.allow(10_100), true);
    expect_eq(lim.allow(10_200), true);
    expect_eq(lim.allow(10_300), true);
    expect_eq(lim.remaining(10_300), u64(0));
    expect_eq(lim.allow(10_999), false);

    // 跨入新窗口（11_000）前只读 remaining 已反映满配额
    expect_eq(lim.remaining(11_000), u64(3));
    expect_eq(lim.resetAfterMs(11_000), u64(1000));
    expect_eq(lim.allow(11_000), true);
    expect_eq(lim.remaining(11_000), u64(2));

    // 跳过多个窗口（例如直接到 15_400）正确对齐到 [15_000, 16_000)
    expect_eq(lim.allow(15_400), true);
    expect_eq(lim.remaining(15_400), u64(2));
    expect_eq(lim.resetAfterMs(15_400), u64(600));
});

test("5. FixedWindowLimiter clock rollback guard and zero-limit edge case", () => {
    let lim = FixedWindowLimiter.init(2, 1000, 5000);
    expect_eq(lim.allow(5100), true);
    expect_eq(lim.allow(5200), true);
    expect_eq(lim.allow(5300), false);

    // 时钟回拨（4500 < 5000）不能重置计数绕过限流
    expect_eq(lim.allow(4500), false);
    expect_eq(lim.remaining(4500), u64(0));

    // limit=0 始终拒绝
    let zero = FixedWindowLimiter.init(0, 1000, 0);
    expect_eq(zero.allow(0), false);
    expect_eq(zero.remaining(0), u64(0));
});
