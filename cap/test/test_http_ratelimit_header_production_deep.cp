import std from "std";
import { test, expect_eq } from "cap:test";
import { RateLimitHeader, RateLimitPolicy } from "cap:http_ratelimit_header";

test("1. RateLimitHeader parses and formats IETF structured policy format", () => {
    const rl = RateLimitHeader.parse("\"default\";r=50;t=30") ?? unreachable;
    expect_eq(std.mem.eql(u8, rl.policy_name ?? unreachable, "default"), true);
    expect_eq(rl.remaining ?? unreachable, u64(50));
    expect_eq(rl.reset_sec ?? unreachable, u64(30));
    expect_eq(rl.limit == null, true);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const fmt = rl.format(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, fmt, "\"default\";r=50;t=30"), true);
});

test("2. RateLimitHeader parses and formats dictionary limit/remaining/reset format", () => {
    const rl = RateLimitHeader.parse("Limit=100, Remaining=42, Reset=15") ?? unreachable;
    expect_eq(rl.policy_name == null, true);
    expect_eq(rl.limit ?? unreachable, u64(100));
    expect_eq(rl.remaining ?? unreachable, u64(42));
    expect_eq(rl.reset_sec ?? unreachable, u64(15));

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const fmt = rl.format(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, fmt, "limit=100, remaining=42, reset=15"), true);

    // 空串与全无效字段拒绝
    expect_eq(RateLimitHeader.parse("") == null, true);
    expect_eq(RateLimitHeader.parse("foo=bar, baz=qux") == null, true);
    expect_eq(RateLimitHeader.parse("\"empty_params\"") == null, true);
});

test("3. RateLimitPolicy parses and formats both bare quota and named burst policy", () => {
    const p1 = RateLimitPolicy.parse("100;w=60") ?? unreachable;
    expect_eq(p1.policy_name == null, true);
    expect_eq(p1.quota, u64(100));
    expect_eq(p1.window_sec, u64(60));
    expect_eq(p1.comment == null, true);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    expect_eq(std.mem.eql(u8, p1.format(&buf) ?? unreachable, "100;w=60"), true);

    const p2 = RateLimitPolicy.parse("\"burst\";q=250;w=10;comment=\"per 10s\"") ?? unreachable;
    expect_eq(std.mem.eql(u8, p2.policy_name ?? unreachable, "burst"), true);
    expect_eq(p2.quota, u64(250));
    expect_eq(p2.window_sec, u64(10));
    expect_eq(std.mem.eql(u8, p2.comment ?? unreachable, "per 10s"), true);
    expect_eq(std.mem.eql(u8, p2.format(&buf) ?? unreachable, "\"burst\";q=250;w=10;comment=\"per 10s\""), true);
});

test("4. RateLimitPolicy rejects w=0, missing quota/window, and malformed values", () => {
    expect_eq(RateLimitPolicy.parse("") == null, true);
    expect_eq(RateLimitPolicy.parse("100;w=0") == null, true);
    expect_eq(RateLimitPolicy.parse("100") == null, true);
    expect_eq(RateLimitPolicy.parse("\"burst\";w=60") == null, true);
    expect_eq(RateLimitPolicy.parse("\"burst\";q=100") == null, true);
    expect_eq(RateLimitPolicy.parse("-10;w=60") == null, true);

    let buf: [64]u8 = std.mem.zeroes([64]u8);
    const bad_pol = RateLimitPolicy{ quota: 10, window_sec: 0 };
    expect_eq(bad_pol.format(&buf) == null, true);
});

test("5. buffer-too-small and empty-struct format guards", () => {
    const empty_rl = RateLimitHeader{};
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    expect_eq(empty_rl.format(&buf) == null, true);

    const rl = RateLimitHeader{ limit: 100, remaining: 50, reset_sec: 30 };
    const pol = RateLimitPolicy{ quota: 100, window_sec: 60, policy_name: "default" };
    let tiny: [6]u8 = std.mem.zeroes([6]u8);
    expect_eq(rl.format(&tiny) == null, true);
    expect_eq(pol.format(&tiny) == null, true);
});
