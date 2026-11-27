import { test, expect_eq } from "cap:test";
import {
    RateLimitHit,
    RateLimiter,
    formatRateLimitHeader,
} from "../../packages/express-rate-limit/src/index.cp";

fn main() {
    test("1. @cap/express-rate-limit — allows requests up to max_requests and blocks excess", () => {
        let limiter = RateLimiter.init(60000, 3);
        const h1 = limiter.hit("10.0.0.1", 1000);
        expect_eq(h1.allowed, true);
        expect_eq(h1.limit, 3 as u32);
        expect_eq(h1.current, 1 as u32);
        expect_eq(h1.remaining, 2 as u32);
        expect_eq(h1.reset_at_ms, 61000 as i64);
        expect_eq(h1.retry_after_sec, 60 as u32);

        const h2 = limiter.hit("10.0.0.1", 2000);
        expect_eq(h2.allowed, true);
        expect_eq(h2.current, 2 as u32);
        expect_eq(h2.remaining, 1 as u32);

        const h3 = limiter.hit("10.0.0.1", 3000);
        expect_eq(h3.allowed, true);
        expect_eq(h3.current, 3 as u32);
        expect_eq(h3.remaining, 0 as u32);

        const h4 = limiter.hit("10.0.0.1", 3500);
        expect_eq(h4.allowed, false);
        expect_eq(h4.current, 4 as u32);
        expect_eq(h4.remaining, 0 as u32);
        expect_eq(h4.retry_after_sec, 58 as u32);
    });

    test("2. @cap/express-rate-limit — resets window cleanly after expiration", () => {
        let limiter = RateLimiter.init(10000, 2);
        * = limiter.hit("client-a", 1000);
        * = limiter.hit("client-a", 2000);
        const blocked = limiter.hit("client-a", 5000);
        expect_eq(blocked.allowed, false);

        const fresh = limiter.hit("client-a", 11000);
        expect_eq(fresh.allowed, true);
        expect_eq(fresh.current, 1 as u32);
        expect_eq(fresh.remaining, 1 as u32);
        expect_eq(fresh.reset_at_ms, 21000 as i64);
    });

    test("3. @cap/express-rate-limit — isolates separate keys and tracks activeCount", () => {
        let limiter = RateLimiter.init(5000, 2);
        * = limiter.hit("ip-1", 1000);
        * = limiter.hit("ip-2", 2000);
        * = limiter.hit("ip-3", 4000);
        expect_eq(limiter.activeCount(4500), 3 as usize);
        expect_eq(limiter.activeCount(6500), 2 as usize);
        expect_eq(limiter.activeCount(9500), 0 as usize);
    });

    test("4. @cap/express-rate-limit — resetKey and resetAll clear rate limit state", () => {
        let limiter = RateLimiter.init(30000, 1);
        * = limiter.hit("user-1", 1000);
        * = limiter.hit("user-2", 1000);
        expect_eq(limiter.hit("user-1", 1500).allowed, false);

        limiter.resetKey("user-1");
        expect_eq(limiter.activeCount(1500), 1 as usize);
        expect_eq(limiter.hit("user-1", 1600).allowed, true);

        limiter.resetAll();
        expect_eq(limiter.activeCount(1600), 0 as usize);
    });

    test("5. @cap/express-rate-limit — evicts expired or oldest slots when table is full", () => {
        let limiter = RateLimiter.init(1000, 5);
        let key_buf: [16]u8 = [0 as u8; 16];
        let i: usize = 0;
        while (i < 64) : (i += 1) {
            key_buf[0] = 'k';
            key_buf[1] = u8((i / 10) + 48);
            key_buf[2] = u8((i % 10) + 48);
            * = limiter.hit(key_buf[0..3] as string, 1000 + i64(i * 10));
        }
        expect_eq(limiter.activeCount(1700), 64 as usize);

        // Insert 65th key at t=2100 where earliest slots have expired
        const h = limiter.hit("overflow-key", 2100);
        expect_eq(h.allowed, true);
        expect_eq(h.current, 1 as u32);
    });

    test("6. @cap/express-rate-limit — formats IETF draft-7 RateLimit header", () => {
        let limiter = RateLimiter.init(60000, 100);
        const h = limiter.hit("api-key", 5000);
        let buf: [128]u8 = [0 as u8; 128];
        const n = formatRateLimitHeader(h, buf[0..128]);
        expect_eq(buf[0..n] as string, "limit=100, remaining=99, reset=60");
    });
}
