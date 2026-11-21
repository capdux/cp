import {
    RangeSpecKind,
    ByteRangeIter,
    resolveSingleByteRange,
} from "cap:http";
import { test, expect } from "cap:test";

test("ByteRangeIter parses bounded, open-ended, and suffix ranges", () => {
    let it = ByteRangeIter.init("BYTES=0-499, 500-999, 9500-, -500").?;

    const r1 = it.next().?;
    expect(r1.kind == RangeSpecKind.bounded);
    const res1 = r1.resolve(10000).?;
    expect(res1.start == 0 && res1.end == 499 && res1.length() == 500);

    const r2 = it.next().?;
    const res2 = r2.resolve(10000).?;
    expect(res2.start == 500 && res2.end == 999);

    const r3 = it.next().?;
    expect(r3.kind == RangeSpecKind.open_end);
    const res3 = r3.resolve(10000).?;
    expect(res3.start == 9500 && res3.end == 9999 && res3.length() == 500);

    const r4 = it.next().?;
    expect(r4.kind == RangeSpecKind.suffix);
    const res4 = r4.resolve(10000).?;
    expect(res4.start == 9500 && res4.end == 9999);

    // Suffix larger than entity clamps to full entity
    const res4_small = r4.resolve(200).?;
    expect(res4_small.start == 0 && res4_small.end == 199);

    expect(it.next() == null);
    expect(!it.hasError());
});

test("resolveSingleByteRange and error / 416 handling", () => {
    // End beyond entity_len clamps to entity_len - 1
    const clamped = resolveSingleByteRange("bytes=100-9999", 500).?;
    expect(clamped.start == 100 && clamped.end == 499 && clamped.length() == 400);

    // Unsatisfiable (start >= entity_len or entity_len == 0 or suffix 0)
    expect(resolveSingleByteRange("bytes=500-600", 500) == null);
    expect(resolveSingleByteRange("bytes=0-10", 0) == null);
    expect(resolveSingleByteRange("bytes=-0", 500) == null);

    // Malformed: non-bytes unit, reversed range, multi-range
    expect(ByteRangeIter.init("items=0-10") == null);
    expect(resolveSingleByteRange("bytes=500-100", 1000) == null);
    expect(resolveSingleByteRange("bytes=0-10, 20-30", 1000) == null);
});
