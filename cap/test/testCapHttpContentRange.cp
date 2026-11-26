import { test, expect_eq } from "cap:test";
import {
    ContentRangeKind,
    ContentRange,
    parseContentRange,
    formatContentRangeHeader,
} from "../../packages/@cap/http-content-range/src/index.cp";

fn main() {
    test("1. @cap/http-content-range — parses standard bytes start-end/size", () => {
        const cr = parseContentRange("bytes 0-499/1234");
        expect_eq(cr.isValid(), true);
        expect_eq(cr.kind == .Bytes, true);
        expect_eq(cr.first_byte, 0 as u64);
        expect_eq(cr.last_byte, 499 as u64);
        expect_eq(cr.complete_length, 1234 as u64);
        expect_eq(cr.rangeLen(), 500 as u64);
    });

    test("2. @cap/http-content-range — parses unknown complete length bytes start-end/*", () => {
        const cr = parseContentRange("bytes 100-199/*");
        expect_eq(cr.kind == .BytesUnknownLength, true);
        expect_eq(cr.first_byte, 100 as u64);
        expect_eq(cr.last_byte, 199 as u64);
        expect_eq(cr.rangeLen(), 100 as u64);
    });

    test("3. @cap/http-content-range — parses unsatisfied range bytes */size", () => {
        const cr = parseContentRange("bytes */1234");
        expect_eq(cr.kind == .Unsatisfied, true);
        expect_eq(cr.complete_length, 1234 as u64);
        expect_eq(cr.rangeLen(), 0 as u64);
    });

    test("4. @cap/http-content-range — rejects invalid bounds, units, and bytes */*", () => {
        expect_eq(parseContentRange("bytes */*").isValid(), false);
        expect_eq(parseContentRange("bytes 500-400/1000").isValid(), false);
        expect_eq(parseContentRange("bytes 0-1000/1000").isValid(), false);
        expect_eq(parseContentRange("items 0-5/10").isValid(), false);
        expect_eq(parseContentRange("").isValid(), false);
    });

    test("5. @cap/http-content-range — formats all three valid variants roundtrip", () => {
        let buf: [64]u8 = undefined;
        const c1 = parseContentRange("bytes 42-1233/1234");
        let n1 = formatContentRangeHeader(c1, buf[0..64]);
        expect_eq(buf[0..n1], "bytes 42-1233/1234");

        const c2 = parseContentRange("bytes 0-99/*");
        let n2 = formatContentRangeHeader(c2, buf[0..64]);
        expect_eq(buf[0..n2], "bytes 0-99/*");

        const c3 = parseContentRange("bytes */0");
        let n3 = formatContentRangeHeader(c3, buf[0..64]);
        expect_eq(buf[0..n3], "bytes */0");
    });

    test("6. @cap/http-content-range — handles OWS padding and invalid formatting", () => {
        const cr = parseContentRange("  bytes 10-10/11  ");
        expect_eq(cr.isValid(), true);
        expect_eq(cr.rangeLen(), 1 as u64);

        let buf: [32]u8 = undefined;
        const inv = parseContentRange("garbage");
        expect_eq(formatContentRangeHeader(inv, buf[0..32]), 0 as usize);
    });
}
