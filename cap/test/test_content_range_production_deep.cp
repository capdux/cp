import { test, expect } from "cap:test";
import {
    ByteRange,
    RangeStatus,
    parseRangeHeader,
    formatContentRange,
    formatUnsatisfiedRange,
    parseContentRange,
} from "cap:content_range";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("content_range: parseRangeHeader closed, open-ended, suffix, multi-range, and 416", () => {
    const r1 = parseRangeHeader("bytes=0-499, 1000-, -100", 1200);
    expect(r1.status == RangeStatus.ok && r1.len == 3);
    expect(r1.ranges[0].start == 0 && r1.ranges[0].end == 499 && r1.ranges[0].length() == 500);
    expect(r1.ranges[1].start == 1000 && r1.ranges[1].end == 1199 && r1.ranges[1].length() == 200);
    expect(r1.ranges[2].start == 1100 && r1.ranges[2].end == 1199 && r1.ranges[2].length() == 100);

    // End clamped to total_size - 1
    const r2 = parseRangeHeader("bytes=900-5000", 1000);
    expect(r2.status == RangeStatus.ok && r2.first().?.end == 999);

    // Unsatisfiable (416)
    expect(parseRangeHeader("bytes=2000-3000", 1000).status == RangeStatus.unsatisfiable);
    expect(parseRangeHeader("bytes=-0", 1000).status == RangeStatus.unsatisfiable);

    // Invalid syntax
    expect(parseRangeHeader("bytes=500-100", 1000).status == RangeStatus.invalid);
    expect(parseRangeHeader("items=0-10", 1000).status == RangeStatus.invalid);
});

test("content_range: format and parse Content-Range headers", () => {
    let buf: [64]u8 = [0; 64];

    const h1 = formatContentRange(ByteRange{ start: 0, end: 499 }, 1234, buf[0..]).?;
    expect(strEq(h1, "bytes 0-499/1234"));
    const p1 = parseContentRange(h1).?;
    expect(p1.has_range && p1.range.start == 0 && p1.range.end == 499 && p1.has_total && p1.total_size == 1234);

    const h2 = formatUnsatisfiedRange(1234, buf[0..]).?;
    expect(strEq(h2, "bytes */1234"));
    const p2 = parseContentRange(h2).?;
    expect(!p2.has_range && p2.has_total && p2.total_size == 1234);

    expect(parseContentRange("bytes */*") == null);
    expect(parseContentRange("bytes 0-2000/1000") == null);
});
