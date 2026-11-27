import { test, expect_eq } from "cap:test";
import {
    isSafePosixWord,
    escapePosix,
    escapeWindows,
    joinPosixCommand,
    isValidEnvName,
} from "../../packages/shell-escape/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/shell-escape — Safe POSIX words pass through & empty becomes ''", () => {
        expect_eq(isSafePosixWord("/usr/local/bin/cap-0.1_rc+1:a,b@c%d=e"), true);
        expect_eq(isSafePosixWord(""), false);
        expect_eq(isSafePosixWord("has space"), false);

        let buf: [64]u8 = undefined;
        const n1 = escapePosix("src/main.cp", buf[0..64]);
        expect_eq(testEq(buf[0..n1], "src/main.cp"), true);

        const n0 = escapePosix("", buf[0..64]);
        expect_eq(testEq(buf[0..n0], "''"), true);
    });

    test("2. @cap/shell-escape — POSIX escaping of spaces, $, backticks & single quotes", () => {
        let buf: [128]u8 = undefined;
        const n1 = escapePosix("hello $HOME `id` ; rm", buf[0..128]);
        expect_eq(testEq(buf[0..n1], "'hello $HOME `id` ; rm'"), true);

        const n2 = escapePosix("it's ok", buf[0..128]);
        expect_eq(testEq(buf[0..n2], "'it'\\''s ok'"), true);
    });

    test("3. @cap/shell-escape — escapeWindows spaces, quotes & trailing backslash doubling", () => {
        let buf: [128]u8 = undefined;
        const n_plain = escapeWindows("plain.exe", buf[0..128]);
        expect_eq(testEq(buf[0..n_plain], "plain.exe"), true);

        const n_empty = escapeWindows("", buf[0..128]);
        expect_eq(testEq(buf[0..n_empty], "\"\""), true);

        const n_dir = escapeWindows("C:\\Program Files\\", buf[0..128]);
        expect_eq(testEq(buf[0..n_dir], "\"C:\\Program Files\\\\\""), true);

        const n_q = escapeWindows("a\\\"b", buf[0..128]);
        expect_eq(testEq(buf[0..n_q], "\"a\\\\\\\"b\""), true);
    });

    test("4. @cap/shell-escape — joinPosixCommand full command assembly", () => {
        const args: [4]string = ["git", "commit", "-m", "feat: add 'cp' support"];
        let buf: [128]u8 = undefined;
        const n = joinPosixCommand(args[0..4], buf[0..128]);
        const expected = "git commit -m 'feat: add '\\''cp'\\'' support'";
        expect_eq(n, expected.len);
        expect_eq(testEq(buf[0..n], expected), true);
    });

    test("5. @cap/shell-escape — isValidEnvName validation", () => {
        expect_eq(isValidEnvName("PATH"), true);
        expect_eq(isValidEnvName("_CAP_HOME_2"), true);
        expect_eq(isValidEnvName(""), false);
        expect_eq(isValidEnvName("1INVALID"), false);
        expect_eq(isValidEnvName("BAD=NAME"), false);
        expect_eq(isValidEnvName("BAD-NAME"), false);
    });

    test("6. @cap/shell-escape — Undersized buffer protection", () => {
        let tiny: [4]u8 = undefined;
        expect_eq(escapePosix("hello world", tiny[0..4]), 0 as usize);
        expect_eq(escapeWindows("hello world", tiny[0..4]), 0 as usize);

        const args: [2]string = ["echo", "hello world"];
        expect_eq(joinPosixCommand(args[0..2], tiny[0..4]), 0 as usize);
    });
}
