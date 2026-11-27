import { test, expect_eq } from "cap:test";
import { strEq } from "cap:ascii";
import {
    PathSpan,
    EnvPair,
    PathKeyOptions,
    isWindowsPlatform,
    isPathKeyName,
    findPathKeyIndex,
    defaultPathKey,
    pathDelimiter,
    pathKey,
    pathKeyFromPairs,
    findPathValue,
    splitPathEntries,
    joinPathEntries,
    prependPathEntry,
} from "../../packages/path-key/src/index.cp";

fn main() {
    test("1. @cap/path-key — isPathKeyName case-insensitive matching", () => {
        expect_eq(isPathKeyName("PATH"), true);
        expect_eq(isPathKeyName("Path"), true);
        expect_eq(isPathKeyName("pAtH"), true);
        expect_eq(isPathKeyName("PATHEXT"), false);
        expect_eq(isPathKeyName("HOME"), false);
    });

    test("2. @cap/path-key — findPathKeyIndex POSIX exact vs Windows case-insensitive last-wins", () => {
        let keys: [4]string = ["HOME", "Path", "USER", "PATH"];
        expect_eq(findPathKeyIndex(keys[0..4], false), 3 as i64);
        let win_keys: [3]string = ["Path", "TEMP", "pAtH"];
        expect_eq(findPathKeyIndex(win_keys[0..3], true), 2 as i64);
        expect_eq(findPathKeyIndex(win_keys[0..3], false), -1 as i64);
    });

    test("3. @cap/path-key — defaultPathKey, pathDelimiter & isWindowsPlatform", () => {
        expect_eq(strEq(defaultPathKey(false), "PATH"), true);
        expect_eq(strEq(defaultPathKey(true), "Path"), true);
        expect_eq(pathDelimiter(false), 58 as u8);
        expect_eq(pathDelimiter(true), 59 as u8);
        expect_eq(isWindowsPlatform("win32"), true);
        expect_eq(isWindowsPlatform("WINDOWS"), true);
        expect_eq(isWindowsPlatform("linux"), false);
        expect_eq(isWindowsPlatform("darwin"), false);
    });

    test("4. @cap/path-key — pathKey() platform override semantics", () => {
        let win_keys: [3]string = ["SystemRoot", "Path", "pAtH"];
        expect_eq(strEq(pathKey(win_keys[0..3], "win32"), "pAtH"), true);
        expect_eq(strEq(pathKey(win_keys[0..3], "linux"), "PATH"), true);
        let no_path: [2]string = ["HOME", "USER"];
        expect_eq(strEq(pathKey(no_path[0..2], "win32"), "Path"), true);
        expect_eq(strEq(pathKey(no_path[0..2], PathKeyOptions.posix().platform), "PATH"), true);
        expect_eq(strEq(pathKey(no_path[0..2], PathKeyOptions.win32().platform), "Path"), true);
    });

    test("5. @cap/path-key — pathKeyFromPairs & findPathValue", () => {
        let env: [3]EnvPair = [
            EnvPair { key: "HOME", value: "/root" },
            EnvPair { key: "Path", value: "C:\\Win" },
            EnvPair { key: "PATH", value: "/usr/bin" },
        ];
        expect_eq(strEq(pathKeyFromPairs(env[0..3], "win32"), "PATH"), true);
        expect_eq(strEq(findPathValue(env[0..3], "win32").?, "/usr/bin"), true);
        expect_eq(strEq(findPathValue(env[0..3], "linux").?, "/usr/bin"), true);

        let win_only: [2]EnvPair = [
            EnvPair { key: "TEMP", value: "C:\\Temp" },
            EnvPair { key: "Path", value: "C:\\Windows" },
        ];
        expect_eq(strEq(pathKeyFromPairs(win_only[0..2], "win32"), "Path"), true);
        expect_eq(strEq(findPathValue(win_only[0..2], "win32").?, "C:\\Windows"), true);
        expect_eq(findPathValue(win_only[0..2], "linux") == null, true);
    });

    test("6. @cap/path-key — splitPathEntries POSIX & Windows lists", () => {
        const p = "/usr/local/bin:/usr/bin::/bin";
        let spans: [8]PathSpan = [PathSpan { start: 0, len: 0 }; 8];
        const n = splitPathEntries(p, false, spans[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(strEq(p[spans[0].start..spans[0].start + spans[0].len], "/usr/local/bin"), true);
        expect_eq(strEq(p[spans[2].start..spans[2].start + spans[2].len], "/bin"), true);

        const wp = "C:\\Windows;C:\\Tools;";
        let wspans: [4]PathSpan = [PathSpan { start: 0, len: 0 }; 4];
        const wn = splitPathEntries(wp, true, wspans[0..4]);
        expect_eq(wn, 2 as usize);
        expect_eq(wspans[0].len, 10 as usize);
        expect_eq(wspans[1].len, 8 as usize);
    });

    test("7. @cap/path-key — joinPathEntries & prependPathEntry", () => {
        let entries: [4]string = ["/opt/bin", "", "/usr/local/bin", "/bin"];
        let buf: [128]u8 = undefined;
        const n1 = joinPathEntries(entries[0..4], false, buf[0..128]);
        expect_eq(strEq(buf[0..n1] as string, "/opt/bin:/usr/local/bin:/bin"), true);

        const n2 = joinPathEntries(entries[0..4], true, buf[0..128]);
        expect_eq(strEq(buf[0..n2] as string, "/opt/bin;/usr/local/bin;/bin"), true);

        const n3 = prependPathEntry("/custom/bin", "/usr/bin:/bin", false, buf[0..128]);
        expect_eq(strEq(buf[0..n3] as string, "/custom/bin:/usr/bin:/bin"), true);

        const n4 = prependPathEntry("/custom/bin", "", false, buf[0..128]);
        expect_eq(strEq(buf[0..n4] as string, "/custom/bin"), true);
    });

    test("8. @cap/path-key — empty PATH & small output buffer safety", () => {
        let spans: [1]PathSpan = [PathSpan { start: 0, len: 0 }; 1];
        expect_eq(splitPathEntries("", false, spans[0..1]), 0 as usize);
        expect_eq(splitPathEntries("/a:/b:/c", false, spans[0..1]), 1 as usize);
    });
}
