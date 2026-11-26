// cap/test/testCapContentDisposition.cp
// Comprehensive unit tests for @cap/content-disposition (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    ContentDisposition,
    parseContentDisposition,
    formatContentDisposition,
    decodeFilenameStar,
    sanitizeFilename
} from "../../packages/@cap/content-disposition/src/index.cp";

fn main() {
    test("1. parseContentDisposition — attachment with filename and filename*", () => {
        const hdr = "attachment; filename=\"report.pdf\"; filename*=UTF-8''%E6%8A%A5%E5%91%8A.pdf";
        const cd: ContentDisposition = parseContentDisposition(hdr);
        expect_eq(cd.valid, true);
        expect_eq(cd.isAttachment(), true);
        expect_eq(cd.isInline(), false);
        expect_eq(cd.hasFilename(), true);
        expect_eq(cd.disp_type, "attachment");
        expect_eq(cd.filename, "report.pdf");
        expect_eq(cd.filename_star, "UTF-8''%E6%8A%A5%E5%91%8A.pdf");
    });

    test("2. parseContentDisposition — case-insensitive matching, inline & invalid headers", () => {
        const cd1 = parseContentDisposition("  INLINE ; FILENAME=avatar.png ; Size=1024 ");
        expect_eq(cd1.valid, true);
        expect_eq(cd1.isInline(), true);
        expect_eq(cd1.isAttachment(), false);
        expect_eq(cd1.filename, "avatar.png");

        const cd2 = parseContentDisposition("ATTACHMENT; FILENAME* = utf-8'zh'%E6%B5%8B%E8%AF%95.txt");
        expect_eq(cd2.valid, true);
        expect_eq(cd2.isAttachment(), true);
        expect_eq(cd2.filename_star, "utf-8'zh'%E6%B5%8B%E8%AF%95.txt");

        // Malformed headers must fail cleanly
        expect_eq(parseContentDisposition("").valid, false);
        expect_eq(parseContentDisposition("   ").valid, false);
        expect_eq(parseContentDisposition("attachment; filename=\"unterminated.pdf").valid, false);
        expect_eq(parseContentDisposition("attachment; brokenparam").valid, false);
    });

    test("3. formatContentDisposition — ASCII filename, default type & empty filename", () => {
        let out: [256]u8 = undefined;

        const n1 = formatContentDisposition("attachment", "report.pdf", out[0..256]);
        expect_eq(out[0..n1], "attachment; filename=\"report.pdf\"");

        const n2 = formatContentDisposition("inline", "photo.png", out[0..256]);
        expect_eq(out[0..n2], "inline; filename=\"photo.png\"");

        const n3 = formatContentDisposition("", "data.csv", out[0..256]);
        expect_eq(out[0..n3], "attachment; filename=\"data.csv\"");

        const n4 = formatContentDisposition("attachment", "", out[0..256]);
        expect_eq(out[0..n4], "attachment");
    });

    test("4. formatContentDisposition — UTF-8 non-ASCII filename auto-emits RFC 5987 filename*", () => {
        let out: [256]u8 = undefined;
        const n = formatContentDisposition("attachment", "报告.pdf", out[0..256]);
        const formatted = out[0..n];
        expect_eq(formatted, "attachment; filename=\"??.pdf\"; filename*=UTF-8''%E6%8A%A5%E5%91%8A.pdf");

        // Round-trip parse and decode
        const parsed = parseContentDisposition(formatted);
        expect_eq(parsed.valid, true);
        expect_eq(parsed.isAttachment(), true);
        expect_eq(parsed.filename, "??.pdf");
        expect_eq(parsed.filename_star, "UTF-8''%E6%8A%A5%E5%91%8A.pdf");

        let decoded: [128]u8 = undefined;
        const dn = decodeFilenameStar(parsed.filename_star, decoded[0..128]);
        expect_eq(decoded[0..dn], "报告.pdf");
    });

    test("5. decodeFilenameStar — UTF-8, language tag, ISO-8859-1 & malformed inputs", () => {
        let buf: [128]u8 = undefined;

        const n1 = decodeFilenameStar("UTF-8''%E6%8A%A5%E5%91%8A.pdf", buf[0..128]);
        expect_eq(buf[0..n1], "报告.pdf");

        const n2 = decodeFilenameStar("utf-8'en'hello%20world%21.txt", buf[0..128]);
        expect_eq(buf[0..n2], "hello world!.txt");

        const n3 = decodeFilenameStar("ISO-8859-1''%A3%20rates.pdf", buf[0..128]);
        expect_eq(buf[0..n3], "£ rates.pdf");

        // Invalid syntax returns 0
        expect_eq(decodeFilenameStar("no-quotes-here", buf[0..128]), 0 as usize);
        expect_eq(decodeFilenameStar("UTF-8'missing-second", buf[0..128]), 0 as usize);
        expect_eq(decodeFilenameStar("''empty-charset", buf[0..128]), 0 as usize);
    });

    test("6. sanitizeFilename — strips path traversal, control chars & rejects dot segments", () => {
        let buf: [128]u8 = undefined;

        const n1 = sanitizeFilename("../../etc/passwd", buf[0..128]);
        expect_eq(buf[0..n1], "passwd");

        const n2 = sanitizeFilename("C:\\Users\\admin\\Downloads\\invoice.pdf", buf[0..128]);
        expect_eq(buf[0..n2], "invoice.pdf");

        const n3 = sanitizeFilename("safe\r\nname\x00.txt", buf[0..128]);
        expect_eq(buf[0..n3], "safename.txt");

        expect_eq(sanitizeFilename("..", buf[0..128]), 0 as usize);
        expect_eq(sanitizeFilename(".", buf[0..128]), 0 as usize);
        expect_eq(sanitizeFilename("///", buf[0..128]), 0 as usize);
    });
}
