import std from "std";
import { test, expect_eq } from "cap:test";
import { AnsiColor, AnsiStyle, stripAnsi, visibleLen, hasAnsi } from "cap:ansi_style";

test("1. AnsiColor 16-color fg/bg SGR codes", () => {
    expect_eq(AnsiColor.black.fgCode(), u8(30));
    expect_eq(AnsiColor.red.fgCode(), u8(31));
    expect_eq(AnsiColor.white.fgCode(), u8(37));
    expect_eq(AnsiColor.bright_black.fgCode(), u8(90));
    expect_eq(AnsiColor.bright_cyan.fgCode(), u8(96));
    expect_eq(AnsiColor.bright_white.fgCode(), u8(97));

    expect_eq(AnsiColor.black.bgCode(), u8(40));
    expect_eq(AnsiColor.green.bgCode(), u8(42));
    expect_eq(AnsiColor.bright_black.bgCode(), u8(100));
    expect_eq(AnsiColor.bright_yellow.bgCode(), u8(103));
    expect_eq(AnsiColor.bright_white.bgCode(), u8(107));
});

test("2. AnsiStyle format with bold, underline, fg/bg, and empty-style passthrough", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    // 空样式直接返回原文本
    const empty = AnsiStyle{};
    expect_eq(empty.isEmpty(), true);
    const passthrough = empty.format("plain", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, passthrough, "plain"), true);
    expect_eq(hasAnsi(passthrough), false);

    // 粗体 + 红色
    const err_style = AnsiStyle{ fg: AnsiColor.red, bold: true };
    const s1 = err_style.format("FAIL", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s1, "\x1b[1;31mFAIL\x1b[0m"), true);
    expect_eq(hasAnsi(s1), true);
    expect_eq(visibleLen(s1), usize(4));

    // 完整修饰 + 亮青前景 + 蓝色背景
    const rich = AnsiStyle{
        fg: AnsiColor.bright_cyan,
        bg: AnsiColor.blue,
        bold: true,
        italic: true,
        underline: true,
        strikethrough: true,
    };
    const s2 = rich.format("CP", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s2, "\x1b[1;3;4;9;96;44mCP\x1b[0m"), true);
    expect_eq(visibleLen(s2), usize(2));
});

test("3. AnsiStyle.formatRgb 24-bit TrueColor output", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const out = AnsiStyle.formatRgb(255, 128, 0, "orange", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, out, "\x1b[38;2;255;128;0morange\x1b[0m"), true);
    expect_eq(visibleLen(out), usize(6));
    expect_eq(hasAnsi(out), true);

    const black = AnsiStyle.formatRgb(0, 9, 10, "x", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, black, "\x1b[38;2;0;9;10mx\x1b[0m"), true);
});

test("4. stripAnsi and visibleLen handle CSI and OSC 8 hyperlinks (BEL and ST terminators)", () => {
    let out: [128]u8 = std.mem.zeroes([128]u8);

    // CSI 颜色与清行序列
    const csi = "\x1b[1;32mPASS\x1b[0m \x1b[2Kdone";
    const stripped_csi = stripAnsi(csi, &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, stripped_csi, "PASS done"), true);
    expect_eq(visibleLen(csi), usize(9));

    // OSC 8 超链接（BEL \x07 终止）
    const osc_bel = "\x1b]8;;https://cp-lang.dev\x07CP Docs\x1b]8;;\x07";
    const stripped_bel = stripAnsi(osc_bel, &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, stripped_bel, "CP Docs"), true);
    expect_eq(visibleLen(osc_bel), usize(7));
    expect_eq(hasAnsi(osc_bel), true);

    // OSC 8 超链接（ST \x1b\\ 终止）+ 内部 SGR 嵌套
    const osc_st = "Link: \x1b]8;;https://example.com\x1b\\\x1b[4;34mClick\x1b[0m\x1b]8;;\x1b\\!";
    const stripped_st = stripAnsi(osc_st, &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, stripped_st, "Link: Click!"), true);
    expect_eq(visibleLen(osc_st), usize(12));
});

test("5. buffer overflow guards and plain/empty input edge cases", () => {
    let tiny: [4]u8 = std.mem.zeroes([4]u8);

    const style = AnsiStyle{ fg: AnsiColor.green, bold: true };
    expect_eq(style.format("hello", &tiny) == null, true);
    expect_eq(AnsiStyle.formatRgb(10, 20, 30, "hello", &tiny) == null, true);

    // stripAnsi 缓冲区不足返回 null
    expect_eq(stripAnsi("\x1b[31mhello\x1b[0m", &tiny) == null, true);

    // 刚好容下 5 字节的缓冲成功
    let exact: [5]u8 = std.mem.zeroes([5]u8);
    const ok = stripAnsi("\x1b[31mhello\x1b[0m", &exact) ?? unreachable;
    expect_eq(std.mem.eql(u8, ok, "hello"), true);

    // 空输入与无 ANSI 输入
    expect_eq(visibleLen(""), usize(0));
    expect_eq(hasAnsi(""), false);
    expect_eq(hasAnsi("hello world"), false);
    expect_eq(visibleLen("hello world"), usize(11));
});
