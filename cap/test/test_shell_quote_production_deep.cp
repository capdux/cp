import std from "std";
import { test, expect_eq } from "cap:test";
import { needsQuoting, quotePosix, joinArgs, ShellArgIter } from "cap:shell_quote";

test("1. needsQuoting and quotePosix handle safe strings, empty string, spaces, and single quotes", () => {
    expect_eq(needsQuoting("hello-world_1.2/a:b@c%d+e=f,g"), false);
    expect_eq(needsQuoting(""), true);
    expect_eq(needsQuoting("hello world"), true);
    expect_eq(needsQuoting("$HOME"), true);
    expect_eq(needsQuoting("a;b"), true);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    expect_eq(std.mem.eql(u8, quotePosix("", &buf) ?? unreachable, "''"), true);
    expect_eq(std.mem.eql(u8, quotePosix("/usr/bin/cp", &buf) ?? unreachable, "/usr/bin/cp"), true);
    expect_eq(std.mem.eql(u8, quotePosix("hello world", &buf) ?? unreachable, "'hello world'"), true);
    expect_eq(std.mem.eql(u8, quotePosix("it's", &buf) ?? unreachable, "'it'\\''s'"), true);
});

test("2. joinArgs and ShellArgIter round-trip arbitrary special characters losslessly", () => {
    const args: [5][]const u8 = [
        "grep",
        "-E",
        "it's a $test `cmd`",
        "",
        "path/with spaces/file.txt",
    ];
    let cmd_buf: [256]u8 = std.mem.zeroes([256]u8);
    const cmdline = joinArgs(&args, &cmd_buf) ?? unreachable;

    let it = ShellArgIter.init(cmdline);
    let tok_buf: [128]u8 = std.mem.zeroes([128]u8);
    let idx: usize = 0;
    while (idx < args.len) {
        const parsed = it.next(&tok_buf) ?? unreachable;
        expect_eq(std.mem.eql(u8, parsed, args[idx]), true);
        idx += 1;
    }
    expect_eq(it.next(&tok_buf) == null, true);
    expect_eq(it.hasError(), false);
});

test("3. ShellArgIter parses single quotes, double-quote escapes, backslash escapes, and adjacent concatenation", () => {
    const cmd = "foo'bar'\"baz\" \"a\\\"b\\\\c\\$d\\`e\\n\" hello\\ world ''";
    let it = ShellArgIter.init(cmd);
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    // 相邻拼接 foo'bar'"baz" -> foobarbaz
    expect_eq(std.mem.eql(u8, it.next(&buf) ?? unreachable, "foobarbaz"), true);
    // 双引号内 \" \\ \$ \` 生效转义，而 \n 保持原样 \n（POSIX 规范）
    expect_eq(std.mem.eql(u8, it.next(&buf) ?? unreachable, "a\"b\\c$d`e\\n"), true);
    // 非引号反斜杠转义空格
    expect_eq(std.mem.eql(u8, it.next(&buf) ?? unreachable, "hello world"), true);
    // 空引号产出空字符串参数
    expect_eq(std.mem.eql(u8, it.next(&buf) ?? unreachable, ""), true);
    expect_eq(it.next(&buf) == null, true);
    expect_eq(it.hasError(), false);
});

test("4. ShellArgIter flags unclosed single/double quotes and dangling backslash", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    let s1 = ShellArgIter.init("echo 'unclosed");
    expect_eq(std.mem.eql(u8, s1.next(&buf) ?? unreachable, "echo"), true);
    expect_eq(s1.next(&buf) == null, true);
    expect_eq(s1.hasError(), true);

    let s2 = ShellArgIter.init("\"unclosed double");
    expect_eq(s2.next(&buf) == null, true);
    expect_eq(s2.hasError(), true);

    let s3 = ShellArgIter.init("trailing\\");
    expect_eq(s3.next(&buf) == null, true);
    expect_eq(s3.hasError(), true);
});

test("5. buffer-too-small safety guards across quotePosix, joinArgs, and ShellArgIter", () => {
    let tiny: [3]u8 = std.mem.zeroes([3]u8);
    expect_eq(quotePosix("hello", &tiny) == null, true);
    expect_eq(quotePosix("a b", &tiny) == null, true);

    const args: [2][]const u8 = ["ab", "cd"];
    expect_eq(joinArgs(&args, &tiny) == null, true);

    let it = ShellArgIter.init("longargument");
    expect_eq(it.next(&tiny) == null, true);
    expect_eq(it.hasError(), true);
});
