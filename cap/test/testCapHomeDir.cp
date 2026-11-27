import { test, expect_eq } from "cap:test";
import {
    resolveHomeDir,
    expandTilde,
    contractTilde,
    joinConfigDir,
} from "../../packages/home/src/index.cp";

fn strEq(buf: []const u8, expected: string): bool {
    if (buf.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/home-dir — POSIX HOME & Windows USERPROFILE / HOMEDRIVE+HOMEPATH priority", () => {
        let buf: [128]u8 = undefined;

        // POSIX uses HOME
        const n1 = resolveHomeDir("/home/alice", "C:\\Users\\alice", "C:", "\\Users\\alice", false, &buf);
        expect_eq(strEq(buf[0..n1], "/home/alice"), true);

        // Windows prefers HOME if set
        const n2 = resolveHomeDir("C:\\msys\\home\\alice", "C:\\Users\\alice", "C:", "\\Users\\alice", true, &buf);
        expect_eq(strEq(buf[0..n2], "C:\\msys\\home\\alice"), true);

        // Windows falls back to USERPROFILE
        const n3 = resolveHomeDir("", "C:\\Users\\alice", "C:", "\\Users\\bob", true, &buf);
        expect_eq(strEq(buf[0..n3], "C:\\Users\\alice"), true);

        // Windows falls back to HOMEDRIVE + HOMEPATH
        const n4 = resolveHomeDir("", "", "D:", "\\Users\\alice", true, &buf);
        expect_eq(strEq(buf[0..n4], "D:\\Users\\alice"), true);
    });

    test("2. @cap/home-dir — expandTilde handles '~', '~/a/b', '~\\win' & skips '~bob/x'", () => {
        let buf: [128]u8 = undefined;

        const n1 = expandTilde("~", "/Users/alice", &buf);
        expect_eq(strEq(buf[0..n1], "/Users/alice"), true);

        const n2 = expandTilde("~/a/b", "/Users/alice/", &buf);
        expect_eq(strEq(buf[0..n2], "/Users/alice/a/b"), true);

        const n3 = expandTilde("~\\win\\cfg", "C:\\Users\\alice\\", &buf);
        expect_eq(strEq(buf[0..n3], "C:\\Users\\alice\\win\\cfg"), true);

        // Must NOT expand ~otheruser
        const n4 = expandTilde("~bob/x", "/Users/alice", &buf);
        expect_eq(strEq(buf[0..n4], "~bob/x"), true);

        // Non-tilde path unchanged
        const n5 = expandTilde("/etc/hosts", "/Users/alice", &buf);
        expect_eq(strEq(buf[0..n5], "/etc/hosts"), true);
    });

    test("3. @cap/home-dir — contractTilde folds exact home prefix & guards '/home/a2'", () => {
        let buf: [128]u8 = undefined;

        const n1 = contractTilde("/Users/alice/docs", "/Users/alice", &buf);
        expect_eq(strEq(buf[0..n1], "~/docs"), true);

        const n2 = contractTilde("/Users/alice", "/Users/alice/", &buf);
        expect_eq(strEq(buf[0..n2], "~"), true);

        // Must NOT falsely contract sibling user '/home/a2'
        const n3 = contractTilde("/home/a2/file", "/home/a", &buf);
        expect_eq(strEq(buf[0..n3], "/home/a2/file"), true);

        // Windows backslash contraction
        const n4 = contractTilde("C:\\Users\\alice\\AppData", "C:\\Users\\alice", &buf);
        expect_eq(strEq(buf[0..n4], "~\\AppData"), true);
    });

    test("4. @cap/home-dir — joinConfigDir XDG override, POSIX .config & Windows Roaming", () => {
        let buf: [128]u8 = undefined;

        const n1 = joinConfigDir("/home/alice", "/custom/xdg/", "cap", false, &buf);
        expect_eq(strEq(buf[0..n1], "/custom/xdg/cap"), true);

        const n2 = joinConfigDir("/home/alice/", "", "cap", false, &buf);
        expect_eq(strEq(buf[0..n2], "/home/alice/.config/cap"), true);

        const n3 = joinConfigDir("C:\\Users\\alice", "", "cap", true, &buf);
        expect_eq(strEq(buf[0..n3], "C:\\Users\\alice\\AppData\\Roaming\\cap"), true);

        // Empty app_name returns base config dir
        const n4 = joinConfigDir("/home/alice", "", "", false, &buf);
        expect_eq(strEq(buf[0..n4], "/home/alice/.config"), true);
    });

    test("5. @cap/home-dir — Round-trip expandTilde <-> contractTilde", () => {
        let expanded: [128]u8 = undefined;
        let contracted: [128]u8 = undefined;
        const home: string = "/home/dev";
        const orig: string = "~/.config/cap/settings.json";

        const ne = expandTilde(orig, home, &expanded);
        expect_eq(strEq(expanded[0..ne], "/home/dev/.config/cap/settings.json"), true);

        const nc = contractTilde(expanded[0..ne], home, &contracted);
        expect_eq(strEq(contracted[0..nc], orig), true);
    });

    test("6. @cap/home-dir — Empty env vars & small buffer truncation boundaries", () => {
        let buf: [32]u8 = undefined;
        expect_eq(resolveHomeDir("", "", "", "", false, &buf), 0);
        expect_eq(resolveHomeDir("", "", "", "", true, &buf), 0);
        expect_eq(joinConfigDir("", "", "cap", false, &buf), 0);
        expect_eq(expandTilde("", "/home/alice", &buf), 0);
        expect_eq(contractTilde("", "/home/alice", &buf), 0);

        let small: [6]u8 = undefined;
        const nt = expandTilde("~/documents", "/home/alice", &small);
        expect_eq(nt, 6);
        expect_eq(strEq(small[0..nt], "/home/"), true);

        const empty_out: []u8 = small[0..0];
        expect_eq(resolveHomeDir("/home/alice", "", "", "", false, empty_out), 0);
        expect_eq(expandTilde("~/a", "/home/alice", empty_out), 0);
    });
}
