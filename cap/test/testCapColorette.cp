import { test, expect_eq } from "cap:test";
import {
    isColorSupported,
    ColoretteOptions,
    Colorette,
    createColors,
    createColorsWithOptions
} from "../../packages/@cap/colorette/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/colorette — isColorSupported respects NO_COLOR, FORCE_COLOR, TERM=dumb, and TTY", () => {
        expect_eq(isColorSupported("1", "1", "xterm-256color", true), false);
        expect_eq(isColorSupported("", "1", "dumb", false), true);
        expect_eq(isColorSupported("", "0", "xterm-256color", true), false);
        expect_eq(isColorSupported("", "", "dumb", true), false);
        expect_eq(isColorSupported("", "", "xterm-256color", true), true);
        expect_eq(isColorSupported("", "", "xterm-256color", false), false);
    });

    test("2. @cap/colorette — modifiers and standard foreground colors", () => {
        let c = createColors(true);
        let buf: [128]u8 = undefined;

        let n1 = c.bold("hello", buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n1, "\x1b[1mhello\x1b[22m"), true);

        let n2 = c.italic("world", buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n2, "\x1b[3mworld\x1b[23m"), true);

        let n3 = c.underline("link", buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n3, "\x1b[4mlink\x1b[24m"), true);

        let n4 = c.red("err", buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n4, "\x1b[31merr\x1b[39m"), true);

        let n5 = c.gray("muted", buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n5, "\x1b[90mmuted\x1b[39m"), true);
    });

    test("3. @cap/colorette — bright foreground and background colors", () => {
        let c = createColors(true);
        let buf: [128]u8 = undefined;

        let n1 = c.redBright("warn", buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n1, "\x1b[91mwarn\x1b[39m"), true);

        let n2 = c.cyanBright("info", buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n2, "\x1b[96minfo\x1b[39m"), true);

        let n3 = c.bgBlue("badge", buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n3, "\x1b[44mbadge\x1b[49m"), true);

        let n4 = c.bgGreenBright("ok", buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n4, "\x1b[102mok\x1b[49m"), true);
    });

    test("4. @cap/colorette — nested close-code recovery for colors and bold/dim", () => {
        let c = createColors(true);
        let inner: [64]u8 = undefined;
        let outer: [128]u8 = undefined;

        // Nested foreground color: red("a " + blue("b") + " c")
        let n_blue = c.blue("b", inner[0..64]);
        expect_eq(eqBytes(inner[0..64], n_blue, "\x1b[34mb\x1b[39m"), true);

        let n_red = c.red("a \x1b[34mb\x1b[39m c", outer[0..128]);
        expect_eq(eqBytes(outer[0..128], n_red, "\x1b[31ma \x1b[34mb\x1b[31m c\x1b[39m"), true);

        // Nested bold containing dim close code (\x1b[22m -> \x1b[22m\x1b[1m)
        let n_bold = c.bold("hi \x1b[2mdim\x1b[22m end", outer[0..128]);
        expect_eq(eqBytes(outer[0..128], n_bold, "\x1b[1mhi \x1b[2mdim\x1b[22m\x1b[1m end\x1b[22m"), true);
    });

    test("5. @cap/colorette — disabled mode passes text through verbatim", () => {
        let opts = ColoretteOptions.fromEnv("1", "", "xterm-256color", true);
        let c = createColorsWithOptions(opts);
        expect_eq(c.isEnabled(), false);

        let buf: [64]u8 = undefined;
        let n1 = c.red("plain", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "plain"), true);

        let n2 = c.bgYellow("raw", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "raw"), true);
    });

    test("6. @cap/colorette — empty input and small buffer safe truncation", () => {
        let c = Colorette.new(true);
        let buf: [8]u8 = undefined;

        let n_empty = c.green("", buf[0..8]);
        expect_eq(n_empty, 0);

        let n_trunc = c.green("abcdefghij", buf[0..8]);
        expect_eq(n_trunc, 8);
        expect_eq(eqBytes(buf[0..8], n_trunc, "\x1b[32mabc"), true);
    });
}
