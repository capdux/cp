// Source: cp/cap/test/test_byterange_production_deep.cp (Production deep tests for cap:byterange)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ByteRange,
    RangeStatus,
    RawRangeKind,
    parseRaw,
    parseRange,
    parseMultiRange,
    formatContentRange,
    formatUnsatisfiableContentRange,
    parseContentRange,
} from "cap:byterange";

test("1. Single-range bounded, open-ended, suffix, and clamping semantics (Bun RangeRequest parity)", () => {
    // Bounded: bytes=10-20 on total=100 -> [10, 20], len=11
    const r1 = parseRange("bytes=10-20", 100);
    expect_eq(r1.status == RangeStatus.Satisfiable, true);
    expect_eq(r1.range.start, 10);
    expect_eq(r1.range.end, 20);
    expect_eq(r1.range.length(), 11);

    // Case-insensitive "BYTES" + spaces around '=' and '-'
    const r2 = parseRange("BYTES \t = 50 - ", 100);
    expect_eq(r2.status == RangeStatus.Satisfiable, true);
    expect_eq(r2.range.start, 50);
    expect_eq(r2.range.end, 99);
    expect_eq(r2.range.length(), 50);

    // Clamping end >= total -> total - 1
    const r3 = parseRange("bytes=80-500", 100);
    expect_eq(r3.status == RangeStatus.Satisfiable, true);
    expect_eq(r3.range.start, 80);
    expect_eq(r3.range.end, 99);

    // Suffix: bytes=-25 on total=100 -> [75, 99]
    const r4 = parseRange("bytes=-25", 100);
    expect_eq(r4.status == RangeStatus.Satisfiable, true);
    expect_eq(r4.range.start, 75);
    expect_eq(r4.range.end, 99);

    // Suffix larger than total saturates start to 0
    const r5 = parseRange("bytes=-999", 100);
    expect_eq(r5.status == RangeStatus.Satisfiable, true);
    expect_eq(r5.range.start, 0);
    expect_eq(r5.range.end, 99);
});

test("2. Unsatisfiable (416) vs fallback-to-full (200 None) edge cases", () => {
    // start >= total -> 416 Unsatisfiable
    expect_eq(parseRange("bytes=100-120", 100).status == RangeStatus.Unsatisfiable, true);
    // suffix 0 -> 416 Unsatisfiable
    expect_eq(parseRange("bytes=-0", 100).status == RangeStatus.Unsatisfiable, true);
    // suffix on empty file (total == 0) -> None (serve 200 empty, matching Bun RangeRequest)
    expect_eq(parseRange("bytes=-10", 0).status == RangeStatus.None, true);
    // inverted bounds end < start -> None
    expect_eq(parseRange("bytes=50-20", 100).status == RangeStatus.None, true);
    // non-bytes unit -> None
    expect_eq(parseRange("items=0-10", 100).status == RangeStatus.None, true);
    // single-range parseRaw rejects multi-range (falls back to None)
    expect_eq(parseRaw("bytes=0-10, 20-30").kind == RawRangeKind.None, true);
});

test("3. RFC 9110 parseMultiRange parses comma-separated ranges zero-alloc", () => {
    let ranges: [4]ByteRange = std.mem.zeroes([4]ByteRange);
    let out: []ByteRange = ranges[0..];

    const res = parseMultiRange("bytes=0-9, 50-59, -10", 100, &out);
    expect_eq(res.status == RangeStatus.Satisfiable, true);
    expect_eq(res.count, 3);
    expect_eq(ranges[0].start == 0 && ranges[0].end == 9, true);
    expect_eq(ranges[1].start == 50 && ranges[1].end == 59, true);
    expect_eq(ranges[2].start == 90 && ranges[2].end == 99, true);

    // All ranges out of bounds -> Unsatisfiable
    const unsat = parseMultiRange("bytes=200-300, 400-500", 100, &out);
    expect_eq(unsat.status == RangeStatus.Unsatisfiable, true);
    expect_eq(unsat.count, 0);

    // Output buffer too small -> None
    let tiny: [1]ByteRange = std.mem.zeroes([1]ByteRange);
    let tiny_s: []ByteRange = tiny[0..];
    expect_eq(parseMultiRange("bytes=0-10, 20-30", 100, &tiny_s).status == RangeStatus.None, true);
});

test("4. Content-Range format & parse round-trip (206, 416, and unknown total '*')", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let out: []u8 = buf[0..];

    const r = ByteRange { start: 100, end: 299 };
    const n1 = formatContentRange(r, 1024, &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "bytes 100-299/1024"), true);

    const p1 = parseContentRange(buf[0..n1]) ?? unreachable;
    expect_eq(p1.is_unsatisfied, false);
    expect_eq(p1.start, 100);
    expect_eq(p1.end, 299);
    expect_eq(p1.total ?? unreachable, 1024);

    // 416 Content-Range: bytes */1024
    const n2 = formatUnsatisfiableContentRange(1024, &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], "bytes */1024"), true);
    const p2 = parseContentRange(buf[0..n2]) ?? unreachable;
    expect_eq(p2.is_unsatisfied, true);
    expect_eq(p2.total ?? unreachable, 1024);

    // Unknown total length: bytes 0-499/*
    const p3 = parseContentRange("bytes 0-499/*") ?? unreachable;
    expect_eq(p3.is_unsatisfied, false);
    expect_eq(p3.start, 0);
    expect_eq(p3.end, 499);
    expect_eq(p3.total == null, true);
});

test("5. Defensive guards: u64 overflow, invalid syntax, out-of-bounds Content-Range, small buffers", () => {
    // u64 overflow in range digits -> None
    expect_eq(parseRaw("bytes=99999999999999999999999-").kind == RawRangeKind.None, true);
    expect_eq(parseRaw("bytes=abc-def").kind == RawRangeKind.None, true);
    expect_eq(parseRaw("bytes=").kind == RawRangeKind.None, true);

    // Invalid Content-Range (end >= total, end < start, missing slash)
    expect_eq(parseContentRange("bytes 10-5/100") == null, true);
    expect_eq(parseContentRange("bytes 0-100/100") == null, true);
    expect_eq(parseContentRange("bytes 0-10") == null, true);
    expect_eq(parseContentRange("items 0-10/100") == null, true);

    // Format guards
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    let tiny_s: []u8 = tiny[0..];
    expect_eq(formatContentRange(ByteRange { start: 10, end: 5 }, 100, &tiny_s) == null, true);
    expect_eq(formatContentRange(ByteRange { start: 0, end: 100 }, 100, &tiny_s) == null, true);
    expect_eq(formatContentRange(ByteRange { start: 0, end: 10 }, 100, &tiny_s) == null, true);
    expect_eq(formatUnsatisfiableContentRange(100, &tiny_s) == null, true);
});
