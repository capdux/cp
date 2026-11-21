// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/uptime Zero-Alloc Integer Parser)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ProcUptime,
    parseProcUptime,
} from "cap:sys";

test("1. parseProcUptime decodes integer, 1-digit, 2-digit, 3-digit, and 4+-digit fractional seconds to ms", () => {
    const u1 = parseProcUptime("12345.67 45678.90\n") ?? unreachable;
    expect_eq(u1.uptime_ms, 12345670);
    expect_eq(u1.idle_ms, 45678900);
    expect_eq(u1.uptimeSeconds(), 12345);
    expect_eq(u1.idleSeconds(), 45678);

    const u2 = parseProcUptime("  100 200.5\r\n") ?? unreachable;
    expect_eq(u2.uptime_ms, 100000);
    expect_eq(u2.idle_ms, 200500);

    const u3 = parseProcUptime("0.123999 0.0051") ?? unreachable;
    expect_eq(u3.uptime_ms, 123);
    expect_eq(u3.idle_ms, 5);
});

test("2. idleRatioPermille computes multi-core idle ratio in [0, 1000] safely without overflow", () => {
    // 4 cores, uptime 1000s, idle 3000s => 750 permille (75.0%)
    const u1 = parseProcUptime("1000.00 3000.00\n") ?? unreachable;
    expect_eq(u1.idleRatioPermille(4), 750);
    expect_eq(u1.idleRatioPermille(8), 375);

    // Clamped to 1000 when idle slightly exceeds uptime * cpu_count
    const u2 = parseProcUptime("100.00 405.50\n") ?? unreachable;
    expect_eq(u2.idleRatioPermille(4), 1000);

    // Guard zero cpu_count and zero uptime
    expect_eq(u1.idleRatioPermille(0), 0);
    const u_zero = parseProcUptime("0.00 0.00\n") ?? unreachable;
    expect_eq(u_zero.idleRatioPermille(4), 0);
});

test("3. formatHuman formats days, hours, minutes, and seconds cleanly", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    // 3*86400 + 4*3600 + 12*60 + 5 = 274325
    const u_days = parseProcUptime("274325.99 100.00\n") ?? unreachable;
    expect_eq(std.mem.eql(u8, u_days.formatHuman(&buf) ?? unreachable, "3d 04h 12m 05s"), true);

    // 2*3600 + 5*60 + 9 = 7509
    const u_hours = parseProcUptime("7509.00 0.00\n") ?? unreachable;
    expect_eq(std.mem.eql(u8, u_hours.formatHuman(&buf) ?? unreachable, "2h 05m 09s"), true);

    // 12*60 + 3 = 723
    const u_mins = parseProcUptime("723.00 0.00\n") ?? unreachable;
    expect_eq(std.mem.eql(u8, u_mins.formatHuman(&buf) ?? unreachable, "12m 03s"), true);

    const u_secs = parseProcUptime("45.00 0.00\n") ?? unreachable;
    expect_eq(std.mem.eql(u8, u_secs.formatHuman(&buf) ?? unreachable, "45s"), true);

    const u_zero = parseProcUptime("0.99 0.00\n") ?? unreachable;
    expect_eq(std.mem.eql(u8, u_zero.formatHuman(&buf) ?? unreachable, "0s"), true);
});

test("4. formatHuman returns null when output buffer is too small", () => {
    const u = parseProcUptime("274325.00 100.00\n") ?? unreachable;
    let tiny: [5]u8 = std.mem.zeroes([5]u8);
    expect_eq(u.formatHuman(&tiny) == null, true);

    // 精确长度匹配 "45s" (3 bytes)
    const u_s = parseProcUptime("45.00 0.00\n") ?? unreachable;
    let exact3: [3]u8 = std.mem.zeroes([3]u8);
    let exact2: [2]u8 = std.mem.zeroes([2]u8);
    expect_eq(std.mem.eql(u8, u_s.formatHuman(&exact3) ?? unreachable, "45s"), true);
    expect_eq(u_s.formatHuman(&exact2) == null, true);
});

test("5. Malformed inputs, missing fields, extra fields, negative values, and overflows return null", () => {
    expect_eq(parseProcUptime("") == null, true);
    expect_eq(parseProcUptime("   \n") == null, true);
    expect_eq(parseProcUptime("123.45\n") == null, true);
    expect_eq(parseProcUptime("123.45 678.90 extra\n") == null, true);
    expect_eq(parseProcUptime("-1.00 2.00\n") == null, true);
    expect_eq(parseProcUptime("12. 34.0\n") == null, true);
    expect_eq(parseProcUptime(".5 1.0\n") == null, true);
    expect_eq(parseProcUptime("1.2.3 4.5\n") == null, true);
    expect_eq(parseProcUptime("12a 34\n") == null, true);
    expect_eq(parseProcUptime("99999999999999999999.0 1.0\n") == null, true);
});
