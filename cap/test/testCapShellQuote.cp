import { test, expect_eq } from "cap:test";
import {
    needsQuoting,
    quoteArg,
    quoteArgs,
    splitCommand,
    ParsedArgSpan,
} from "../../packages/shell-quote/src/index.cp";

fn eqBytes(a: []const u8, b: []const u8): bool {
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
    test("safe bare tokens need no quoting", () => {
        let buf: [64]u8 = undefined;
        expect_eq(needsQuoting("hello-world_1.0/a:b"), false);
        expect_eq(needsQuoting("hello world"), true);
        expect_eq(needsQuoting(""), true);
        let n = quoteArg("cap", buf[0..64]);
        expect_eq(n, 3);
        expect_eq(eqBytes(buf[0..3], "cap"), true);
    });

    test("quoteArg escapes spaces, metacharacters, and single quotes safely", () => {
        let buf: [128]u8 = undefined;
        let n1 = quoteArg("", buf[0..128]);
        expect_eq(eqBytes(buf[0..(n1 as usize)], "''"), true);

        let n2 = quoteArg("a b; rm -rf /", buf[0..128]);
        expect_eq(eqBytes(buf[0..(n2 as usize)], "'a b; rm -rf /'"), true);

        let n3 = quoteArg("it's", buf[0..128]);
        expect_eq(eqBytes(buf[0..(n3 as usize)], "'it'\\''s'"), true);
    });

    test("quoteArgs joins full argv command line", () => {
        const argv: [3][]const u8 = ["grep", "-E", "foo bar|baz"];
        let buf: [128]u8 = undefined;
        let n = quoteArgs(argv[0..3], buf[0..128]);
        expect_eq(n > 0, true);
        expect_eq(eqBytes(buf[0..(n as usize)], "grep -E 'foo bar|baz'"), true);
    });

    test("splitCommand parses single, double, and backslash escapes", () => {
        let out_buf: [256]u8 = undefined;
        let spans: [8]ParsedArgSpan = undefined;
        let res = splitCommand("git commit -m \"feat: add \\\"quote\\\"\" 'it'\\''s' a\\ b", out_buf[0..256], spans[0..8]);
        expect_eq(res.ok, true);
        expect_eq(res.arg_count, 6);
        expect_eq(eqBytes(out_buf[spans[0].start .. spans[0].start + spans[0].len], "git"), true);
        expect_eq(eqBytes(out_buf[spans[3].start .. spans[3].start + spans[3].len], "feat: add \"quote\""), true);
        expect_eq(eqBytes(out_buf[spans[4].start .. spans[4].start + spans[4].len], "it's"), true);
        expect_eq(eqBytes(out_buf[spans[5].start .. spans[5].start + spans[5].len], "a b"), true);
    });

    test("quoteArgs -> splitCommand roundtrip preserves exact argv", () => {
        const argv: [3][]const u8 = ["echo", "hello 'world' $HOME", ""];
        let cmd_buf: [256]u8 = undefined;
        let clen = quoteArgs(argv[0..3], cmd_buf[0..256]);
        expect_eq(clen > 0, true);

        let out_buf: [256]u8 = undefined;
        let spans: [8]ParsedArgSpan = undefined;
        let res = splitCommand(cmd_buf[0..(clen as usize)], out_buf[0..256], spans[0..8]);
        expect_eq(res.ok, true);
        expect_eq(res.arg_count, 3);
        expect_eq(eqBytes(out_buf[spans[1].start .. spans[1].start + spans[1].len], "hello 'world' $HOME"), true);
        expect_eq(spans[2].len, 0);
    });

    test("unterminated quotes and undersized buffers fail safely", () => {
        let out_buf: [64]u8 = undefined;
        let spans: [4]ParsedArgSpan = undefined;
        expect_eq(splitCommand("echo 'unterminated", out_buf[0..64], spans[0..4]).ok, false);
        expect_eq(splitCommand("echo \"unterminated", out_buf[0..64], spans[0..4]).ok, false);
        let tiny: [3]u8 = undefined;
        expect_eq(quoteArg("hello world", tiny[0..3]), -1);
    });
}
