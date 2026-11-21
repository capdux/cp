import std from "std";
import { test, expect_eq } from "cap:test";
import { WarningIter, isStaleWarning, formatWarning } from "cap:http";

test("1. WarningIter parses multi-warning headers with quoted commas and HTTP-dates", () => {
    const hdr =
        "110 - \"Response is Stale\", "
        ++ "112 edge.cp.dev:8080 \"Disconnected, \\\"offline\\\"\" \"Wed, 21 Oct 2015 07:28:00 GMT\", "
        ++ "299 proxy \"Misc warning\"";

    let it = WarningIter.init(hdr);

    const w0 = it.next() ?? unreachable;
    expect_eq(w0.code, u16(110));
    expect_eq(std.mem.eql(u8, w0.agent, "-"), true);
    expect_eq(std.mem.eql(u8, w0.text, "Response is Stale"), true);
    expect_eq(w0.date == null, true);

    const w1 = it.next() ?? unreachable;
    expect_eq(w1.code, u16(112));
    expect_eq(std.mem.eql(u8, w1.agent, "edge.cp.dev:8080"), true);
    expect_eq(std.mem.eql(u8, w1.text, "Disconnected, \\\"offline\\\""), true);
    expect_eq(std.mem.eql(u8, w1.date ?? unreachable, "Wed, 21 Oct 2015 07:28:00 GMT"), true);

    const w2 = it.next() ?? unreachable;
    expect_eq(w2.code, u16(299));
    expect_eq(std.mem.eql(u8, w2.agent, "proxy"), true);
    expect_eq(std.mem.eql(u8, w2.text, "Misc warning"), true);
    expect_eq(w2.date == null, true);

    expect_eq(it.next() == null, true);
});

test("2. isStaleWarning classifies 110..113 vs non-stale codes", () => {
    expect_eq(isStaleWarning(110), true);
    expect_eq(isStaleWarning(111), true);
    expect_eq(isStaleWarning(112), true);
    expect_eq(isStaleWarning(113), true);

    expect_eq(isStaleWarning(199), false);
    expect_eq(isStaleWarning(214), false);
    expect_eq(isStaleWarning(299), false);
});

test("3. formatWarning round-trips with WarningIter with and without date", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const f1 = formatWarning(110, "-", "Response is Stale", null, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, f1, "110 - \"Response is Stale\""), true);

    const f2 = formatWarning(214, "cdn.cp.dev", "Transformed \"img\"", "Wed, 21 Oct 2015 07:28:00 GMT", &buf) ?? unreachable;
    expect_eq(
        std.mem.eql(u8, f2, "214 cdn.cp.dev \"Transformed \\\"img\\\"\" \"Wed, 21 Oct 2015 07:28:00 GMT\""),
        true
    );

    let it = WarningIter.init(f2);
    const parsed = it.next() ?? unreachable;
    expect_eq(parsed.code, u16(214));
    expect_eq(std.mem.eql(u8, parsed.agent, "cdn.cp.dev"), true);
    expect_eq(std.mem.eql(u8, parsed.date ?? unreachable, "Wed, 21 Oct 2015 07:28:00 GMT"), true);
});

test("4. WarningIter skips malformed entries gracefully", () => {
    const hdr = "99 - \"bad code\", abc - \"nan\", 110 - unquoted, 199 ok \"Valid\"";
    let it = WarningIter.init(hdr);
    const w = it.next() ?? unreachable;
    expect_eq(w.code, u16(199));
    expect_eq(std.mem.eql(u8, w.agent, "ok"), true);
    expect_eq(std.mem.eql(u8, w.text, "Valid"), true);
    expect_eq(it.next() == null, true);
});

test("5. formatWarning rejects out-of-range codes, invalid agent, and tiny buffers", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    expect_eq(formatWarning(99, "-", "x", null, &buf) == null, true);
    expect_eq(formatWarning(1000, "-", "x", null, &buf) == null, true);
    expect_eq(formatWarning(110, "", "x", null, &buf) == null, true);
    expect_eq(formatWarning(110, "bad agent", "x", null, &buf) == null, true);

    let tiny: [8]u8 = std.mem.zeroes([8]u8);
    expect_eq(formatWarning(110, "proxy", "Response is Stale", null, &tiny) == null, true);
});
