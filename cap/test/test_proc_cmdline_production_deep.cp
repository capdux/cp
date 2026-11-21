// Source: cp/cap/test/test_proc_cmdline_production_deep.cp (Production deep tests for cap:proc_cmdline)

import { test, expect_eq } from "cap:test";
import {
    NulArgIter,
    countArgs,
    argAt,
    ProcEnvEntry,
    ProcEnvIter,
    findProcEnv,
    formatCmdlineDisplay,
} from "cap:proc_cmdline";

test("proc_cmdline: NulArgIter handles trailing NUL, missing trailing NUL and empty middle args") {
    const with_nul = "/usr/bin/cap\x00--flag\x00\x00file.cp\x00";
    expect_eq(countArgs(with_nul), 4);
    expect_eq(std.mem.eql(u8, argAt(with_nul, 0).?, "/usr/bin/cap"), true);
    expect_eq(std.mem.eql(u8, argAt(with_nul, 1).?, "--flag"), true);
    expect_eq(argAt(with_nul, 2).?.len, 0);
    expect_eq(std.mem.eql(u8, argAt(with_nul, 3).?, "file.cp"), true);
    expect_eq(argAt(with_nul, 4) == null, true);

    // 无尾随 NUL
    const no_nul = "echo\x00hello";
    expect_eq(countArgs(no_nul), 2);
    expect_eq(std.mem.eql(u8, argAt(no_nul, 1).?, "hello"), true);

    // 空缓冲
    expect_eq(countArgs(""), 0);
    expect_eq(argAt("", 0) == null, true);
}

test("proc_cmdline: ProcEnvIter parses key=value with embedded '=' and skips malformed items") {
    const env_buf =
        "PATH=/usr/bin:/bin\x00" ++
        "MALFORMED_NO_EQ\x00" ++
        "=EMPTY_KEY\x00" ++
        "CONN=host=127.0.0.1;port=5432\x00" ++
        "EMPTY_VAL=\x00";

    let it = ProcEnvIter.init(env_buf);

    const e1 = it.next().?;
    expect_eq(std.mem.eql(u8, e1.key, "PATH"), true);
    expect_eq(std.mem.eql(u8, e1.value, "/usr/bin:/bin"), true);

    const e2 = it.next().?;
    expect_eq(std.mem.eql(u8, e2.key, "CONN"), true);
    expect_eq(std.mem.eql(u8, e2.value, "host=127.0.0.1;port=5432"), true);

    const e3 = it.next().?;
    expect_eq(std.mem.eql(u8, e3.key, "EMPTY_VAL"), true);
    expect_eq(e3.value.len, 0);

    expect_eq(it.next() == null, true);
}

test("proc_cmdline: findProcEnv exact match and POSIX first-match semantics") {
    const env_buf = "FOO=first\x00FOO_BAR=other\x00FOO=second\x00";

    expect_eq(std.mem.eql(u8, findProcEnv(env_buf, "FOO").?, "first"), true);
    expect_eq(std.mem.eql(u8, findProcEnv(env_buf, "FOO_BAR").?, "other"), true);
    expect_eq(findProcEnv(env_buf, "MISSING") == null, true);
    expect_eq(findProcEnv(env_buf, "") == null, true);
}

test("proc_cmdline: formatCmdlineDisplay quotes spaces, quotes, empty args and shell metachars") {
    const cmd = "/bin/sh\x00-c\x00echo 'it's $HOME'\x00\x00plain_arg\x00";
    let out: [256]u8 = [0; 256];

    const disp = formatCmdlineDisplay(cmd, out[0..]).?;
    expect_eq(
        std.mem.eql(u8, disp, "/bin/sh -c 'echo '\\''it'\\''s $HOME'\\''' '' plain_arg"),
        true
    );

    // 全安全参数不加引号
    const simple = "cap\x00build\x00--release\x00src/main.cp\x00";
    const disp2 = formatCmdlineDisplay(simple, out[0..]).?;
    expect_eq(std.mem.eql(u8, disp2, "cap build --release src/main.cp"), true);
}

test("proc_cmdline: empty display and small output buffer overflow guard") {
    let out: [64]u8 = [0; 64];
    expect_eq(formatCmdlineDisplay("", out[0..]).?.len, 0);

    let tiny: [5]u8 = [0; 5];
    expect_eq(formatCmdlineDisplay("cap\x00build\x00", tiny[0..]) == null, true);
}
