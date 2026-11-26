import { test, expect_eq } from "cap:test";
import {
    NO_DIFF_MESSAGE,
    DiffOptions,
    DiffSummary,
    diffStats,
    diffLinesUnified
} from "../../packages/@cap/jest-diff/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn containsSubstr(buf: []const u8, len: usize, needle: string): bool {
    if (needle.len == 0 || needle.len > len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= len) {
        let ok: bool = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (buf[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j = j + 1;
        }
        if (ok) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn main() {
    test("1. @cap/jest-diff — identical inputs emit NO_DIFF_MESSAGE and zero diffStats", () => {
        let s: DiffSummary = diffStats("alpha\nbeta", "alpha\nbeta");
        expect_eq(s.has_diff, false);
        expect_eq(s.equal, 2);
        expect_eq(s.insertions, 0);
        expect_eq(s.deletions, 0);

        let buf: [256]u8 = undefined;
        let n = diffLinesUnified("alpha\nbeta", "alpha\nbeta", DiffOptions.defaults(), buf[0..256]);
        expect_eq(eqBytes(buf[0..256], n, NO_DIFF_MESSAGE), true);
    });

    test("2. @cap/jest-diff — plain unified diff with default Expected/Received annotations", () => {
        let a = "line1\nold_line\nline3";
        let b = "line1\nnew_line\nline3";
        let buf: [512]u8 = undefined;
        let n = diffLinesUnified(a, b, DiffOptions.defaults(), buf[0..512]);
        let expected_out = "- Expected\n+ Received\n\n  line1\n- old_line\n+ new_line\n  line3\n";
        expect_eq(eqBytes(buf[0..512], n, expected_out), true);
    });

    test("3. @cap/jest-diff — include_change_counts appends -N / +M to header annotations", () => {
        let a = "a\nb\nc";
        let b = "a\nx\ny\nc";
        let stats = diffStats(a, b);
        expect_eq(stats.has_diff, true);
        expect_eq(stats.deletions, 1);
        expect_eq(stats.insertions, 2);
        expect_eq(stats.equal, 2);

        let opts = DiffOptions {
            a_annotation: "- Expected",
            b_annotation: "+ Received",
            context_lines: 5,
            expand: true,
            include_change_counts: true,
            use_color: false
        };
        let buf: [512]u8 = undefined;
        let n = diffLinesUnified(a, b, opts, buf[0..512]);
        expect_eq(containsSubstr(buf[0..512], n, "- Expected  - 1\n"), true);
        expect_eq(containsSubstr(buf[0..512], n, "+ Received  + 2\n"), true);
    });

    test("4. @cap/jest-diff — use_color applies red, green, and gray ANSI codes via @cap/picocolors", () => {
        let a = "same\nold";
        let b = "same\nnew";
        let buf: [512]u8 = undefined;
        let n = diffLinesUnified(a, b, DiffOptions.colored(), buf[0..512]);
        expect_eq(containsSubstr(buf[0..512], n, "\x1b[31m- old\x1b[39m\n"), true);
        expect_eq(containsSubstr(buf[0..512], n, "\x1b[32m+ new\x1b[39m\n"), true);
        expect_eq(containsSubstr(buf[0..512], n, "\x1b[90m  same\x1b[39m\n"), true);
    });

    test("5. @cap/jest-diff — context_lines folds distant equal lines unless expand is true", () => {
        let a = "1\n2\n3\n4\n5\nOLD\n7\n8\n9\n10\n11";
        let b = "1\n2\n3\n4\n5\nNEW\n7\n8\n9\n10\n11";

        let folded_opts = DiffOptions {
            a_annotation: "- Expected",
            b_annotation: "+ Received",
            context_lines: 1,
            expand: false,
            include_change_counts: false,
            use_color: false
        };
        let buf1: [1024]u8 = undefined;
        let n1 = diffLinesUnified(a, b, folded_opts, buf1[0..1024]);
        expect_eq(containsSubstr(buf1[0..1024], n1, "@@ ... @@\n"), true);
        expect_eq(containsSubstr(buf1[0..1024], n1, "  5\n"), true);
        expect_eq(containsSubstr(buf1[0..1024], n1, "  1\n"), false);

        let expanded_opts = DiffOptions {
            a_annotation: "- Expected",
            b_annotation: "+ Received",
            context_lines: 1,
            expand: true,
            include_change_counts: false,
            use_color: false
        };
        let buf2: [1024]u8 = undefined;
        let n2 = diffLinesUnified(a, b, expanded_opts, buf2[0..1024]);
        expect_eq(containsSubstr(buf2[0..1024], n2, "@@ ... @@\n"), false);
        expect_eq(containsSubstr(buf2[0..1024], n2, "  1\n"), true);
        expect_eq(containsSubstr(buf2[0..1024], n2, "  11\n"), true);
    });

    test("6. @cap/jest-diff — custom annotations and small buffer safe truncation", () => {
        let opts = DiffOptions {
            a_annotation: "- Snap",
            b_annotation: "+ Actual",
            context_lines: 2,
            expand: false,
            include_change_counts: false,
            use_color: false
        };
        let buf: [16]u8 = undefined;
        let n = diffLinesUnified("foo", "bar", opts, buf[0..16]);
        expect_eq(n, 16);
        expect_eq(containsSubstr(buf[0..16], n, "- Snap\n"), true);
    });
}
