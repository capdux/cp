// cap/test/testCapShlex.cp
// Comprehensive unit tests for @cap/shlex

import { test, expect_eq } from "cap:test";
import {
    ShlexError,
    needsQuote,
    hasShellMetachars,
    quoteArg,
    joinArgs,
    splitShell,
} from "../../packages/shlex/src/index.cp";

fn main() {
    test("1. @cap/shlex — needsQuote, quoteArg & injection neutralization", () => {
        expect_eq(needsQuote("hello-world_1.0/path"), false);
        expect_eq(needsQuote(""), true);
        expect_eq(needsQuote("hello world"), true);
        expect_eq(needsQuote("$(whoami)"), true);

        let buf: [256]u8 = undefined;
        expect_eq(quoteArg("plain", buf[0..256]), "plain");
        expect_eq(quoteArg("", buf[0..256]), "''");
        expect_eq(quoteArg("hello world", buf[0..256]), "'hello world'");
        expect_eq(quoteArg("a'b", buf[0..256]), "'a'\\''b'");
        expect_eq(quoteArg("$(rm -rf /)", buf[0..256]), "'$(rm -rf /)'");
    });

    test("2. @cap/shlex — joinArgs full command line construction", () => {
        const args: [4]string = ["cap", "deploy", "--desc", "hello 'capdux' $USER"];
        let buf: [512]u8 = undefined;
        const cmd = joinArgs(args[0..4], buf[0..512]);
        expect_eq(cmd, "cap deploy --desc 'hello '\\''capdux'\\'' $USER'");
    });

    test("3. @cap/shlex — splitShell quotes, escapes & comments", () => {
        const r = splitShell("cap run --msg 'hello world' \"a\\\"b\\\\c\" foo\\ bar # trailing comment");
        expect_eq(r.ok, true);
        expect_eq(r.count, 6 as usize);
        expect_eq(r.tokenAt(0 as usize), "cap");
        expect_eq(r.tokenAt(1 as usize), "run");
        expect_eq(r.tokenAt(2 as usize), "--msg");
        expect_eq(r.tokenAt(3 as usize), "hello world");
        expect_eq(r.tokenAt(4 as usize), "a\"b\\c");
        expect_eq(r.tokenAt(5 as usize), "foo bar");
    });

    test("4. @cap/shlex — Round-trip joinArgs -> splitShell lossless parity", () => {
        const orig: [4]string = ["echo", "", "it's a $test; | ok", "a\"b"];
        let buf: [512]u8 = undefined;
        const joined = joinArgs(orig[0..4], buf[0..512]);
        const parsed = splitShell(joined);
        expect_eq(parsed.ok, true);
        expect_eq(parsed.count, 4 as usize);
        expect_eq(parsed.tokenAt(0 as usize), "echo");
        expect_eq(parsed.tokenAt(1 as usize), "");
        expect_eq(parsed.tokenAt(2 as usize), "it's a $test; | ok");
        expect_eq(parsed.tokenAt(3 as usize), "a\"b");
    });

    test("5. @cap/shlex — Error detection & metachar guard", () => {
        const e1 = splitShell("echo 'unclosed");
        expect_eq(e1.ok, false);
        expect_eq(e1.err == ShlexError.UnclosedSingleQuote, true);

        const e2 = splitShell("echo \"unclosed");
        expect_eq(e2.ok, false);
        expect_eq(e2.err == ShlexError.UnclosedDoubleQuote, true);

        const e3 = splitShell("echo trailing\\");
        expect_eq(e3.ok, false);
        expect_eq(e3.err == ShlexError.TrailingBackslash, true);

        expect_eq(hasShellMetachars("safe_file.txt"), false);
        expect_eq(hasShellMetachars("a; rm -rf /"), true);
        expect_eq(hasShellMetachars("a | grep b"), true);
    });
}
