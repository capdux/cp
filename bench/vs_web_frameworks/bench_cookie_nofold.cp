// Non-folding ReleaseFast Cookie Set-Cookie Benchmark for fair CP vs Bun vs Node comparison
import {
    CookieOptions,
    CookieSameSite,
    formatSetCookie,
} from "../../lib/web/app.cp";
import { WallClock } from "../../lib/time/clock.cp";

fn main() {
    const tokens: [4]string = [
        "tok_0123456789abcdef",
        "tok_fedcba9876543210",
        "tok_a1b2c3d4e5f60718",
        "tok_9988776655443322",
    ];
    let cbuf: [256]u8 = [0; 256];
    const opts = CookieOptions{
        path: "/",
        domain: "cp.dev",
        max_age: 86400,
        http_only: true,
        secure: true,
        same_site: CookieSameSite.Lax,
    };

    const n_cookie: usize = 2000000;
    let acc: usize = 0;
    const t0 = WallClock.monotonicNs();
    let i: usize = 0;
    while (i < n_cookie) : (i += 1) {
        if (n := formatSetCookie("session_id", tokens[i & 3], opts, cbuf[0..])) {
            acc += n + @as(usize, cbuf[15]);
        }
    }
    const dt_ns = WallClock.monotonicNs() - t0;
    const dt_safe: u64 = if (dt_ns > 0) @as(u64, @intCast(dt_ns)) else 1;
    const ops_per_sec: u64 = (@as(u64, n_cookie) * 1000000000) / dt_safe;
    @print("cookie_nofold_2M_ns={} ({} ops/s) acc={}", dt_safe, ops_per_sec, acc);
}
