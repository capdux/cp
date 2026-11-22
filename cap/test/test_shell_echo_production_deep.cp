// Production Deep Tests for cap:shell echo formatter & escape expander (lib/shell/echo.cp)
import { test, expect_eq } from "cap:test";
import {
    EchoError,
    EchoOptions,
    EchoEscapeResult,
    expandEchoEscapes,
    formatEchoArgs,
} from "cap:shell";

test("shell echo: basic arguments, empty args, and -n / -e / -E flag precedence", fn() {
    let buf: [128]u8 = undefined;

    const empty: [0]string = [];
    expect_eq(formatEchoArgs(empty[0..], buf[0..]) catch unreachable, "\n");

    const basic = ["hello", "world"];
    expect_eq(formatEchoArgs(basic[0..], buf[0..]) catch unreachable, "hello world\n");

    const no_nl = ["-n", "hello", "world"];
    expect_eq(formatEchoArgs(no_nl[0..], buf[0..]) catch unreachable, "hello world");

    // Combined flags `-ne` and later `-E` overriding `-e`
    const override_E = ["-ne", "-E", "a\\nb"];
    expect_eq(formatEchoArgs(override_E[0..], buf[0..]) catch unreachable, "a\\nb");

    // `-E` then `-e` enables escapes
    const override_e = ["-E", "-ne", "a\\nb"];
    expect_eq(formatEchoArgs(override_e[0..], buf[0..]) catch unreachable, "a\nb");
});

test("shell echo: invalid flag stops option parsing & preserves literal argument", fn() {
    let buf: [128]u8 = undefined;

    // `-z` is not a valid echo flag -> printed literally, and subsequent `-n` is also literal
    const bad_flag = ["-z", "-n", "hi"];
    expect_eq(formatEchoArgs(bad_flag[0..], buf[0..]) catch unreachable, "-z -n hi\n");

    // `-ne1` has invalid char '1' -> entire token treated as plain argument
    const mixed = ["-ne1", "foo"];
    expect_eq(formatEchoArgs(mixed[0..], buf[0..]) catch unreachable, "-ne1 foo\n");

    // Bare `-` is not a flag
    const bare_dash = ["-"];
    expect_eq(formatEchoArgs(bare_dash[0..], buf[0..]) catch unreachable, "-\n");

    const opts = EchoOptions.parseFlags(bad_flag[0..]);
    expect_eq(opts.args_start, 0);
    expect_eq(opts.no_newline, false);
    expect_eq(opts.escape_sequences, false);
});

test("shell echo: standard control escapes (\\a, \\b, \\e/\\E, \\f, \\n, \\r, \\t, \\v, \\\\)", fn() {
    let buf: [64]u8 = undefined;
    const res = expandEchoEscapes("\\\\\\a\\b\\e\\E\\f\\n\\r\\t\\v", buf[0..]) catch unreachable;
    expect_eq(res.stopped, false);
    expect_eq(res.written, 10);
    expect_eq(buf[0], '\\');
    expect_eq(buf[1], 0x07);
    expect_eq(buf[2], 0x08);
    expect_eq(buf[3], 0x1b);
    expect_eq(buf[4], 0x1b);
    expect_eq(buf[5], 0x0c);
    expect_eq(buf[6], '\n');
    expect_eq(buf[7], '\r');
    expect_eq(buf[8], '\t');
    expect_eq(buf[9], 0x0b);
});

test("shell echo: octal (\\0NNN), hex (\\xHH), invalid \\x fallback & unknown escapes", fn() {
    let buf: [64]u8 = undefined;

    // \0101 = 'A' (65), \x42 = 'B' (66), \0 = NUL (0)
    const r1 = expandEchoEscapes("\\0101\\x42\\0!", buf[0..]) catch unreachable;
    expect_eq(r1.written, 4);
    expect_eq(buf[0], 'A');
    expect_eq(buf[1], 'B');
    expect_eq(buf[2], 0);
    expect_eq(buf[3], '!');

    // Invalid \x with no hex digits preserves literal "\x" (matches Bun echo.cp)
    const r2 = expandEchoEscapes("\\xZZ", buf[0..]) catch unreachable;
    expect_eq(buf[0..r2.written], "\\xZZ");

    // Unknown escape `\q` preserves `\q`, trailing lone `\` preserves `\`
    const r3 = expandEchoEscapes("\\q\\", buf[0..]) catch unreachable;
    expect_eq(buf[0..r3.written], "\\q\\");
});

test("shell echo: \\c stops output immediately and suppresses trailing newline & remaining args", fn() {
    let buf: [64]u8 = undefined;
    const args = ["-e", "first\\cignored", "second"];
    const out = formatEchoArgs(args[0..], buf[0..]) catch unreachable;
    expect_eq(out, "first");

    const args2 = ["-e", "one", "two\\c", "three"];
    const out2 = formatEchoArgs(args2[0..], buf[0..]) catch unreachable;
    expect_eq(out2, "one two");
});

test("shell echo: BufferTooSmall guard on formatter and escape expander", fn() {
    let tiny: [3]u8 = undefined;
    const args = ["hello"];
    let fmt_oob = false;
    * = formatEchoArgs(args[0..], tiny[0..]) catch { fmt_oob = true; };
    expect_eq(fmt_oob, true);

    let esc_oob = false;
    * = expandEchoEscapes("abcd", tiny[0..]) catch { esc_oob = true; };
    expect_eq(esc_oob, true);
});
