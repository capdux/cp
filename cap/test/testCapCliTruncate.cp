import { test, expect_eq } from "cap:test";
import { stringWidth } from "../../packages/@cap/string-width/src/index.cp";
import {
    TruncatePosition,
    TruncateOptions,
    cliTruncateInto,
    cliTruncateEndInto,
    cliTruncateStartInto,
    cliTruncateMiddleInto,
} from "../../packages/@cap/cli-truncate/src/index.cp";

fn ctTestEq(actual: []const u8, expected: []const u8): bool {
    if (actual.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < actual.len) {
        if (actual[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn ctTestAppend(dst: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < dst.len) {
        dst[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn ctTestAppendEsc(dst: []u8, pos: usize, suffix: string): usize {
    let p: usize = pos;
    if (p < dst.len) {
        dst[p] = 27;
        p += 1;
    }
    return ctTestAppend(dst, p, suffix);
}

fn main() {
    test("1. @cap/cli-truncate — No-op fit, zero columns, and tiny column bounds", () => {
        let out: [128]u8 = undefined;

        // Fits within columns -> unchanged
        const n1: usize = cliTruncateEndInto("unicorn", 10, &out);
        expect_eq(ctTestEq(out[0..n1], "unicorn"), true);

        const n2: usize = cliTruncateEndInto("unicorn", 7, &out);
        expect_eq(ctTestEq(out[0..n2], "unicorn"), true);

        // columns == 0 -> empty
        const n3: usize = cliTruncateEndInto("unicorn", 0, &out);
        expect_eq(n3, 0);

        // columns <= ellipsis width ("...") -> sliced ellipsis
        const n4: usize = cliTruncateEndInto("unicorn", 3, &out);
        expect_eq(ctTestEq(out[0..n4], "..."), true);

        const n5: usize = cliTruncateEndInto("unicorn", 2, &out);
        expect_eq(ctTestEq(out[0..n5], ".."), true);

        // Empty text
        const n6: usize = cliTruncateEndInto("", 5, &out);
        expect_eq(n6, 0);
    });

    test("2. @cap/cli-truncate — END, START, and MIDDLE truncation positions", () => {
        let out: [128]u8 = undefined;
        const text: string = "abcdefghij"; // 10 cols

        // END: available = 7 - 3 = 4 -> "abcd..."
        const n1: usize = cliTruncateEndInto(text, 7, &out);
        expect_eq(ctTestEq(out[0..n1], "abcd..."), true);
        expect_eq(stringWidth(out[0..n1]), 7);

        // START: available = 7 - 3 = 4 -> "...ghij"
        const n2: usize = cliTruncateStartInto(text, 7, &out);
        expect_eq(ctTestEq(out[0..n2], "...ghij"), true);
        expect_eq(stringWidth(out[0..n2]), 7);

        // MIDDLE: available = 7 - 3 = 4 -> left=2, right=2 -> "ab...ij"
        const n3: usize = cliTruncateMiddleInto(text, 7, &out);
        expect_eq(ctTestEq(out[0..n3], "ab...ij"), true);
        expect_eq(stringWidth(out[0..n3]), 7);

        // MIDDLE odd available: columns=8, available=5 -> left=3, right=2 -> "abc...ij"
        const n4: usize = cliTruncateMiddleInto(text, 8, &out);
        expect_eq(ctTestEq(out[0..n4], "abc...ij"), true);
        expect_eq(stringWidth(out[0..n4]), 8);
    });

    test("3. @cap/cli-truncate — Custom ellipsis (Unicode '…' and empty string)", () => {
        let out: [128]u8 = undefined;
        const text: string = "unicorns"; // 8 cols

        const opts_uni: TruncateOptions = TruncateOptions{
            position: TruncatePosition.END,
            prefer_truncation_on_space: false,
            ellipsis: "…",
        };
        const n1: usize = cliTruncateInto(text, 5, opts_uni, &out);
        expect_eq(ctTestEq(out[0..n1], "unic…"), true);
        expect_eq(stringWidth(out[0..n1]), 5);

        const opts_empty: TruncateOptions = TruncateOptions{
            position: TruncatePosition.END,
            prefer_truncation_on_space: false,
            ellipsis: "",
        };
        const n2: usize = cliTruncateInto(text, 4, opts_empty, &out);
        expect_eq(ctTestEq(out[0..n2], "unic"), true);
        expect_eq(stringWidth(out[0..n2]), 4);
    });

    test("4. @cap/cli-truncate — ANSI SGR preservation & auto-reset across END / START / MIDDLE", () => {
        // Build: \x1b[31mHello World!\x1b[39m (12 visible cols)
        let src: [64]u8 = undefined;
        let sp: usize = 0;
        sp = ctTestAppendEsc(&src, sp, "[31m");
        sp = ctTestAppend(&src, sp, "Hello World!");
        sp = ctTestAppendEsc(&src, sp, "[39m");

        let out: [128]u8 = undefined;

        // END to 8 cols: 5 cols ("Hello") wrapped in \x1b[31m...\x1b[39m + "..."
        const n1: usize = cliTruncateEndInto(src[0..sp], 8, &out);
        let exp1: [64]u8 = undefined;
        let ep1: usize = 0;
        ep1 = ctTestAppendEsc(&exp1, ep1, "[31m");
        ep1 = ctTestAppend(&exp1, ep1, "Hello");
        ep1 = ctTestAppendEsc(&exp1, ep1, "[39m");
        ep1 = ctTestAppend(&exp1, ep1, "...");
        expect_eq(ctTestEq(out[0..n1], exp1[0..ep1]), true);
        expect_eq(stringWidth(out[0..n1]), 8);

        // START to 9 cols: "..." + 6 cols ("World!") wrapped in \x1b[31m...\x1b[39m
        const n2: usize = cliTruncateStartInto(src[0..sp], 9, &out);
        let exp2: [64]u8 = undefined;
        let ep2: usize = 0;
        ep2 = ctTestAppend(&exp2, ep2, "...");
        ep2 = ctTestAppendEsc(&exp2, ep2, "[31m");
        ep2 = ctTestAppend(&exp2, ep2, "World!");
        ep2 = ctTestAppendEsc(&exp2, ep2, "[39m");
        expect_eq(ctTestEq(out[0..n2], exp2[0..ep2]), true);
        expect_eq(stringWidth(out[0..n2]), 9);

        // MIDDLE to 7 cols: left=2 ("He"), "...", right=2 ("d!")
        const n3: usize = cliTruncateMiddleInto(src[0..sp], 7, &out);
        expect_eq(stringWidth(out[0..n3]), 7);
    });

    test("5. @cap/cli-truncate — prefer_truncation_on_space for END, START, and MIDDLE", () => {
        let out: [128]u8 = undefined;
        const text: string = "the quick brown fox"; // 19 cols

        // END with space preference: columns=14 -> available=11 ("the quick b") -> breaks at space col 9 -> "the quick..."
        const opts_end: TruncateOptions = TruncateOptions{
            position: TruncatePosition.END,
            prefer_truncation_on_space: true,
            ellipsis: "...",
        };
        const n1: usize = cliTruncateInto(text, 14, opts_end, &out);
        expect_eq(ctTestEq(out[0..n1], "the quick..."), true);

        // START with space preference: columns=12 -> available=9, cut_start=10 ('b' in "brown"), col 9 IS space -> "...brown fox"
        const opts_start: TruncateOptions = TruncateOptions{
            position: TruncatePosition.START,
            prefer_truncation_on_space: true,
            ellipsis: "...",
        };
        const n2: usize = cliTruncateInto(text, 12, opts_start, &out);
        expect_eq(ctTestEq(out[0..n2], "...brown fox"), true);

        // START mid-word: columns=11 -> available=8, cut_start=11 ('r' in "brown") -> skips to next word "fox" -> "...fox"
        const n3: usize = cliTruncateInto(text, 11, opts_start, &out);
        expect_eq(ctTestEq(out[0..n3], "...fox"), true);

        // MIDDLE with space preference: columns=13 -> available=10 -> left=5 ("the q" -> "the"), right_start=14 ('n' -> "fox")
        const opts_mid: TruncateOptions = TruncateOptions{
            position: TruncatePosition.MIDDLE,
            prefer_truncation_on_space: true,
            ellipsis: "...",
        };
        const n4: usize = cliTruncateInto(text, 13, opts_mid, &out);
        expect_eq(ctTestEq(out[0..n4], "the...fox"), true);
    });

    test("6. @cap/cli-truncate — CJK double-width boundary safety", () => {
        // "你好世界CP" = 2 + 2 + 2 + 2 + 1 + 1 = 10 cols
        const text: string = "你好世界CP";
        expect_eq(stringWidth(text), 10);

        let out: [128]u8 = undefined;
        // END to 8 cols with "..." (3 cols) -> available=5 -> fits "你好" (4 cols), '世' doesn't fit -> "你好..." (width 7 <= 8)
        const n1: usize = cliTruncateEndInto(text, 8, &out);
        expect_eq(ctTestEq(out[0..n1], "你好..."), true);
        expect_eq(stringWidth(out[0..n1]) <= 8, true);

        // END to 7 cols with "…" (1 col) -> available=6 -> fits "你好世" (6 cols) -> "你好世…" (width 7)
        const opts_cjk: TruncateOptions = TruncateOptions{
            position: TruncatePosition.END,
            prefer_truncation_on_space: false,
            ellipsis: "…",
        };
        const n2: usize = cliTruncateInto(text, 7, opts_cjk, &out);
        expect_eq(ctTestEq(out[0..n2], "你好世…"), true);
        expect_eq(stringWidth(out[0..n2]), 7);
    });
}
