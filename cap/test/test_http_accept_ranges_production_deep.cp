import std from "std";
import { test, expect_eq } from "cap:test";
import {
    AcceptRangesKind,
    AcceptRangesIter,
    parseAcceptRanges,
    supportsByteRanges,
    supportsRangeUnit,
} from "cap:http";

test("1. parseAcceptRanges parses bytes, none, and custom units case-insensitively", () => {
    const b = parseAcceptRanges("bytes");
    expect_eq(b.kind == AcceptRangesKind.bytes, true);
    expect_eq(b.supports_bytes, true);
    expect_eq(b.has_custom, false);

    const b_upper = parseAcceptRanges(" \t BYTES \t ");
    expect_eq(b_upper.kind == AcceptRangesKind.bytes, true);
    expect_eq(b_upper.supports_bytes, true);
    expect_eq(b_upper.has_custom, false);

    const n = parseAcceptRanges("None");
    expect_eq(n.kind == AcceptRangesKind.none, true);
    expect_eq(n.supports_bytes, false);
    expect_eq(n.has_custom, false);

    const c = parseAcceptRanges("items, records");
    expect_eq(c.kind == AcceptRangesKind.custom, true);
    expect_eq(c.supports_bytes, false);
    expect_eq(c.has_custom, true);

    const mixed = parseAcceptRanges("items, Bytes, pages");
    expect_eq(mixed.kind == AcceptRangesKind.bytes, true);
    expect_eq(mixed.supports_bytes, true);
    expect_eq(mixed.has_custom, true);
});

test("2. parseAcceptRanges rejects none-mixed, empty, comma-only, and malformed headers", () => {
    // none mixed with other units or repeated is invalid per RFC 9110 §14.3
    expect_eq(parseAcceptRanges("none, bytes").kind == AcceptRangesKind.invalid, true);
    expect_eq(parseAcceptRanges("bytes, NONE").kind == AcceptRangesKind.invalid, true);
    expect_eq(parseAcceptRanges("none, items").kind == AcceptRangesKind.invalid, true);
    expect_eq(parseAcceptRanges("none, none").kind == AcceptRangesKind.invalid, true);

    // Empty / whitespace / comma-only headers are invalid
    expect_eq(parseAcceptRanges("").kind == AcceptRangesKind.invalid, true);
    expect_eq(parseAcceptRanges("  \t ").kind == AcceptRangesKind.invalid, true);
    expect_eq(parseAcceptRanges(",,, ,").kind == AcceptRangesKind.invalid, true);

    // Malformed tokens and CRLF injection attempts are invalid
    expect_eq(parseAcceptRanges("bytes, bad unit").kind == AcceptRangesKind.invalid, true);
    expect_eq(parseAcceptRanges("bytes\r\n").kind == AcceptRangesKind.invalid, true);
    expect_eq(parseAcceptRanges("\"bytes\"").kind == AcceptRangesKind.invalid, true);
});

test("3. AcceptRangesIter iterates valid RFC 9110 range-unit tokens and skips empty or malformed segments", () => {
    const hdr = " , bytes , \t items , , bad unit , \"quoted\" , pages , ";
    let it = AcceptRangesIter.init(hdr);

    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "bytes"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "items"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "pages"), true);
    expect_eq(it.next() == null, true);

    let empty_it = AcceptRangesIter.init(" , , \t ");
    expect_eq(empty_it.next() == null, true);
});

test("4. supportsByteRanges returns true only for valid headers containing bytes", () => {
    expect_eq(supportsByteRanges("bytes"), true);
    expect_eq(supportsByteRanges("BYTES"), true);
    expect_eq(supportsByteRanges("items, bytes"), true);
    expect_eq(supportsByteRanges(" , bytes , "), true);

    expect_eq(supportsByteRanges("none"), false);
    expect_eq(supportsByteRanges("items"), false);
    expect_eq(supportsByteRanges("none, bytes"), false);
    expect_eq(supportsByteRanges("bytes, bad unit"), false);
    expect_eq(supportsByteRanges(""), false);
});

test("5. supportsRangeUnit matches case-insensitively and rejects none, invalid headers, and malformed queries", () => {
    const hdr = "bytes, items, x-chunks";

    expect_eq(supportsRangeUnit(hdr, "bytes"), true);
    expect_eq(supportsRangeUnit(hdr, "BYTES"), true);
    expect_eq(supportsRangeUnit(hdr, "Items"), true);
    expect_eq(supportsRangeUnit(hdr, "X-CHUNKS"), true);
    expect_eq(supportsRangeUnit(hdr, "pages"), false);

    // "none" is never a supported range unit
    expect_eq(supportsRangeUnit("none", "none"), false);
    expect_eq(supportsRangeUnit("none", "bytes"), false);
    expect_eq(supportsRangeUnit("none, items", "items"), false);

    // Invalid headers and invalid query names return false
    expect_eq(supportsRangeUnit("items, bad unit", "items"), false);
    expect_eq(supportsRangeUnit(hdr, ""), false);
    expect_eq(supportsRangeUnit(hdr, "items "), false);
    expect_eq(supportsRangeUnit(hdr, "bad unit"), false);
});
