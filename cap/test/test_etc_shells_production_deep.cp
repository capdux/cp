import {
    parseShellLine,
    EtcShellsIter,
    isAllowedShell,
    findShellByBasename,
    countShells,
    isNologinShell,
} from "cap:etc_shells";
import { test, expect, expect_eq } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

const SAMPLE_SHELLS =
    "# /etc/shells: valid login shells\n" ++
    "\n" ++
    "   # indented comment\r\n" ++
    "/bin/sh\n" ++
    "  /bin/bash  \r\n" ++
    "relative/bin/sh\n" ++
    "/bin/with space\n" ++
    "/bin/with\ttab\n" ++
    "/bin/dir/\n" ++
    "/\n" ++
    "/usr/bin/zsh\n" ++
    "/usr/bin/bash\n" ++
    "/usr/local/bin/fish";

test("EtcShellsIter streams valid absolute paths and filters comments, blanks, and malformed entries", () => {
    let it = EtcShellsIter.init(SAMPLE_SHELLS);

    expect(strEq(it.next().?, "/bin/sh"));
    expect(strEq(it.next().?, "/bin/bash"));
    expect(strEq(it.next().?, "/usr/bin/zsh"));
    expect(strEq(it.next().?, "/usr/bin/bash"));
    expect(strEq(it.next().?, "/usr/local/bin/fish"));
    expect(it.next() == null);
    expect(it.next() == null);

    expect_eq(countShells(SAMPLE_SHELLS), 5);
});

test("isAllowedShell exact absolute path matching", () => {
    expect(isAllowedShell(SAMPLE_SHELLS, "/bin/sh"));
    expect(isAllowedShell(SAMPLE_SHELLS, "/bin/bash"));
    expect(isAllowedShell(SAMPLE_SHELLS, "/usr/bin/zsh"));
    expect(isAllowedShell(SAMPLE_SHELLS, "/usr/local/bin/fish"));

    // Reject partial, untrimmed, malformed, or unlisted paths
    expect(!isAllowedShell(SAMPLE_SHELLS, "bash"));
    expect(!isAllowedShell(SAMPLE_SHELLS, "/bin/bash "));
    expect(!isAllowedShell(SAMPLE_SHELLS, "relative/bin/sh"));
    expect(!isAllowedShell(SAMPLE_SHELLS, "/bin/with space"));
    expect(!isAllowedShell(SAMPLE_SHELLS, "/sbin/nologin"));
    expect(!isAllowedShell(SAMPLE_SHELLS, ""));
});

test("findShellByBasename returns first matching full path and rejects invalid basenames", () => {
    // Returns first occurrence when multiple paths share basename ("/bin/bash" before "/usr/bin/bash")
    expect(strEq(findShellByBasename(SAMPLE_SHELLS, "bash").?, "/bin/bash"));
    expect(strEq(findShellByBasename(SAMPLE_SHELLS, "zsh").?, "/usr/bin/zsh"));
    expect(strEq(findShellByBasename(SAMPLE_SHELLS, "fish").?, "/usr/local/bin/fish"));
    expect(strEq(findShellByBasename(SAMPLE_SHELLS, "sh").?, "/bin/sh"));

    // Must not match substrings or paths containing `/`
    expect(findShellByBasename(SAMPLE_SHELLS, "ash") == null);
    expect(findShellByBasename(SAMPLE_SHELLS, "ba") == null);
    expect(findShellByBasename(SAMPLE_SHELLS, "/bin/bash") == null);
    expect(findShellByBasename(SAMPLE_SHELLS, "bin/bash") == null);
    expect(findShellByBasename(SAMPLE_SHELLS, "") == null);
    expect(findShellByBasename(SAMPLE_SHELLS, "tcsh") == null);
});

test("isNologinShell identifies standard non-interactive shells", () => {
    expect(isNologinShell("/sbin/nologin"));
    expect(isNologinShell("/usr/sbin/nologin"));
    expect(isNologinShell("/bin/false"));
    expect(isNologinShell("/usr/bin/false"));

    expect(!isNologinShell("/bin/sh"));
    expect(!isNologinShell("/bin/bash"));
    expect(!isNologinShell("/bin/true"));
    expect(!isNologinShell("nologin"));
    expect(!isNologinShell(""));
});

test("parseShellLine malformed input and control-character protection", () => {
    // Blank, whitespace, and comments
    expect(parseShellLine("") == null);
    expect(parseShellLine("   \t  \r") == null);
    expect(parseShellLine("#/bin/sh") == null);
    expect(parseShellLine("  # /bin/bash") == null);

    // Non-absolute, root-only, or trailing slash
    expect(parseShellLine("bin/bash") == null);
    expect(parseShellLine("./sh") == null);
    expect(parseShellLine("/") == null);
    expect(parseShellLine("/bin/") == null);

    // Internal whitespace and control characters (NUL, BEL, ESC, DEL)
    expect(parseShellLine("/bin/sh -c") == null);
    expect(parseShellLine("/bin/sh\targ") == null);
    expect(parseShellLine("/bin/sh\x00evil") == null);
    expect(parseShellLine("/bin/sh\x07") == null);
    expect(parseShellLine("/bin/sh\x1b") == null);
    expect(parseShellLine("/bin/sh\x7f") == null);

    // Valid trimmed CRLF line
    expect(strEq(parseShellLine("\t /usr/bin/dash \r").?, "/usr/bin/dash"));

    // Empty content edge cases
    let empty_it = EtcShellsIter.init("");
    expect(empty_it.next() == null);
    expect_eq(countShells(""), 0);
    expect(!isAllowedShell("", "/bin/sh"));
    expect(findShellByBasename("", "sh") == null);
});
