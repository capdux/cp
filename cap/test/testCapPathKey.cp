import { test, expect_eq } from "cap:test";
import {
    PathSpan, isPathKeyName, findPathKeyIndex,
    defaultPathKey, pathDelimiter, splitPathEntries
} from "../../packages/path-key/src/index.cp";

fn eqStr(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("isPathKeyName case-insensitive matching", () => {
        expect_eq(isPathKeyName("PATH"), true);
        expect_eq(isPathKeyName("Path"), true);
        expect_eq(isPathKeyName("pAtH"), true);
        expect_eq(isPathKeyName("PATHEXT"), false);
        expect_eq(isPathKeyName("HOME"), false);
    });

    test("findPathKeyIndex POSIX exact vs Windows case-insensitive last-wins", () => {
        let keys: [4]string = ["HOME", "Path", "USER", "PATH"];
        expect_eq(findPathKeyIndex(keys[0..4], false), 3 as i64);
        let win_keys: [3]string = ["Path", "TEMP", "pAtH"];
        expect_eq(findPathKeyIndex(win_keys[0..3], true), 2 as i64);
        expect_eq(findPathKeyIndex(win_keys[0..3], false), -1 as i64);
    });

    test("defaultPathKey & pathDelimiter", () => {
        expect_eq(eqStr(defaultPathKey(false), "PATH"), true);
        expect_eq(eqStr(defaultPathKey(true), "Path"), true);
        expect_eq(pathDelimiter(false), 58 as u8);
        expect_eq(pathDelimiter(true), 59 as u8);
    });

    test("splitPathEntries POSIX colon list", () => {
        let p = "/usr/local/bin:/usr/bin::/bin";
        let spans: [8]PathSpan = [PathSpan { start: 0, len: 0 }; 8];
        let n = splitPathEntries(p, false, spans[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(eqStr(p[spans[0].start..spans[0].start + spans[0].len], "/usr/local/bin"), true);
        expect_eq(eqStr(p[spans[2].start..spans[2].start + spans[2].len], "/bin"), true);
    });

    test("splitPathEntries Windows semicolon list", () => {
        let p = "C:\\Windows;C:\\Tools;";
        let spans: [4]PathSpan = [PathSpan { start: 0, len: 0 }; 4];
        let n = splitPathEntries(p, true, spans[0..4]);
        expect_eq(n, 2 as usize);
        expect_eq(spans[0].len, 10 as usize);
        expect_eq(spans[1].len, 8 as usize);
    });

    test("empty PATH & small output buffer safety", () => {
        let spans: [1]PathSpan = [PathSpan { start: 0, len: 0 }; 1];
        expect_eq(splitPathEntries("", false, spans[0..1]), 0 as usize);
        expect_eq(splitPathEntries("/a:/b:/c", false, spans[0..1]), 1 as usize);
    });
}
