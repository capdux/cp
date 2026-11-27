// cap/test/testCapMultipartBoundary.cp
// Comprehensive unit tests for @cap/multipart-boundary (RFC 7578)

import { test, expect_eq } from "cap:test";
import {
    MultipartPart,
    isValidBoundary,
    extractBoundary,
    nextPart,
    countParts,
} from "../../packages/busboy/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/multipart-boundary — extractBoundary bare & quoted values", () => {
        expect_eq(
            sliceEq(
                extractBoundary("multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW"),
                "----WebKitFormBoundary7MA4YWxkTrZu0gW",
            ),
            true,
        );
        expect_eq(
            sliceEq(
                extractBoundary("multipart/form-data; charset=utf-8; BOUNDARY=\"abc-123\""),
                "abc-123",
            ),
            true,
        );
        expect_eq(extractBoundary("application/json").len, 0 as usize);
    });

    test("2. @cap/multipart-boundary — single text field part parsing", () => {
        const body = "--bnd\r\nContent-Disposition: form-data; name=\"username\"\r\n\r\nalice\r\n--bnd--\r\n";
        const p: MultipartPart = nextPart(body, "bnd", 0);

        expect_eq(p.valid, true);
        expect_eq(sliceEq(p.name, "username"), true);
        expect_eq(p.filename.len, 0 as usize);
        expect_eq(sliceEq(p.body, "alice"), true);
        expect_eq(nextPart(body, "bnd", p.next_pos).valid, false);
    });

    test("3. @cap/multipart-boundary — multi-part mixed text & file upload", () => {
        const body = "--B1\r\nContent-Disposition: form-data; name=\"title\"\r\n\r\nReport\r\n--B1\r\nContent-Disposition: form-data; name=\"file\"; filename=\"a.txt\"\r\nContent-Type: text/plain\r\n\r\nhello file\r\n--B1--\r\n";

        const p1: MultipartPart = nextPart(body, "B1", 0);
        expect_eq(p1.valid, true);
        expect_eq(sliceEq(p1.name, "title"), true);
        expect_eq(sliceEq(p1.body, "Report"), true);

        const p2: MultipartPart = nextPart(body, "B1", p1.next_pos);
        expect_eq(p2.valid, true);
        expect_eq(sliceEq(p2.name, "file"), true);
        expect_eq(sliceEq(p2.filename, "a.txt"), true);
        expect_eq(sliceEq(p2.content_type, "text/plain"), true);
        expect_eq(sliceEq(p2.body, "hello file"), true);

        expect_eq(nextPart(body, "B1", p2.next_pos).valid, false);
    });

    test("4. @cap/multipart-boundary — countParts across multiple parts", () => {
        const body = "preamble\r\n--X\r\nContent-Disposition: form-data; name=\"a\"\r\n\r\n1\r\n--X\r\nContent-Disposition: form-data; name=\"b\"\r\n\r\n2\r\n--X\r\nContent-Disposition: form-data; name=\"c\"\r\n\r\n3\r\n--X--\r\n";
        expect_eq(countParts(body, "X"), 3 as usize);
    });

    test("5. @cap/multipart-boundary — rejects missing closing boundary & invalid boundary", () => {
        expect_eq(isValidBoundary("abc-123"), true);
        expect_eq(isValidBoundary(""), false);
        expect_eq(isValidBoundary("trailing "), false);
        expect_eq(isValidBoundary("bad;char"), false);

        // Missing closing boundary
        const truncated = "--B1\r\nContent-Disposition: form-data; name=\"a\"\r\n\r\nhello";
        expect_eq(nextPart(truncated, "B1", 0).valid, false);
        expect_eq(countParts(truncated, "B1"), 0 as usize);
    });

    test("6. @cap/multipart-boundary — empty body & CRLF inside body", () => {
        const body = "--Z\r\nContent-Disposition: form-data; name=\"empty\"\r\n\r\n\r\n--Z\r\nContent-Disposition: form-data; name=\"multi\"\r\n\r\nline1\r\nline2\r\n--Z--\r\n";

        const p1: MultipartPart = nextPart(body, "Z", 0);
        expect_eq(p1.valid, true);
        expect_eq(sliceEq(p1.name, "empty"), true);
        expect_eq(p1.body.len, 0 as usize);

        const p2: MultipartPart = nextPart(body, "Z", p1.next_pos);
        expect_eq(p2.valid, true);
        expect_eq(sliceEq(p2.name, "multi"), true);
        expect_eq(sliceEq(p2.body, "line1\r\nline2"), true);
    });
}
