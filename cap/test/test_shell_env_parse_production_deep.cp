// Source: Absorbed from /data/dev/bun/src/env_loader.zig (POSIX Shell Env Assignment Parser)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ShellEnvEntry,
    ShellEnvIter,
    getShellEnv,
} from "cap:shell";

test("1. ShellEnvIter parses export prefix, plain assignments, comments, blank lines, and line numbers", () => {
    const src =
        "# profile header\n"
        ++ "\n"
        ++ "export PATH=/usr/local/bin:/usr/bin # inline comment\n"
        ++ "   EDITOR=vim   # trailing comment\n"
        ++ "COLOR=#ff00ff\n"
        ++ "EMPTY= # empty value\n"
        ++ "export_dir=/tmp/out\n"
        ++ "unset PATH\n"
        ++ "export\tLANG=en_US.UTF-8\r\n";

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let it = ShellEnvIter.init(src);

    const e1 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "PATH"), true);
    expect_eq(std.mem.eql(u8, e1.value, "/usr/local/bin:/usr/bin"), true);
    expect_eq(e1.is_exported, true);
    expect_eq(e1.line_no, 3);

    const e2 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.key, "EDITOR"), true);
    expect_eq(std.mem.eql(u8, e2.value, "vim"), true);
    expect_eq(e2.is_exported, false);
    expect_eq(e2.line_no, 4);

    const e3 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.key, "COLOR"), true);
    expect_eq(std.mem.eql(u8, e3.value, "#ff00ff"), true);
    expect_eq(e3.is_exported, false);
    expect_eq(e3.line_no, 5);

    const e4 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e4.key, "EMPTY"), true);
    expect_eq(e4.value.len, 0);
    expect_eq(e4.line_no, 6);

    const e5 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e5.key, "export_dir"), true);
    expect_eq(std.mem.eql(u8, e5.value, "/tmp/out"), true);
    expect_eq(e5.is_exported, false);
    expect_eq(e5.line_no, 7);

    // unset PATH on line 8 is skipped; LANG is on line 9
    const e6 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e6.key, "LANG"), true);
    expect_eq(std.mem.eql(u8, e6.value, "en_US.UTF-8"), true);
    expect_eq(e6.is_exported, true);
    expect_eq(e6.line_no, 9);

    expect_eq(it.next(&buf) == null, true);
});

test("2. Single-quoted literals preserve escapes, hash, spaces, and multiline newlines", () => {
    const src =
        "RAW='hello\\nworld # not comment $HOME'\n"
        ++ "export MULTI='line1\nline2'\n"
        ++ "AFTER=ok\n";

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let it = ShellEnvIter.init(src);

    const e1 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "RAW"), true);
    expect_eq(std.mem.eql(u8, e1.value, "hello\\nworld # not comment $HOME"), true);
    expect_eq(e1.line_no, 1);

    const e2 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.key, "MULTI"), true);
    expect_eq(std.mem.eql(u8, e2.value, "line1\nline2"), true);
    expect_eq(e2.is_exported, true);
    expect_eq(e2.line_no, 2);

    const e3 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.key, "AFTER"), true);
    expect_eq(std.mem.eql(u8, e3.value, "ok"), true);
    expect_eq(e3.line_no, 4);
});

test("3. Double-quoted values decode escapes and line continuations while preserving unknown escapes", () => {
    const src =
        "ESC=\"a\\nb\\rc\\td\\\"e\\\\f\\$g\" # trailing comment\n"
        ++ "export CONT=\"hello \\\nworld\"\n"
        ++ "KEEP=\"\\q\\z\"\n";

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let it = ShellEnvIter.init(src);

    const e1 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "ESC"), true);
    expect_eq(std.mem.eql(u8, e1.value, "a\nb\rc\td\"e\\f$g"), true);
    expect_eq(e1.line_no, 1);

    const e2 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.key, "CONT"), true);
    expect_eq(std.mem.eql(u8, e2.value, "hello world"), true);
    expect_eq(e2.is_exported, true);
    expect_eq(e2.line_no, 2);

    const e3 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.key, "KEEP"), true);
    expect_eq(std.mem.eql(u8, e3.value, "\\q\\z"), true);
    expect_eq(e3.line_no, 4);
});

test("4. getShellEnv applies last-wins override and unset clearing semantics", () => {
    const src =
        "FOO=first\n"
        ++ "export FOO=\"second\\tval\"\n"
        ++ "BAR=alive\n"
        ++ "unset BAR BAZ # clear both\n"
        ++ "REBORN=old\n"
        ++ "unset -v REBORN\n"
        ++ "REBORN=new\n"
        ++ "FUNC_VAR=keep\n"
        ++ "unset -f FUNC_VAR\n";

    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const v_foo = getShellEnv(src, "FOO", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, v_foo, "second\tval"), true);

    expect_eq(getShellEnv(src, "BAR", &buf) == null, true);
    expect_eq(getShellEnv(src, "MISSING", &buf) == null, true);

    const v_reborn = getShellEnv(src, "REBORN", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, v_reborn, "new"), true);

    const v_func = getShellEnv(src, "FUNC_VAR", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, v_func, "keep"), true);
});

test("5. Malformed lines, invalid keys, UTF-8 BOM, unclosed quotes, and tiny buffer guards", () => {
    // UTF-8 BOM + 畸形行 + 非法 key + 普通 shell 命令 + 正常键值
    const bom_src =
        "\xEF\xBB\xBF"
        ++ "123BAD=nope\n"
        ++ "echo hello=world\n"
        ++ "export ONLY_NAME\n"
        ++ "BAD-KEY=nope\n"
        ++ "GOOD_KEY=ok_val\n";

    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let it = ShellEnvIter.init(bom_src);
    const e1 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "GOOD_KEY"), true);
    expect_eq(std.mem.eql(u8, e1.value, "ok_val"), true);
    expect_eq(it.next(&buf) == null, true);

    // 未闭合引号安全返回 null
    expect_eq(getShellEnv("BROKEN=\"unterminated", "BROKEN", &buf) == null, true);
    expect_eq(getShellEnv("BROKEN='unterminated", "BROKEN", &buf) == null, true);

    // 非法查询 key 返回 null
    expect_eq(getShellEnv("A=1", "", &buf) == null, true);
    expect_eq(getShellEnv("A=1", "1BAD", &buf) == null, true);

    // 极小缓冲区防护：超长无关行不影响后续短值查询，目标值溢出返回 null
    const size_src =
        "LONG_OTHER=this_is_a_very_long_value_that_exceeds_tiny_buf\n"
        ++ "SHORT=hi\n"
        ++ "TARGET=ok\n"
        ++ "TARGET=too_long_for_tiny_buf\n";
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    const v_short = getShellEnv(size_src, "SHORT", &tiny) ?? unreachable;
    expect_eq(std.mem.eql(u8, v_short, "hi"), true);
    expect_eq(getShellEnv(size_src, "TARGET", &tiny) == null, true);
});
