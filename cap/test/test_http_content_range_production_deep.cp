import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ContentRangeKind,
    ParsedContentRange as ContentRange,
    parseContentRange,
    formatContentRange,
} from "cap:http";

test("1. parseContentRange parses standard bytes range, unknown length, and unsatisfied 416 headers", () => {
    const r1 = parseContentRange("bytes 0-499/1234") ?? unreachable;
    expect_eq(r1.isBytes(), true);
    expect_eq(r1.kind == ContentRangeKind.range, true);
    expect_eq(r1.start, u64(0));
    expect_eq(r1.end, u64(499));
    expect_eq(r1.complete_length ?? unreachable, u64(1234));
    expect_eq(r1.sliceLength() ?? unreachable, u64(500));
    expect_eq(r1.isComplete(), false);

    // Complete resource coverage
    const r_full = parseContentRange("BYTES 0-1233/1234") ?? unreachable;
    expect_eq(r_full.isBytes(), true);
    expect_eq(r_full.sliceLength() ?? unreachable, u64(1234));
    expect_eq(r_full.isComplete(), true);

    // Unknown complete length "*"
    const r_unk = parseContentRange("bytes 100-199/*") ?? unreachable;
    expect_eq(r_unk.kind == ContentRangeKind.range, true);
    expect_eq(r_unk.complete_length == null, true);
    expect_eq(r_unk.sliceLength() ?? unreachable, u64(100));
    expect_eq(r_unk.isComplete(), false);

    // Unsatisfied range (416)
    const r_unsat = parseContentRange("bytes */1234") ?? unreachable;
    expect_eq(r_unsat.kind == ContentRangeKind.unsatisfied, true);
    expect_eq(r_unsat.complete_length ?? unreachable, u64(1234));
    expect_eq(r_unsat.sliceLength() == null, true);
    expect_eq(r_unsat.isComplete(), false);
});

test("2. parseContentRange supports custom units and zero-length unsatisfied 416", () => {
    const custom = parseContentRange("items 10-19/50") ?? unreachable;
    expect_eq(custom.isBytes(), false);
    expect_eq(std.mem.eql(u8, custom.unit, "items"), true);
    expect_eq(custom.sliceLength() ?? unreachable, u64(10));

    // Empty file 416 response: "bytes */0" is valid per RFC 9110
    const empty_file = parseContentRange("bytes */0") ?? unreachable;
    expect_eq(empty_file.kind == ContentRangeKind.unsatisfied, true);
    expect_eq(empty_file.complete_length ?? unreachable, u64(0));
});

test("3. formatContentRange round-trips all three valid RFC 9110 forms", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const f1 = formatContentRange(parseContentRange("bytes 0-499/1234") ?? unreachable, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, f1, "bytes 0-499/1234"), true);

    const f2 = formatContentRange(parseContentRange("bytes 500-999/*") ?? unreachable, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, f2, "bytes 500-999/*"), true);

    const f3 = formatContentRange(parseContentRange("bytes */1234") ?? unreachable, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, f3, "bytes */1234"), true);
});

test("4. parseContentRange rejects RFC-forbidden */*, inverted range, out-of-bounds end, overflow, and garbage", () => {
    // RFC 9110 forbids bytes */*
    expect_eq(parseContentRange("bytes */*") == null, true);

    // Inverted range start > end
    expect_eq(parseContentRange("bytes 500-499/1000") == null, true);

    // end >= complete_length
    expect_eq(parseContentRange("bytes 0-1000/1000") == null, true);
    expect_eq(parseContentRange("bytes 0-1001/1000") == null, true);
    expect_eq(parseContentRange("bytes 0-0/0") == null, true);

    // u64 overflow (> 18446744073709551615)
    expect_eq(parseContentRange("bytes 0-1/99999999999999999999") == null, true);

    // Extra spaces, trailing garbage, missing slash/dash
    expect_eq(parseContentRange("bytes  0-499/1234") == null, true);
    expect_eq(parseContentRange("bytes 0-499/1234 extra") == null, true);
    expect_eq(parseContentRange("bytes 0-499") == null, true);
    expect_eq(parseContentRange("bytes 0/1234") == null, true);
    expect_eq(parseContentRange("") == null, true);
});

test("5. formatContentRange rejects invalid states and tiny output buffers", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    // Unsatisfied with null complete_length (would emit forbidden */*)
    const bad_unsat = ContentRange{
        unit: "bytes",
        kind: ContentRangeKind.unsatisfied,
        start: 0,
        end: 0,
        complete_length: null,
    };
    expect_eq(formatContentRange(bad_unsat, &buf) == null, true);

    // Inverted range
    const bad_inv = ContentRange{
        unit: "bytes",
        kind: ContentRangeKind.range,
        start: 10,
        end: 5,
        complete_length: 100,
    };
    expect_eq(formatContentRange(bad_inv, &buf) == null, true);

    // end >= complete_length
    const bad_oob = ContentRange{
        unit: "bytes",
        kind: ContentRangeKind.range,
        start: 0,
        end: 100,
        complete_length: 100,
    };
    expect_eq(formatContentRange(bad_oob, &buf) == null, true);

    // Tiny buffer
    const valid = ContentRange{
        unit: "bytes",
        kind: ContentRangeKind.range,
        start: 0,
        end: 499,
        complete_length: 1234,
    };
    let tiny: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(formatContentRange(valid, &tiny) == null, true);
});
