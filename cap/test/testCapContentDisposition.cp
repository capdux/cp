// cap/test/testCapContentDisposition.cp
// Comprehensive unit tests for @cap/content-disposition (RFC 6266 / RFC 5987)

import { test, expect_eq } from "cap:test";
import {
    DISP_INLINE,
    DISP_ATTACHMENT,
    DISP_FORM_DATA,
    ParsedDisposition,
    format,
    parse,
} from "../../packages/@cap/content-disposition/src/index.cp";

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
    test("1. @cap/content-disposition — inline & attachment without filename", () => {
        let dst: [64]u8 = undefined;
        let out: [64]u8 = undefined;

        const n1: i64 = format(DISP_INLINE, "", dst[0..64]);
        expect_eq(sliceEq(dst[0..(n1 as usize)], "inline"), true);
        const p1: ParsedDisposition = parse(dst[0..(n1 as usize)], out[0..64]);
        expect_eq(p1.ok, true);
        expect_eq(p1.kind, DISP_INLINE);
        expect_eq(p1.filename_len, 0 as usize);

        const n2: i64 = format(DISP_ATTACHMENT, "", dst[0..64]);
        expect_eq(sliceEq(dst[0..(n2 as usize)], "attachment"), true);
        const p2: ParsedDisposition = parse(dst[0..(n2 as usize)], out[0..64]);
        expect_eq(p2.ok, true);
        expect_eq(p2.kind, DISP_ATTACHMENT);
        expect_eq(p2.filename_len, 0 as usize);
    });

    test("2. @cap/content-disposition — pure ASCII filename format & parse round-trip", () => {
        let dst: [128]u8 = undefined;
        const n: i64 = format(DISP_ATTACHMENT, "report-2026.pdf", dst[0..128]);
        expect_eq(n > 0, true);
        expect_eq(
            sliceEq(dst[0..(n as usize)], "attachment; filename=\"report-2026.pdf\""),
            true,
        );

        let out: [64]u8 = undefined;
        const p: ParsedDisposition = parse(dst[0..(n as usize)], out[0..64]);
        expect_eq(p.ok, true);
        expect_eq(p.kind, DISP_ATTACHMENT);
        expect_eq(p.is_utf8, false);
        expect_eq(sliceEq(out[0..p.filename_len], "report-2026.pdf"), true);
    });

    test("3. @cap/content-disposition — UTF-8 filename emits filename*=UTF-8'' & decodes cleanly", () => {
        const utf8_name = "报告.pdf";
        let dst: [256]u8 = undefined;
        const n: i64 = format(DISP_ATTACHMENT, utf8_name, dst[0..256]);
        expect_eq(n > 0, true);

        let out: [64]u8 = undefined;
        const p: ParsedDisposition = parse(dst[0..(n as usize)], out[0..64]);
        expect_eq(p.ok, true);
        expect_eq(p.kind, DISP_ATTACHMENT);
        expect_eq(p.is_utf8, true);
        expect_eq(sliceEq(out[0..p.filename_len], utf8_name), true);
    });

    test("4. @cap/content-disposition — strips directory path prefixes to basename", () => {
        let dst: [128]u8 = undefined;
        const n1: i64 = format(DISP_ATTACHMENT, "../../etc/passwd", dst[0..128]);
        expect_eq(sliceEq(dst[0..(n1 as usize)], "attachment; filename=\"passwd\""), true);

        const n2: i64 = format(DISP_ATTACHMENT, "C:\\Users\\demo\\secret.txt", dst[0..128]);
        expect_eq(sliceEq(dst[0..(n2 as usize)], "attachment; filename=\"secret.txt\""), true);
    });

    test("5. @cap/content-disposition — quoted escapes & unquoted token parsing", () => {
        let out: [64]u8 = undefined;
        const p1: ParsedDisposition = parse("attachment; filename=plain.txt", out[0..64]);
        expect_eq(p1.ok, true);
        expect_eq(p1.kind, DISP_ATTACHMENT);
        expect_eq(sliceEq(out[0..p1.filename_len], "plain.txt"), true);

        const p2: ParsedDisposition = parse("form-data; name=\"file\"; filename=\"a\\\"b;c.txt\"", out[0..64]);
        expect_eq(p2.ok, true);
        expect_eq(p2.kind, DISP_FORM_DATA);
        expect_eq(sliceEq(out[0..p2.filename_len], "a\"b;c.txt"), true);
    });

    test("6. @cap/content-disposition — rejects invalid headers & tiny buffers", () => {
        let dst: [8]u8 = undefined;
        expect_eq(format(99, "a.txt", dst[0..8]), -1 as i64);
        expect_eq(format(DISP_ATTACHMENT, "report.pdf", dst[0..8]), -1 as i64);

        let out: [4]u8 = undefined;
        expect_eq(parse("", out[0..4]).ok, false);
        expect_eq(parse("unknown; filename=\"a.txt\"", out[0..4]).ok, false);
        // Output buffer too small for filename
        expect_eq(parse("attachment; filename=\"toolong.pdf\"", out[0..4]).ok, false);
        // Malformed percent encoding
        expect_eq(parse("attachment; filename*=UTF-8''%ZZ", out[0..4]).ok, false);
    });
}
