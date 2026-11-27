import { test, expect_eq } from "cap:test";
import {
    SPINNER_DOTS,
    SPINNER_LINE,
    SPINNER_ARC,
    SPINNER_BOUNCE,
    SPINNER_ARROW,
    Spinner,
} from "../../packages/ora/src/index.cp";
import {
    cursorHide,
    cursorShow,
    eraseLine,
    cursorLeft,
} from "../../packages/ansi-escapes/src/index.cp";

fn main() {
    test("1. @cap/ora-spinner — Frame counts across all 5 built-in spinner styles", () => {
        expect_eq(Spinner.new(SPINNER_DOTS, "Loading").frameCount(), 10);
        expect_eq(Spinner.new(SPINNER_LINE, "Working").frameCount(), 4);
        expect_eq(Spinner.new(SPINNER_ARC, "Building").frameCount(), 6);
        expect_eq(Spinner.new(SPINNER_BOUNCE, "Syncing").frameCount(), 4);
        expect_eq(Spinner.new(SPINNER_ARROW, "Routing").frameCount(), 8);
    });

    test("2. @cap/ora-spinner — Frame progression and wrap-around via tick()", () => {
        let sp = Spinner.new(SPINNER_LINE, "Compiling");
        expect_eq(sp.currentFrame(), "-");
        expect_eq(sp.tick(), "\\");
        expect_eq(sp.tick(), "|");
        expect_eq(sp.tick(), "/");
        expect_eq(sp.tick(), "-"); // wraps back to frame 0

        let dots = Spinner.new(SPINNER_DOTS, "Init");
        expect_eq(dots.currentFrame(), "⠋");
        expect_eq(dots.tick(), "⠙");
    });

    test("3. @cap/ora-spinner — renderFrame() plain vs ANSI (CURSOR_LEFT + ERASE_LINE)", () => {
        let sp = Spinner.new(SPINNER_LINE, "Downloading");
        let out: [128]u8 = undefined;

        const n1 = sp.renderFrame(false, out[0..128]);
        expect_eq(out[0..n1], "- Downloading");

        sp.setText("Installing");
        sp.tick();
        const n2 = sp.renderFrame(true, out[0..128]);
        expect_eq(out[0..n2], "\x1b[G\x1b[2K\\ Installing");
    });

    test("4. @cap/ora-spinner — Status line renderers (succeed/fail/warn/info)", () => {
        const sp = Spinner.new(SPINNER_DOTS, "Default task");
        let out: [128]u8 = undefined;

        const n1 = sp.renderSucceed("Build succeeded", out[0..128]);
        expect_eq(out[0..n1], "✔ Build succeeded\n");

        const n2 = sp.renderFail("Tests failed", out[0..128]);
        expect_eq(out[0..n2], "✖ Tests failed\n");

        const n3 = sp.renderWarn("Deprecated flag", out[0..128]);
        expect_eq(out[0..n3], "⚠ Deprecated flag\n");

        const n4 = sp.renderInfo("Listening on :3000", out[0..128]);
        expect_eq(out[0..n4], "ℹ Listening on :3000\n");

        // Empty msg falls back to spinner text
        const n5 = sp.renderSucceed("", out[0..128]);
        expect_eq(out[0..n5], "✔ Default task\n");
    });

    test("5. @cap/ora-spinner — Cursor hide/show sequences reuse @cap/ansi-escapes", () => {
        const sp = Spinner.new(SPINNER_ARC, "Waiting");
        expect_eq(sp.hideCursorSeq(), cursorHide());
        expect_eq(sp.showCursorSeq(), cursorShow());
        expect_eq(cursorLeft(), "\x1b[G");
        expect_eq(eraseLine(), "\x1b[2K");
    });

    test("6. @cap/ora-spinner — ARC/BOUNCE/ARROW glyphs & small buffer safety", () => {
        let arc = Spinner.new(SPINNER_ARC, "");
        expect_eq(arc.currentFrame(), "◜");
        expect_eq(arc.tick(), "◠");

        let arr = Spinner.new(SPINNER_ARROW, "Go");
        expect_eq(arr.currentFrame(), "←");
        expect_eq(arr.tick(), "↖");

        let tiny: [5]u8 = undefined;
        const long_sp = Spinner.new(SPINNER_LINE, "Long message here");
        const n = long_sp.renderFrame(false, tiny[0..5]);
        expect_eq(n, 5);
        expect_eq(tiny[0..5], "- Lon");
    });
}
