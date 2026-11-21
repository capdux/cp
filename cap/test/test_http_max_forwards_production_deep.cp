import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MaxForwardsAction,
    parseMaxForwards,
    evaluateMaxForwards,
    formatMaxForwards,
} from "cap:http_max_forwards";

test("1. parseMaxForwards parses valid 1*DIGIT values with optional OWS and u32 bounds", () => {
    expect_eq(parseMaxForwards("0") ?? unreachable, u32(0));
    expect_eq(parseMaxForwards("1") ?? unreachable, u32(1));
    expect_eq(parseMaxForwards("42") ?? unreachable, u32(42));
    expect_eq(parseMaxForwards(" \t 10 \t ") ?? unreachable, u32(10));
    expect_eq(parseMaxForwards("007") ?? unreachable, u32(7));
    expect_eq(parseMaxForwards("4294967295") ?? unreachable, u32(4294967295));
});

test("2. parseMaxForwards rejects negative, signed, malformed, CRLF, and overflow inputs", () => {
    expect_eq(parseMaxForwards("") == null, true);
    expect_eq(parseMaxForwards("   \t ") == null, true);
    expect_eq(parseMaxForwards("-1") == null, true);
    expect_eq(parseMaxForwards("+5") == null, true);
    expect_eq(parseMaxForwards("1 2") == null, true);
    expect_eq(parseMaxForwards("12abc") == null, true);
    expect_eq(parseMaxForwards("5\r\n") == null, true);
    expect_eq(parseMaxForwards("4294967296") == null, true);
    expect_eq(parseMaxForwards("99999999999") == null, true);
});

test("3. evaluateMaxForwards applies hop limit strictly to TRACE and OPTIONS", () => {
    // TRACE with positive value decrements by 1
    const t1 = evaluateMaxForwards("TRACE", "3");
    expect_eq(t1.action == MaxForwardsAction.forward, true);
    expect_eq(t1.next_value, u32(2));

    // OPTIONS with value 1 decrements to 0 and forwards
    const o1 = evaluateMaxForwards("OPTIONS", " 1 ");
    expect_eq(o1.action == MaxForwardsAction.forward, true);
    expect_eq(o1.next_value, u32(0));

    // TRACE and OPTIONS with value 0 respond directly
    const t0 = evaluateMaxForwards("TRACE", "0");
    expect_eq(t0.action == MaxForwardsAction.respond_directly, true);
    expect_eq(t0.next_value, u32(0));

    const o0 = evaluateMaxForwards("OPTIONS", "00");
    expect_eq(o0.action == MaxForwardsAction.respond_directly, true);
    expect_eq(o0.next_value, u32(0));

    // Missing header forwards normally
    const t_none = evaluateMaxForwards("TRACE", null);
    expect_eq(t_none.action == MaxForwardsAction.forward, true);
    expect_eq(t_none.next_value, u32(0));
});

test("4. evaluateMaxForwards rejects malformed headers on TRACE/OPTIONS and ignores other methods", () => {
    // Malformed header on TRACE / OPTIONS returns bad_request
    const bad_t = evaluateMaxForwards("TRACE", "-1");
    expect_eq(bad_t.action == MaxForwardsAction.bad_request, true);

    const bad_o = evaluateMaxForwards("OPTIONS", "abc");
    expect_eq(bad_o.action == MaxForwardsAction.bad_request, true);

    const bad_overflow = evaluateMaxForwards("OPTIONS", "4294967296");
    expect_eq(bad_overflow.action == MaxForwardsAction.bad_request, true);

    // Non-TRACE/OPTIONS methods ignore Max-Forwards even when 0 or malformed
    const get_zero = evaluateMaxForwards("GET", "0");
    expect_eq(get_zero.action == MaxForwardsAction.forward, true);

    const post_bad = evaluateMaxForwards("POST", "not-a-number");
    expect_eq(post_bad.action == MaxForwardsAction.forward, true);

    // Lowercase method names are not TRACE/OPTIONS per RFC 9110 case sensitivity
    const lower_trace = evaluateMaxForwards("trace", "0");
    expect_eq(lower_trace.action == MaxForwardsAction.forward, true);
});

test("5. formatMaxForwards outputs canonical decimal without leading zeros and checks buffer size", () => {
    let buf: [16]u8 = std.mem.zeroes([16]u8);

    const s0 = formatMaxForwards(0, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s0, "0"), true);

    const s9 = formatMaxForwards(9, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s9, "9"), true);

    const s42 = formatMaxForwards(42, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s42, "42"), true);

    const smax = formatMaxForwards(4294967295, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, smax, "4294967295"), true);

    // Exact buffer fit succeeds
    let exact: [10]u8 = std.mem.zeroes([10]u8);
    const sexact = formatMaxForwards(4294967295, &exact) ?? unreachable;
    expect_eq(std.mem.eql(u8, sexact, "4294967295"), true);

    // Insufficient buffer returns null
    let tiny: [2]u8 = std.mem.zeroes([2]u8);
    expect_eq(formatMaxForwards(100, &tiny) == null, true);
    let empty_buf: [0]u8 = std.mem.zeroes([0]u8);
    expect_eq(formatMaxForwards(0, &empty_buf) == null, true);
});
