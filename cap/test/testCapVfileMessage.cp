import { test, expect_eq } from "cap:test";
import {
    Severity,
    makePoint,
    makePosition,
    pointAtOffset,
    createMessage,
    stringifyPosition,
    formatMessage,
} from "../../packages/@cap/vfile-message/src/index.cp";

fn main() {
    test("1. @cap/vfile-message — formats point and range positions", () => {
        let buf: [64]u8 = [0; 64];
        const p1 = makePoint(3, 12, 40);
        const n1 = stringifyPosition(makePosition(p1, p1), buf[0..64]);
        expect_eq(buf[0..n1] as string, "3:12");

        const p2 = makePoint(3, 20, 48);
        const n2 = stringifyPosition(makePosition(p1, p2), buf[0..64]);
        expect_eq(buf[0..n2] as string, "3:12-3:20");
    });

    test("2. @cap/vfile-message — pointAtOffset computes 1-based line and column", () => {
        const src = "first\nsecond\nthird";
        const pt = pointAtOffset(src, 9); // 'o' in "second" (index 6='s',7='e',8='c',9='o')
        expect_eq(pt.line, 2 as u32);
        expect_eq(pt.column, 4 as u32);
        expect_eq(pt.offset, 9 as u32);
    });

    test("3. @cap/vfile-message — formatMessage renders full lint diagnostic", () => {
        const msg = createMessage(
            "Unexpected trailing space",
            "docs/guide.md",
            "remark-lint",
            "no-trailing-spaces",
            .Warning,
            makePoint(10, 5, 120),
        );
        expect_eq(msg.isFatal(), false);
        expect_eq(msg.hasPosition(), true);

        let buf: [256]u8 = [0; 256];
        const n = formatMessage(msg, buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "docs/guide.md:10:5: warning: Unexpected trailing space [remark-lint:no-trailing-spaces]",
        );
    });

    test("4. @cap/vfile-message — handles fatal errors and missing source/rule", () => {
        const msg = createMessage("Syntax error", "main.cp", "", "", .Error, makePoint(1, 1, 0));
        expect_eq(msg.isFatal(), true);
        let buf: [128]u8 = [0; 128];
        const n = formatMessage(msg, buf[0..128]);
        expect_eq(buf[0..n] as string, "main.cp:1:1: error: Syntax error");
    });

    test("5. @cap/vfile-message — handles unset position and empty file path", () => {
        const msg = createMessage("General note", "", "cap", "info", .Info, makePoint(0, 0, 0));
        expect_eq(msg.hasPosition(), false);
        let buf: [128]u8 = [0; 128];
        const n = formatMessage(msg, buf[0..128]);
        expect_eq(buf[0..n] as string, "info: General note [cap:info]");
    });

    test("6. @cap/vfile-message — pointAtOffset clamps out-of-bounds offset safely", () => {
        const pt = pointAtOffset("ab\ncd", 999);
        expect_eq(pt.line, 2 as u32);
        expect_eq(pt.column, 3 as u32);
        expect_eq(pt.offset, 5 as u32);
    });
}
