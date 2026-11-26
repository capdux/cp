import { test, expect_eq } from "cap:test";
import {
    CliWidthOptions,
    defaultCliWidth,
    parseColumnsEnv,
    resolveCliWidth,
} from "../../packages/@cap/cli-width/src/index.cp";

fn main() {
    test("1. @cap/cli-width — defaultCliWidth() and parseColumnsEnv() valid inputs", () => {
        expect_eq(defaultCliWidth(), 80);
        expect_eq(parseColumnsEnv("120").?, 120);
        expect_eq(parseColumnsEnv("  96 \n").?, 96);
        expect_eq(parseColumnsEnv("1").?, 1);
    });

    test("2. @cap/cli-width — parseColumnsEnv() rejects invalid/zero/empty/overflow", () => {
        expect_eq(parseColumnsEnv("") == null, true);
        expect_eq(parseColumnsEnv("   ") == null, true);
        expect_eq(parseColumnsEnv("0") == null, true);
        expect_eq(parseColumnsEnv("-80") == null, true);
        expect_eq(parseColumnsEnv("80px") == null, true);
        expect_eq(parseColumnsEnv("99999999999") == null, true);
    });

    test("3. @cap/cli-width — Priority: tty_cols > COLUMNS env > default_width", () => {
        const opts: CliWidthOptions = CliWidthOptions {
            default_width: 100,
            min_width: 0,
            max_width: 0,
        };
        // TTY wins over COLUMNS and default
        expect_eq(resolveCliWidth(132, "90", opts), 132);
        // COLUMNS wins when tty_cols == 0
        expect_eq(resolveCliWidth(0, "90", opts), 90);
        // default_width wins when both tty_cols == 0 and COLUMNS invalid
        expect_eq(resolveCliWidth(0, "", opts), 100);
        expect_eq(resolveCliWidth(0, "invalid", opts), 100);
    });

    test("4. @cap/cli-width — Fallback to 80 when default_width is 0", () => {
        const opts: CliWidthOptions = CliWidthOptions {
            default_width: 0,
            min_width: 0,
            max_width: 0,
        };
        expect_eq(resolveCliWidth(0, "", opts), 80);
    });

    test("5. @cap/cli-width — Clamping with min_width and max_width", () => {
        const opts: CliWidthOptions = CliWidthOptions {
            default_width: 80,
            min_width: 40,
            max_width: 120,
        };
        expect_eq(resolveCliWidth(20, "", opts), 40);
        expect_eq(resolveCliWidth(200, "", opts), 120);
        expect_eq(resolveCliWidth(0, "30", opts), 40);
        expect_eq(resolveCliWidth(0, "180", opts), 120);
        expect_eq(resolveCliWidth(0, "88", opts), 88);
    });

    test("6. @cap/cli-width — Single-sided min or max bound clamping", () => {
        const min_only: CliWidthOptions = CliWidthOptions {
            default_width: 80,
            min_width: 60,
            max_width: 0,
        };
        expect_eq(resolveCliWidth(30, "", min_only), 60);
        expect_eq(resolveCliWidth(240, "", min_only), 240);

        const max_only: CliWidthOptions = CliWidthOptions {
            default_width: 80,
            min_width: 0,
            max_width: 100,
        };
        expect_eq(resolveCliWidth(20, "", max_only), 20);
        expect_eq(resolveCliWidth(150, "", max_only), 100);
    });
}
