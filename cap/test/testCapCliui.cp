import { test, expect_eq } from "cap:test";
import {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER,
    Align,
    alignToU8,
    Column,
    CliUi,
    formatTwoCol,
} from "../../packages/cliui/src/index.cp";

fn cuTestEq(actual: []const u8, expected: []const u8): bool {
    if (actual.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < actual.len) {
        if (actual[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn cuAppend(dst: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < dst.len) {
        dst[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn cuAppendEsc(dst: []u8, pos: usize, suffix: string): usize {
    let p: usize = pos;
    if (p < dst.len) {
        dst[p] = 27;
        p += 1;
    }
    return cuAppend(dst, p, suffix);
}

fn main() {
    test("1. @cap/cliui — Two-column CLI help menu layout & formatTwoCol", () => {
        let out: [256]u8 = undefined;
        const n: usize = formatTwoCol("-f, --file", "Path to config file", 14, 40, &out);
        expect_eq(cuTestEq(out[0..n], "-f, --file    Path to config file"), true);

        let ui: CliUi = CliUi.new(40);
        let r1: [2]Column = [
            Column.sized("-h, --help", 14),
            Column.plain("Show help"),
        ];
        let r2: [2]Column = [
            Column.sized("-v, --ver", 14),
            Column.plain("Show version"),
        ];
        ui.div(&r1);
        ui.div(&r2);
        const n2: usize = ui.render(&out);
        expect_eq(cuTestEq(out[0..n2], "-h, --help    Show help\n-v, --ver     Show version"), true);
    });

    test("2. @cap/cliui — Three-column Left, Center, Right alignment", () => {
        let ui: CliUi = CliUi.new(30);
        let cols: [3]Column = [
            Column { text: "NAME", width: 10, alignment: alignToU8(Align.Left), pad_left: 0, pad_right: 0, border: false },
            Column { text: "MID", width: 10, alignment: alignToU8(Align.Center), pad_left: 0, pad_right: 0, border: false },
            Column { text: "SIZE", width: 10, alignment: alignToU8(Align.Right), pad_left: 0, pad_right: 0, border: false },
        ];
        ui.div(&cols);

        let out: [128]u8 = undefined;
        const n: usize = ui.render(&out);
        // "NAME      " (10) + "   MID    " (3 + 3 + 4 = 10) + "      SIZE" (6 + 4 = 10)
        expect_eq(cuTestEq(out[0..n], "NAME         MID          SIZE"), true);
    });

    test("3. @cap/cliui — Multi-line wrapping across side-by-side columns", () => {
        let ui: CliUi = CliUi.new(22);
        let cols: [2]Column = [
            Column.sized("--opt", 8),
            Column.plain("Enable verbose debug logs"),
        ];
        ui.div(&cols);

        let out: [256]u8 = undefined;
        const n: usize = ui.render(&out);
        // Right col gets 14 cols: "Enable verbose" (14) / "debug logs" (10)
        expect_eq(cuTestEq(out[0..n], "--opt   Enable verbose\n        debug logs"), true);
    });

    test("4. @cap/cliui — Padding, borders, and automatic remaining width distribution", () => {
        let ui: CliUi = CliUi.new(20);
        // Col 0 fixed width 6 (pad_left=1, pad_right=1 -> inner=4), Col 1 & Col 2 auto-split 14 -> 7 each
        let cols: [3]Column = [
            Column { text: "ID", width: 6, alignment: ALIGN_LEFT, pad_left: 1, pad_right: 1, border: false },
            Column { text: "A", width: 0, alignment: ALIGN_LEFT, pad_left: 0, pad_right: 0, border: true },
            Column { text: "B", width: 0, alignment: ALIGN_RIGHT, pad_left: 0, pad_right: 0, border: true },
        ];
        ui.div(&cols);

        let out: [128]u8 = undefined;
        const n: usize = ui.render(&out);
        // Col 0: " ID   " (1 + 2 + 2 + 1 = 6)
        // Col 1: "|A    |" (1 + 1 + 4 + 1 = 7)
        // Col 2: "|    B|" (1 + 4 + 1 + 1 = 7)
        expect_eq(cuTestEq(out[0..n], " ID   |A    ||    B|"), true);
    });

    test("5. @cap/cliui — ANSI colored text preserves exact column alignment", () => {
        // Left col: "\x1b[32m--ok\x1b[0m" (visible width 4), width 10 -> 6 spaces padding
        let colored: [32]u8 = undefined;
        let cp: usize = 0;
        cp = cuAppendEsc(&colored, cp, "[32m");
        cp = cuAppend(&colored, cp, "--ok");
        cp = cuAppendEsc(&colored, cp, "[0m");

        let out: [128]u8 = undefined;
        const n: usize = formatTwoCol(colored[0..cp], "All checks passed", 10, 40, &out);

        let exp: [128]u8 = undefined;
        let ep: usize = 0;
        ep = cuAppendEsc(&exp, ep, "[32m");
        ep = cuAppend(&exp, ep, "--ok");
        ep = cuAppendEsc(&exp, ep, "[0m");
        ep = cuAppend(&exp, ep, "      All checks passed");

        expect_eq(cuTestEq(out[0..n], exp[0..ep]), true);
    });

    test("6. @cap/cliui — span horizontal composition and resetOutput reuse", () => {
        let ui: CliUi = CliUi.new(30);
        let s1: [1]Column = [Column.sized("[INFO]", 8)];
        let s2: [1]Column = [Column.plain("Server ready")];
        ui.span(&s1);
        ui.span(&s2);

        let d1: [1]Column = [Column.plain("Listening on :8080")];
        ui.div(&d1);

        let out: [128]u8 = undefined;
        const n1: usize = ui.render(&out);
        expect_eq(cuTestEq(out[0..n1], "[INFO]  Server ready\nListening on :8080"), true);

        // Reuse the same CliUi buffer after resetOutput()
        ui.resetOutput();
        expect_eq(ui.len, 0);
        let d2: [1]Column = [Column.plain("Fresh output")];
        ui.div(&d2);
        const n2: usize = ui.render(&out);
        expect_eq(cuTestEq(out[0..n2], "Fresh output"), true);
    });
}
