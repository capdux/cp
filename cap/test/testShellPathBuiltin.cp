// Origin: Pure-CP Stdlib Test Suite (lib/shell/pathBuiltin.cp) | Created: 2026-11-22
// Production Deep Verification Suite for POSIX & Bun Shell `basename` / `dirname` Builtins:
//   1. posixBasename edge cases (empty, root `/`, `///`, trailing slashes, nested, single segment)
//   2. posixBasenameStripSuffix (exact suffix strip, non-matching suffix, full-name suffix preserved)
//   3. posixDirname edge cases (empty, single segment -> `.`, root `/`, `///`, `/usr`, `/usr/bin/`)
//   4. runBasenameBuiltin CLI modes: 2-arg suffix, `-a` multiple, `-s` / `--suffix=`, `-z` NUL, `--`
//   5. runDirnameBuiltin CLI modes: multi-path batch, `-z` NUL-delimited, `--` dash-prefixed filename
//   6. Error guards: MissingOperand, ExtraOperand, InvalidOption & OutputTooSmall

import { test, expect_eq, expect_true } from "cap:test";
import {
    posixBasename,
    posixBasenameStripSuffix,
    posixDirname,
    runBasenameBuiltin,
    runDirnameBuiltin,
} from "../../lib/shell/pathBuiltin.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Test 1: posixBasename edge cases
// ============================================================================
test("pathBuiltin: posixBasename handles root, trailing slashes, and empty path", () => {
    expect_true(strEq(posixBasename(""), ""));
    expect_true(strEq(posixBasename("/"), "/"));
    expect_true(strEq(posixBasename("///"), "/"));
    expect_true(strEq(posixBasename("/usr/bin/"), "bin"));
    expect_true(strEq(posixBasename("/usr/bin///"), "bin"));
    expect_true(strEq(posixBasename("a/b/c.cp"), "c.cp"));
    expect_true(strEq(posixBasename("README.md"), "README.md"));
});

// ============================================================================
// Test 2: posixBasenameStripSuffix semantics
// ============================================================================
test("pathBuiltin: posixBasenameStripSuffix preserves exact-match suffix per POSIX", () => {
    expect_true(strEq(posixBasenameStripSuffix("/a/b/foo.test.cp", ".cp"), "foo.test"));
    expect_true(strEq(posixBasenameStripSuffix("/a/b/foo.cp", ".zig"), "foo.cp"));
    // When basename equals suffix, POSIX requires keeping the full basename
    expect_true(strEq(posixBasenameStripSuffix("/a/b/.cp", ".cp"), ".cp"));
    expect_true(strEq(posixBasenameStripSuffix("a/b/c", ""), "c"));
});

// ============================================================================
// Test 3: posixDirname edge cases
// ============================================================================
test("pathBuiltin: posixDirname handles root, single segment, and redundant slashes", () => {
    expect_true(strEq(posixDirname(""), "."));
    expect_true(strEq(posixDirname("foo"), "."));
    expect_true(strEq(posixDirname("/"), "/"));
    expect_true(strEq(posixDirname("///"), "/"));
    expect_true(strEq(posixDirname("/usr"), "/"));
    expect_true(strEq(posixDirname("///usr///"), "/"));
    expect_true(strEq(posixDirname("/usr/bin/"), "/usr"));
    expect_true(strEq(posixDirname("a//b//c"), "a//b"));
});

// ============================================================================
// Test 4: runBasenameBuiltin CLI flags (-a, -s, --suffix=, -z, --, 2-arg)
// ============================================================================
test("pathBuiltin: runBasenameBuiltin supports 2-arg suffix, -a, -s, --suffix=, -z & --", () => {
    let out: [256]u8 = [0; 256];

    // Traditional 2-arg: basename /path/to/app.ts .ts
    const args1: [2][]const u8 = ["/path/to/app.ts", ".ts"];
    const n1 = runBasenameBuiltin(args1[0..], out[0..]) catch 0;
    expect_true(strEq(out[0..n1], "app\n"));

    // -s .cp implies multiple
    const args2: [4][]const u8 = ["-s", ".cp", "lib/a.cp", "lib/b.cp"];
    const n2 = runBasenameBuiltin(args2[0..], out[0..]) catch 0;
    expect_true(strEq(out[0..n2], "a\nb\n"));

    // --suffix= and -z NUL termination
    const args3: [4][]const u8 = ["-z", "--suffix=.h", "/inc/a.h", "/inc/b.h"];
    const n3 = runBasenameBuiltin(args3[0..], out[0..]) catch 0;
    expect_eq(n3, 4);
    expect_eq(out[0], 'a');
    expect_eq(out[1], 0);
    expect_eq(out[2], 'b');
    expect_eq(out[3], 0);

    // `--` option terminator for dash-prefixed filename
    const args4: [2][]const u8 = ["--", "-weird-file"];
    const n4 = runBasenameBuiltin(args4[0..], out[0..]) catch 0;
    expect_true(strEq(out[0..n4], "-weird-file\n"));
});

// ============================================================================
// Test 5: runDirnameBuiltin multi-path batch & -z NUL delimiter
// ============================================================================
test("pathBuiltin: runDirnameBuiltin handles multi-path batch, -z & --", () => {
    let out: [256]u8 = [0; 256];

    const args1: [3][]const u8 = ["/usr/bin/env", "local.txt", "/"];
    const n1 = runDirnameBuiltin(args1[0..], out[0..]) catch 0;
    expect_true(strEq(out[0..n1], "/usr/bin\n.\n/\n"));

    const args2: [3][]const u8 = ["-z", "--", "-dir/file"];
    const n2 = runDirnameBuiltin(args2[0..], out[0..]) catch 0;
    expect_eq(n2, 5);
    expect_true(strEq(out[0..4], "-dir"));
    expect_eq(out[4], 0);
});

// ============================================================================
// Test 6: Error guards (MissingOperand, ExtraOperand, InvalidOption, OutputTooSmall)
// ============================================================================
test("pathBuiltin: rejects missing/extra operands, unknown flags, and tiny output buffer", () => {
    let out: [64]u8 = [0; 64];
    const empty: [0][]const u8 = [];

    let missing_b = false;
    * = runBasenameBuiltin(empty[0..], out[0..]) catch brk: {
        missing_b = true;
        break :brk 0;
    };
    expect_true(missing_b);

    // 3 positional args without -a -> ExtraOperand
    const extra: [3][]const u8 = ["a", "b", "c"];
    let got_extra = false;
    * = runBasenameBuiltin(extra[0..], out[0..]) catch brk: {
        got_extra = true;
        break :brk 0;
    };
    expect_true(got_extra);

    // Unknown flag
    const bad_opt: [2][]const u8 = ["-X", "foo"];
    let got_invalid = false;
    * = runDirnameBuiltin(bad_opt[0..], out[0..]) catch brk: {
        got_invalid = true;
        break :brk 0;
    };
    expect_true(got_invalid);

    // OutputTooSmall guard
    let tiny: [2]u8 = [0; 2];
    const one: [1][]const u8 = ["/usr/local/bin"];
    let too_small = false;
    * = runDirnameBuiltin(one[0..], tiny[0..]) catch brk: {
        too_small = true;
        break :brk 0;
    };
    expect_true(too_small);
});
