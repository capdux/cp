import { test, expect_eq } from "cap:test";
import {
    JitterMode,
    BackoffConfig,
    BackoffState,
    parseRetryAfter,
    isRetryableHttpStatus,
} from "cap:net";

test("1. JitterMode.none exponential growth, max_delay_ms cap, overflow guard, and exhaustion", () => {
    const cfg = BackoffConfig{
        base_delay_ms: 100,
        max_delay_ms: 500,
        max_attempts: 5,
        jitter: JitterMode.none,
    };
    let st = BackoffState.init(1);

    expect_eq(st.isExhausted(cfg), false);
    expect_eq(st.nextDelayMs(cfg) ?? unreachable, u64(100));
    expect_eq(st.nextDelayMs(cfg) ?? unreachable, u64(200));
    expect_eq(st.nextDelayMs(cfg) ?? unreachable, u64(400));
    expect_eq(st.nextDelayMs(cfg) ?? unreachable, u64(500));
    expect_eq(st.nextDelayMs(cfg) ?? unreachable, u64(500));
    expect_eq(st.isExhausted(cfg), true);
    expect_eq(st.nextDelayMs(cfg) == null, true);

    // reset 后重新从 100ms 开始
    st.reset();
    expect_eq(st.isExhausted(cfg), false);
    expect_eq(st.nextDelayMs(cfg) ?? unreachable, u64(100));

    // 高 attempt（如 100 次）绝不发生移位或乘法溢出
    const huge_cfg = BackoffConfig{
        base_delay_ms: 1000,
        max_delay_ms: 60_000,
        max_attempts: 100,
        jitter: JitterMode.none,
    };
    let st2 = BackoffState.init(0);
    let i: u32 = 0;
    while (i < 100) {
        const d = st2.nextDelayMs(huge_cfg) ?? unreachable;
        expect_eq(d <= 60_000, true);
        i += 1;
    }
    expect_eq(st2.nextDelayMs(huge_cfg) == null, true);
});

test("2. Full and Equal jitter bounds and deterministic seed reproducibility", () => {
    const full_cfg = BackoffConfig{
        base_delay_ms: 200,
        max_delay_ms: 2000,
        max_attempts: 6,
        jitter: JitterMode.full,
    };
    let a = BackoffState.init(0x1234_5678);
    let b = BackoffState.init(0x1234_5678);

    const caps: [6]u64 = [200, 400, 800, 1600, 2000, 2000];
    let i: usize = 0;
    while (i < 6) {
        const da = a.nextDelayMs(full_cfg) ?? unreachable;
        const db = b.nextDelayMs(full_cfg) ?? unreachable;
        expect_eq(da, db);
        expect_eq(da <= caps[i], true);
        i += 1;
    }

    // Equal jitter 严格落在 [cap/2, cap]
    const eq_cfg = BackoffConfig{
        base_delay_ms: 200,
        max_delay_ms: 2000,
        max_attempts: 6,
        jitter: JitterMode.equal,
    };
    let c = BackoffState.init(999);
    let j: usize = 0;
    while (j < 6) {
        const d = c.nextDelayMs(eq_cfg) ?? unreachable;
        expect_eq(d >= caps[j] / 2, true);
        expect_eq(d <= caps[j], true);
        j += 1;
    }
});

test("3. parseRetryAfter decimal seconds and whitespace trimming", () => {
    const r1 = parseRetryAfter("120", 1700000000) ?? unreachable;
    expect_eq(r1.delay_secs, u64(120));
    expect_eq(r1.is_http_date, false);

    const r2 = parseRetryAfter("  0  ", 1700000000) ?? unreachable;
    expect_eq(r2.delay_secs, u64(0));
    expect_eq(r2.is_http_date, false);

    expect_eq(parseRetryAfter("", 1700000000) == null, true);
    expect_eq(parseRetryAfter("-5", 1700000000) == null, true);
    expect_eq(parseRetryAfter("12s", 1700000000) == null, true);
});

test("4. parseRetryAfter RFC 9110 IMF-fixdate exact epoch math and past-date clamping", () => {
    // Wed, 21 Oct 2015 07:28:00 GMT == 1445412480
    const target_epoch: i64 = 1445412480;
    const future = parseRetryAfter("Wed, 21 Oct 2015 07:28:00 GMT", target_epoch - 90) ?? unreachable;
    expect_eq(future.is_http_date, true);
    expect_eq(future.delay_secs, u64(90));

    // 过去时间安全钳制为 0 秒
    const past = parseRetryAfter("Wed, 21 Oct 2015 07:28:00 GMT", target_epoch + 500) ?? unreachable;
    expect_eq(past.is_http_date, true);
    expect_eq(past.delay_secs, u64(0));

    // 闰年校验：2024-02-29 合法，2023-02-29 非法
    expect_eq(parseRetryAfter("Thu, 29 Feb 2024 00:00:00 GMT", 1709164700) != null, true);
    expect_eq(parseRetryAfter("Wed, 29 Feb 2023 00:00:00 GMT", 1677628800) == null, true);
    expect_eq(parseRetryAfter("Wed, 21 Oct 2015 07:28:00 UTC", target_epoch) == null, true);
});

test("5. isRetryableHttpStatus and defaultConfig sanity", () => {
    const def = BackoffConfig.defaultConfig();
    expect_eq(def.base_delay_ms > 0, true);
    expect_eq(def.max_delay_ms >= def.base_delay_ms, true);
    expect_eq(def.max_attempts > 0, true);

    expect_eq(isRetryableHttpStatus(408), true);
    expect_eq(isRetryableHttpStatus(425), true);
    expect_eq(isRetryableHttpStatus(429), true);
    expect_eq(isRetryableHttpStatus(500), true);
    expect_eq(isRetryableHttpStatus(502), true);
    expect_eq(isRetryableHttpStatus(503), true);
    expect_eq(isRetryableHttpStatus(504), true);

    expect_eq(isRetryableHttpStatus(200), false);
    expect_eq(isRetryableHttpStatus(400), false);
    expect_eq(isRetryableHttpStatus(401), false);
    expect_eq(isRetryableHttpStatus(403), false);
    expect_eq(isRetryableHttpStatus(404), false);
    expect_eq(isRetryableHttpStatus(422), false);
    expect_eq(isRetryableHttpStatus(501), false);
});
