import { test, expect_eq } from "cap:test";
import {
    floorCharBoundary,
    truncateBytes,
    countCodepoints,
    truncateChars,
    truncateWithEllipsis,
} from "../../packages/cli-truncate/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/trunc-utf8 — floorCharBoundary never splits 3-byte CJK", () => {
        // "你好世界" = 12 bytes (3 bytes each)
        let zh: string = "你好世界";
        expect_eq(zh.len, 12);
        expect_eq(floorCharBoundary(zh, 5), 3);
        expect_eq(floorCharBoundary(zh, 6), 6);
        expect_eq(floorCharBoundary(zh, 2), 0);
        expect_eq(eqStr(truncateBytes(zh, 7), "你好"), true);
    });

    test("2. @cap/trunc-utf8 — never splits 4-byte Emoji", () => {
        let em: string = "A🚀B"; // 1 + 4 + 1 = 6 bytes
        expect_eq(countCodepoints(em), 3);
        expect_eq(floorCharBoundary(em, 3), 1);
        expect_eq(floorCharBoundary(em, 5), 5);
        expect_eq(eqStr(truncateBytes(em, 4), "A"), true);
        expect_eq(eqStr(truncateBytes(em, 5), "A🚀"), true);
    });

    test("3. @cap/trunc-utf8 — truncateChars by codepoint count", () => {
        expect_eq(eqStr(truncateChars("你好世界", 2), "你好"), true);
        expect_eq(eqStr(truncateChars("A🚀B", 2), "A🚀"), true);
        expect_eq(eqStr(truncateChars("hello", 10), "hello"), true);
        expect_eq(truncateChars("hello", 0).len, 0);
    });

    test("4. @cap/trunc-utf8 — truncateWithEllipsis ASCII & CJK", () => {
        let buf: [64]u8 = undefined;
        let n1: usize = truncateWithEllipsis("Hello, World!", 8, "...", buf[0..]);
        expect_eq(eqStr(buf[0..n1], "Hello..."), true);

        // "你好世界" (12B), max 10B, "..." (3B) => budget 7B => "你好" (6B) + "..." (3B) = 9B
        let n2: usize = truncateWithEllipsis("你好世界", 10, "...", buf[0..]);
        expect_eq(eqStr(buf[0..n2], "你好..."), true);
    });

    test("5. @cap/trunc-utf8 — no-op when input fits within max_bytes", () => {
        let buf: [32]u8 = undefined;
        let n: usize = truncateWithEllipsis("short", 10, "...", buf[0..]);
        expect_eq(eqStr(buf[0..n], "short"), true);
    });

    test("6. @cap/trunc-utf8 — edge cases & tiny buffer protection", () => {
        let buf: [16]u8 = undefined;
        let n1: usize = truncateWithEllipsis("abcdef", 2, "...", buf[0..]);
        expect_eq(eqStr(buf[0..n1], ".."), true);

        let tiny: [2]u8 = undefined;
        expect_eq(truncateWithEllipsis("abcdef", 5, "...", tiny[0..]), 0);
    });
}
