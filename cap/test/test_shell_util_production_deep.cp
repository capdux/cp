import { test, expect_eq } from "cap:test";
import {
    isSafeUnquoted,
    quotePosixBuf,
    escapeDoubleQuoted,
    joinCommandBuf,
    posixBasename,
    posixDirname,
    isValidEnvVarName,
    splitEnvAssignment,
} from "cap:shell";

test("1. isSafeUnquoted & quotePosixBuf (empty, safe, spaces, single quotes, subshell chars)", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    expect_eq(isSafeUnquoted("hello_world-1.2/foo@bar:80"), true);
    expect_eq(isSafeUnquoted(""), false);
    expect_eq(isSafeUnquoted("hello world"), false);
    expect_eq(isSafeUnquoted("$(id)"), false);

    // Empty string -> ''
    expect_eq(std.mem.eql(u8, quotePosixBuf("", buf[0..]).?, "''"), true);
    // Safe unquoted -> verbatim
    expect_eq(std.mem.eql(u8, quotePosixBuf("/usr/bin/env", buf[0..]).?, "/usr/bin/env"), true);
    // Spaces & shell metachars -> single quoted
    expect_eq(std.mem.eql(u8, quotePosixBuf("hello $(world)", buf[0..]).?, "'hello $(world)'"), true);
    // Embedded single quote -> '\''
    expect_eq(std.mem.eql(u8, quotePosixBuf("it's", buf[0..]).?, "'it'\\''s'"), true);
    // Buffer too small -> null
    expect_eq(quotePosixBuf("hello world", buf[0..5]) == null, true);
});

test("2. escapeDoubleQuoted & joinCommandBuf", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const esc = escapeDoubleQuoted("a$b`c\"d\\e", buf[0..]).?;
    expect_eq(std.mem.eql(u8, esc, "a\\$b\\`c\\\"d\\\\e"), true);
    expect_eq(escapeDoubleQuoted("$HOME", buf[0..3]) == null, true);

    const args: [4][]const u8 = ["echo", "hello world", "", "a'b"];
    const cmd = joinCommandBuf(args[0..], buf[0..]).?;
    expect_eq(std.mem.eql(u8, cmd, "echo 'hello world' '' 'a'\\''b'"), true);

    const empty_args: [0][]const u8 = [];
    expect_eq(std.mem.eql(u8, joinCommandBuf(empty_args[0..], buf[0..]).?, ""), true);
});

test("3. posixBasename full POSIX edge cases & suffix stripping", () => {
    expect_eq(std.mem.eql(u8, posixBasename("", ""), "."), true);
    expect_eq(std.mem.eql(u8, posixBasename("/", ""), "/"), true);
    expect_eq(std.mem.eql(u8, posixBasename("///", ""), "/"), true);
    expect_eq(std.mem.eql(u8, posixBasename("a/b/", ""), "b"), true);
    expect_eq(std.mem.eql(u8, posixBasename("/usr/local/bin/cp", ""), "cp"), true);
    expect_eq(std.mem.eql(u8, posixBasename("foo.tar.gz", ".gz"), "foo.tar"), true);
    expect_eq(std.mem.eql(u8, posixBasename("foo.tar.gz", ".zip"), "foo.tar.gz"), true);
    // Suffix equal to entire basename must NOT be stripped per POSIX
    expect_eq(std.mem.eql(u8, posixBasename(".gz", ".gz"), ".gz"), true);
});

test("4. posixDirname full POSIX edge cases", () => {
    expect_eq(std.mem.eql(u8, posixDirname(""), "."), true);
    expect_eq(std.mem.eql(u8, posixDirname("foo"), "."), true);
    expect_eq(std.mem.eql(u8, posixDirname("/"), "/"), true);
    expect_eq(std.mem.eql(u8, posixDirname("///"), "/"), true);
    expect_eq(std.mem.eql(u8, posixDirname("/foo"), "/"), true);
    expect_eq(std.mem.eql(u8, posixDirname("/foo/"), "/"), true);
    expect_eq(std.mem.eql(u8, posixDirname("a/b/"), "a"), true);
    expect_eq(std.mem.eql(u8, posixDirname("/a///b//"), "/a"), true);
    expect_eq(std.mem.eql(u8, posixDirname("/usr/local/bin"), "/usr/local"), true);
});

test("5. isValidEnvVarName & splitEnvAssignment", () => {
    expect_eq(isValidEnvVarName("PATH"), true);
    expect_eq(isValidEnvVarName("_PRIVATE_1"), true);
    expect_eq(isValidEnvVarName(""), false);
    expect_eq(isValidEnvVarName("1ABC"), false);
    expect_eq(isValidEnvVarName("FOO-BAR"), false);

    const a1 = splitEnvAssignment("FOO=bar=baz").?;
    expect_eq(std.mem.eql(u8, a1.key, "FOO"), true);
    expect_eq(std.mem.eql(u8, a1.value, "bar=baz"), true);

    const a2 = splitEnvAssignment("EMPTY=").?;
    expect_eq(std.mem.eql(u8, a2.key, "EMPTY"), true);
    expect_eq(std.mem.eql(u8, a2.value, ""), true);

    expect_eq(splitEnvAssignment("NO_EQUALS") == null, true);
    expect_eq(splitEnvAssignment("1BAD=val") == null, true);
    expect_eq(splitEnvAssignment("=val") == null, true);
});
