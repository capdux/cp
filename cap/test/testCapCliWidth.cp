import { test, expect_eq } from "cap:test";
import {
    CliWidthOptions,
    TtyStreamInfo,
    CliWidthConfig,
    defaultCliWidth,
    clampCliWidth,
    parseColumnsEnv,
    parseColumnsFromEnvList,
    resolveCliWidth,
    cliWidth,
} from "../../packages/cli-width/src/index.cp";

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
        expect_eq(resolveCliWidth(132, "90", opts), 132);
        expect_eq(resolveCliWidth(0, "90", opts), 90);
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
        expect_eq(clampCliWidth(50, 60, 100), 60);
    });

    test("7. @cap/cli-width — cliWidth(CliWidthConfig) full output > tty > env > default precedence", () => {
        let cfg = CliWidthConfig.defaults();
        expect_eq(cliWidth(cfg), 80);

        cfg.default_width = 100;
        expect_eq(cliWidth(cfg), 100);

        cfg.env_columns = "110";
        expect_eq(cliWidth(cfg), 110);

        cfg.tty = TtyStreamInfo.tty(128, 40);
        expect_eq(cliWidth(cfg), 128);

        cfg.output = TtyStreamInfo.tty(160, 48);
        expect_eq(cliWidth(cfg), 160);

        // Non-TTY output stream is ignored
        cfg.output = TtyStreamInfo { is_tty: false, columns: 200, rows: 50 };
        expect_eq(cliWidth(cfg), 128);

        // Clamping applies to cliWidth too
        cfg.max_width = 120;
        expect_eq(cliWidth(cfg), 120);
    });

    test("8. @cap/cli-width — parseColumnsFromEnvList scans KEY=VALUE arrays", () => {
        const env1: [4]string = ["HOME=/root", "COLUMNS=92", "TERM=xterm", "COLUMNS=144"];
        expect_eq(parseColumnsFromEnvList(env1[0..4]).?, 144);

        const env2: [2]string = ["TERM=xterm", "COLUMNS=bad"];
        expect_eq(parseColumnsFromEnvList(env2[0..2]) == null, true);

        const defOpts = CliWidthOptions.defaults();
        expect_eq(defOpts.default_width, 80);
    });
}
