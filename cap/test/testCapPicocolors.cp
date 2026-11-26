// cap/test/testCapPicocolors.cp
// Comprehensive unit tests for @cap/picocolors (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    PICO_BUF_CAP,
    detectColorSupport,
    ColorConfig,
    PicoFormatter,
    PicoBuf
} from "../../packages/@cap/picocolors/src/index.cp";

fn main() {
    test("1. detectColorSupport & ColorConfig — NO_COLOR, FORCE_COLOR & TTY precedence", () => {
        expect_eq(detectColorSupport("", "", true), true);
        expect_eq(detectColorSupport("", "", false), false);
        expect_eq(detectColorSupport("1", "", true), false);
        expect_eq(detectColorSupport("", "1", false), true);
        expect_eq(detectColorSupport("1", "1", false), true);
        expect_eq(detectColorSupport("", "0", true), false);

        const c1 = ColorConfig.fromEnv("1", "", true);
        expect_eq(c1.is_color_supported, false);
        const c2 = ColorConfig.fromEnv("", "1", false);
        expect_eq(c2.is_color_supported, true);
    });

    test("2. PicoFormatter — foreground, background & modifier ANSI wrapping", () => {
        const fmt = PicoFormatter.new(true);
        let out: [128]u8 = undefined;

        const n1 = fmt.red("error", out[0..128]);
        expect_eq(out[0..n1], "\x1b[31merror\x1b[39m");

        const n2 = fmt.green("ok", out[0..128]);
        expect_eq(out[0..n2], "\x1b[32mok\x1b[39m");

        const n3 = fmt.bold("title", out[0..128]);
        expect_eq(out[0..n3], "\x1b[1mtitle\x1b[22m");

        const n4 = fmt.gray("muted", out[0..128]);
        expect_eq(out[0..n4], "\x1b[90mmuted\x1b[39m");

        const n5 = fmt.bgYellow("warn", out[0..128]);
        expect_eq(out[0..n5], "\x1b[43mwarn\x1b[49m");

        const n6 = fmt.underline("link", out[0..128]);
        expect_eq(out[0..n6], "\x1b[4mlink\x1b[24m");
    });

    test("3. PicoFormatter — disabled mode passes text through without ANSI codes", () => {
        const fmt = PicoFormatter.fromConfig(ColorConfig.disabled());
        let out: [128]u8 = undefined;

        expect_eq(fmt.isColorSupported(), false);
        const n1 = fmt.red("plain error", out[0..128]);
        expect_eq(out[0..n1], "plain error");

        const n2 = fmt.bold("plain bold", out[0..128]);
        expect_eq(out[0..n2], "plain bold");

        // Empty input returns 0 bytes even when enabled
        const fmtOn = PicoFormatter.new(true);
        expect_eq(fmtOn.cyan("", out[0..128]), 0 as usize);
    });

    test("4. PicoFormatter — nested close-code recovery (red wrapping blue, bold wrapping dim)", () => {
        const fmt = PicoFormatter.new(true);
        let inner: [64]u8 = undefined;
        let combined: [128]u8 = undefined;
        let outer: [256]u8 = undefined;

        // Build "foo " + blue("bar") + " baz"
        const bLen = fmt.blue("bar", inner[0..64]);
        let pos: usize = 0;
        const pfx = "foo ";
        let i: usize = 0;
        while (i < pfx.len) { combined[pos] = pfx[i]; pos = pos + 1; i = i + 1; }
        i = 0;
        while (i < bLen) { combined[pos] = inner[i]; pos = pos + 1; i = i + 1; }
        const sfx = " baz";
        i = 0;
        while (i < sfx.len) { combined[pos] = sfx[i]; pos = pos + 1; i = i + 1; }

        const rLen = fmt.red(combined[0..pos], outer[0..256]);
        expect_eq(outer[0..rLen], "\x1b[31mfoo \x1b[34mbar\x1b[31m baz\x1b[39m");

        // Bold wrapping dim recovers bold after shared \x1b[22m close
        const dLen = fmt.dim("soft", inner[0..64]);
        const bdLen = fmt.bold(inner[0..dLen], outer[0..256]);
        expect_eq(outer[0..bdLen], "\x1b[1m\x1b[2msoft\x1b[22m\x1b[1m\x1b[22m");
    });

    test("5. PicoBuf — zero-heap chained accumulation in enabled & disabled modes", () => {
        let pb = PicoBuf.new(true);
        pb.bold("CAP");
        pb.pushPlain(" ");
        pb.green("PASS");
        pb.pushPlain(" ");
        pb.bgRed("ERR");
        expect_eq(
            pb.buf[0..pb.len],
            "\x1b[1mCAP\x1b[22m \x1b[32mPASS\x1b[39m \x1b[41mERR\x1b[49m"
        );

        pb.clear();
        expect_eq(pb.len, 0 as usize);

        let plain = PicoBuf.fromConfig(ColorConfig.disabled());
        plain.bold("CAP");
        plain.pushPlain(" ");
        plain.cyan("OK");
        expect_eq(plain.buf[0..plain.len], "CAP OK");
    });

    test("6. Buffer truncation safety — tiny output slice & PicoBuf 1024-byte cap", () => {
        const fmt = PicoFormatter.new(true);
        let tiny: [8]u8 = undefined;
        const n = fmt.magenta("long-text-here", tiny[0..8]);
        expect_eq(n, 8 as usize);
        expect_eq(tiny[0..8], "\x1b[35mlon");

        let pb = PicoBuf.new(false);
        let k: usize = 0;
        while (k < 40) {
            pb.pushPlain("01234567890123456789012345678901"); // 32 bytes * 40 = 1280 > 1024
            k = k + 1;
        }
        expect_eq(pb.len, PICO_BUF_CAP);
    });
}
