import { test, expect_eq } from "cap:test";
import {
    WhichIterator, isExplicitPath, countPathDirs, pathDirAt, hasPathExt
} from "../../packages/@cap/which-bin/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/which-bin — POSIX PATH iteration yields normalized candidates", () => {
        let it = WhichIterator.newPosix("cap", "/usr/local/bin:/usr/bin/:/bin");
        let buf: [128]u8 = undefined;
        expect_eq(eqBytes(buf[0..128], it.next(buf[0..128]), "/usr/local/bin/cap"), true);
        expect_eq(eqBytes(buf[0..128], it.next(buf[0..128]), "/usr/bin/cap"), true);
        expect_eq(eqBytes(buf[0..128], it.next(buf[0..128]), "/bin/cap"), true);
        expect_eq(it.next(buf[0..128]), 0);
    });

    test("2. @cap/which-bin — Windows PATH + PATHEXT expansion", () => {
        let it = WhichIterator.newWindows("git", "\"C:\\Program Files\\Git\\bin\";C:\\Tools\\", ".EXE;.CMD");
        let buf: [128]u8 = undefined;
        expect_eq(eqBytes(buf[0..128], it.next(buf[0..128]), "C:/Program Files/Git/bin/git.EXE"), true);
        expect_eq(eqBytes(buf[0..128], it.next(buf[0..128]), "C:/Program Files/Git/bin/git.CMD"), true);
        expect_eq(eqBytes(buf[0..128], it.next(buf[0..128]), "C:/Tools/git.EXE"), true);
        expect_eq(eqBytes(buf[0..128], it.next(buf[0..128]), "C:/Tools/git.CMD"), true);
        expect_eq(it.next(buf[0..128]), 0);
    });

    test("3. @cap/which-bin — Windows skips PATHEXT if command already has extension", () => {
        expect_eq(hasPathExt("node.exe", ".EXE;.CMD"), true);
        expect_eq(hasPathExt("node", ".EXE;.CMD"), false);
        let it = WhichIterator.newWindows("node.exe", "C:\\Node", ".EXE;.CMD");
        let buf: [128]u8 = undefined;
        expect_eq(eqBytes(buf[0..128], it.next(buf[0..128]), "C:/Node/node.exe"), true);
        expect_eq(it.next(buf[0..128]), 0);
    });

    test("4. @cap/which-bin — Explicit path yields normalized path once", () => {
        expect_eq(isExplicitPath("./bin/cap"), true);
        expect_eq(isExplicitPath("cap"), false);
        let it = WhichIterator.newPosix("./bin\\cap", "/usr/bin");
        let buf: [128]u8 = undefined;
        expect_eq(eqBytes(buf[0..128], it.next(buf[0..128]), "./bin/cap"), true);
        expect_eq(it.next(buf[0..128]), 0);
    });

    test("5. @cap/which-bin — countPathDirs and pathDirAt skip empty segments", () => {
        expect_eq(countPathDirs(":/a::/b/:", 58), 2);
        let buf: [64]u8 = undefined;
        expect_eq(eqBytes(buf[0..64], pathDirAt(":/a::/b/:", 58, 0, buf[0..64]), "/a"), true);
        expect_eq(eqBytes(buf[0..64], pathDirAt(":/a::/b/:", 58, 1, buf[0..64]), "/b"), true);
        expect_eq(pathDirAt(":/a::/b/:", 58, 2, buf[0..64]), 0);
    });

    test("6. @cap/which-bin — Empty command or empty PATH returns 0 safely", () => {
        let it1 = WhichIterator.newPosix("", "/usr/bin");
        let it2 = WhichIterator.newPosix("cap", "");
        let buf: [64]u8 = undefined;
        expect_eq(it1.next(buf[0..64]), 0);
        expect_eq(it2.next(buf[0..64]), 0);
    });
}
