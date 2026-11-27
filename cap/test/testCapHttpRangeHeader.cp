// cap/test/testCapHttpRangeHeader.cp
// Comprehensive unit tests for @cap/http-range-header

import { test, expect_eq } from "cap:test";
import {
    RangePosition,
    SyntacticByteRange,
    ValidatedByteRange,
    parseRangeHeader,
    validateRanges,
    formatContentRange,
    formatUnsatisfiableContentRange,
} from "../../packages/http-range-header/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/http-range-header — Parse single closed, open-ended & suffix ranges", () => {
        let syn: [4]SyntacticByteRange = undefined;

        const n1 = parseRangeHeader("bytes=0-499", syn[0..]);
        expect_eq(n1, 1 as usize);
        expect_eq(syn[0].kind == RangePosition.StartAndEnd, true);
        expect_eq(syn[0].start, 0 as u64);
        expect_eq(syn[0].end, 499 as u64);

        const n2 = parseRangeHeader("bytes=500-", syn[0..]);
        expect_eq(n2, 1 as usize);
        expect_eq(syn[0].kind == RangePosition.StartOnly, true);
        expect_eq(syn[0].start, 500 as u64);

        const n3 = parseRangeHeader("bytes=-250", syn[0..]);
        expect_eq(n3, 1 as usize);
        expect_eq(syn[0].kind == RangePosition.Suffix, true);
        expect_eq(syn[0].end, 250 as u64);
    });

    test("2. @cap/http-range-header — Parse multi-range comma list with OWS", () => {
        let syn: [8]SyntacticByteRange = undefined;
        const n = parseRangeHeader("  BYTES=0-99, 200-299 , 500- , -50  ", syn[0..]);
        expect_eq(n, 4 as usize);
        expect_eq(syn[0].kind == RangePosition.StartAndEnd, true);
        expect_eq(syn[0].end, 99 as u64);
        expect_eq(syn[1].start, 200 as u64);
        expect_eq(syn[1].end, 299 as u64);
        expect_eq(syn[2].kind == RangePosition.StartOnly, true);
        expect_eq(syn[2].start, 500 as u64);
        expect_eq(syn[3].kind == RangePosition.Suffix, true);
        expect_eq(syn[3].end, 50 as u64);
    });

    test("3. @cap/http-range-header — Reject non-bytes units & malformed headers", () => {
        let syn: [4]SyntacticByteRange = undefined;
        expect_eq(parseRangeHeader("", syn[0..]), 0 as usize);
        expect_eq(parseRangeHeader("items=0-10", syn[0..]), 0 as usize);
        expect_eq(parseRangeHeader("bytes=", syn[0..]), 0 as usize);
        expect_eq(parseRangeHeader("bytes=-", syn[0..]), 0 as usize);
        expect_eq(parseRangeHeader("bytes=10-5", syn[0..]), 0 as usize);
        expect_eq(parseRangeHeader("bytes=0-10,", syn[0..]), 0 as usize);
        expect_eq(parseRangeHeader("bytes=0-10, , 20-30", syn[0..]), 0 as usize);
        expect_eq(parseRangeHeader("bytes=abc-10", syn[0..]), 0 as usize);
    });

    test("4. @cap/http-range-header — validateRanges clamping, open-ended & suffix resolution", () => {
        let syn: [4]SyntacticByteRange = undefined;
        const parsed = parseRangeHeader("bytes=0-9999, 800-, -150, -5000", syn[0..]);
        expect_eq(parsed, 4 as usize);

        let valid: [4]ValidatedByteRange = undefined;
        const cnt = validateRanges(syn[0..parsed], 1000 as u64, valid[0..]);
        expect_eq(cnt, 4 as i64);

        // 0-9999 clamped to 0-999
        expect_eq(valid[0].start, 0 as u64);
        expect_eq(valid[0].end, 999 as u64);
        expect_eq(valid[0].byteLen(), 1000 as u64);

        // 800- -> 800-999
        expect_eq(valid[1].start, 800 as u64);
        expect_eq(valid[1].end, 999 as u64);
        expect_eq(valid[1].byteLen(), 200 as u64);

        // -150 -> 850-999
        expect_eq(valid[2].start, 850 as u64);
        expect_eq(valid[2].end, 999 as u64);

        // -5000 clamped to 0-999
        expect_eq(valid[3].start, 0 as u64);
        expect_eq(valid[3].end, 999 as u64);
    });

    test("5. @cap/http-range-header — Unsatisfiable ranges return -1 (HTTP 416)", () => {
        let syn: [4]SyntacticByteRange = undefined;
        let valid: [4]ValidatedByteRange = undefined;

        const n1 = parseRangeHeader("bytes=1000-2000", syn[0..]);
        expect_eq(validateRanges(syn[0..n1], 1000 as u64, valid[0..]), -1 as i64);

        const n2 = parseRangeHeader("bytes=-0", syn[0..]);
        expect_eq(validateRanges(syn[0..n2], 1000 as u64, valid[0..]), -1 as i64);

        const n3 = parseRangeHeader("bytes=0-10", syn[0..]);
        expect_eq(validateRanges(syn[0..n3], 0 as u64, valid[0..]), -1 as i64);
    });

    test("6. @cap/http-range-header — formatContentRange & formatUnsatisfiableContentRange", () => {
        let buf: [64]u8 = undefined;

        const n1 = formatContentRange(0 as u64, 499 as u64, 1000 as u64, buf[0..]);
        expect_eq(strEq(buf[0..n1], "bytes 0-499/1000"), true);

        const n2 = formatContentRange(500 as u64, 999 as u64, 1000 as u64, buf[0..]);
        expect_eq(strEq(buf[0..n2], "bytes 500-999/1000"), true);

        const n3 = formatUnsatisfiableContentRange(1000 as u64, buf[0..]);
        expect_eq(strEq(buf[0..n3], "bytes */1000"), true);
    });
}
