// Source: Absorbed from /data/dev/bun/src/ini.zig (INI Config Safe Writer & Escaper)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    isValidIniKey,
    needsIniQuotes,
    appendIniSection,
    appendIniSubsection,
    appendIniPair,
    appendIniComment,
} from "cap:ini_writer";

test("1. isValidIniKey validates legal keys and rejects forbidden chars or edge whitespace", () => {
    expect_eq(isValidIniKey("host"), true);
    expect_eq(isValidIniKey("app.db-port_1"), true);
    expect_eq(isValidIniKey("user name"), true);

    expect_eq(isValidIniKey(""), false);
    expect_eq(isValidIniKey(" lead"), false);
    expect_eq(isValidIniKey("trail "), false);
    expect_eq(isValidIniKey("\ttab"), false);
    expect_eq(isValidIniKey("a=b"), false);
    expect_eq(isValidIniKey("a[0]"), false);
    expect_eq(isValidIniKey("a]b"), false);
    expect_eq(isValidIniKey("a;b"), false);
    expect_eq(isValidIniKey("a#b"), false);
    expect_eq(isValidIniKey("a\nb"), false);
    expect_eq(isValidIniKey("a\rb"), false);
});

test("2. needsIniQuotes detects edge whitespace, comment chars, quotes, backslashes, and controls", () => {
    expect_eq(needsIniQuotes(""), false);
    expect_eq(needsIniQuotes("simple_value-123"), false);
    expect_eq(needsIniQuotes("hello world"), false);
    expect_eq(needsIniQuotes("https://example.com:8080/path"), false);

    expect_eq(needsIniQuotes(" leading"), true);
    expect_eq(needsIniQuotes("trailing "), true);
    expect_eq(needsIniQuotes("\tindent"), true);
    expect_eq(needsIniQuotes("a;b"), true);
    expect_eq(needsIniQuotes("color#fff"), true);
    expect_eq(needsIniQuotes("say \"hi\""), true);
    expect_eq(needsIniQuotes("C:\\Users\\cp"), true);
    expect_eq(needsIniQuotes("line1\nline2"), true);
    expect_eq(needsIniQuotes("cr\rhere"), true);
    expect_eq(needsIniQuotes("a\tb"), true);
});

test("3. appendIniSection, appendIniSubsection, and appendIniPair produce valid INI and gitconfig", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let pos: usize = 0;

    expect_eq(appendIniSection(&buf, *pos, "core"), true);
    expect_eq(appendIniPair(&buf, *pos, "editor", "vim"), true);
    expect_eq(appendIniPair(&buf, *pos, "empty", ""), true);
    expect_eq(appendIniSubsection(&buf, *pos, "remote", "my \"origin\"\\1"), true);
    expect_eq(appendIniPair(&buf, *pos, "msg", "a\"b\\c\nd\re\tf"), true);
    expect_eq(appendIniPair(&buf, *pos, "padded", " hi "), true);

    const expected =
        "[core]\n"
        ++ "editor=vim\n"
        ++ "empty=\n"
        ++ "[remote \"my \\\"origin\\\"\\\\1\"]\n"
        ++ "msg=\"a\\\"b\\\\c\\nd\\re\\tf\"\n"
        ++ "padded=\" hi \"\n";

    expect_eq(std.mem.eql(u8, buf[0..pos], expected), true);
});

test("4. appendIniComment prefixes single-line, multiline LF/CRLF, and empty comments", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let pos: usize = 0;

    expect_eq(appendIniComment(&buf, *pos, "single line"), true);
    expect_eq(appendIniComment(&buf, *pos, "line 1\nline 2\r\nline 3"), true);
    expect_eq(appendIniComment(&buf, *pos, ""), true);

    const expected =
        "# single line\n"
        ++ "# line 1\n"
        ++ "# line 2\n"
        ++ "# line 3\n"
        ++ "# \n";

    expect_eq(std.mem.eql(u8, buf[0..pos], expected), true);
});

test("5. Atomic rollback of pos on buffer exhaustion and rejection of malformed section/key/subsection", () => {
    let buf: [16]u8 = std.mem.zeroes([16]u8);
    let pos: usize = 0;

    // 写入 6 字节 "[ok]\n" (5 bytes)
    expect_eq(appendIniSection(&buf, *pos, "ok"), true);
    expect_eq(pos, 5);

    // 非法输入不修改 pos
    expect_eq(appendIniSection(&buf, *pos, ""), false);
    expect_eq(appendIniSection(&buf, *pos, "bad]sec"), false);
    expect_eq(appendIniSubsection(&buf, *pos, "ok", "bad\nsub"), false);
    expect_eq(appendIniPair(&buf, *pos, "bad=key", "v"), false);
    expect_eq(pos, 5);

    // 缓冲不足时原子回滚 pos
    expect_eq(appendIniPair(&buf, *pos, "long_key", "very_long_value"), false);
    expect_eq(pos, 5);
    expect_eq(appendIniSection(&buf, *pos, "very_long_section_name"), false);
    expect_eq(pos, 5);
    expect_eq(appendIniSubsection(&buf, *pos, "sec", "very_long_sub"), false);
    expect_eq(pos, 5);
    expect_eq(appendIniComment(&buf, *pos, "this comment exceeds 16 bytes"), false);
    expect_eq(pos, 5);

    // 缓冲余量充足的短写入仍可正常追加
    expect_eq(appendIniPair(&buf, *pos, "k", "v"), true);
    expect_eq(std.mem.eql(u8, buf[0..pos], "[ok]\nk=v\n"), true);
});
