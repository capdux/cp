import { test, expect_eq } from "cap:test";
import { EnvInfo } from "../../packages/is-unicode-supported/src/index.cp";
import {
    FigureKind,
    figureFor,
    figureForEnv,
    replaceSymbols,
} from "../../packages/figures/src/index.cp";

fn main() {
    test("1. @cap/figures — figureFor Unicode symbols", () => {
        expect_eq(figureFor(FigureKind.Tick, true), "✔");
        expect_eq(figureFor(FigureKind.Cross, true), "✖");
        expect_eq(figureFor(FigureKind.Pointer, true), "❯");
        expect_eq(figureFor(FigureKind.Ellipsis, true), "…");
        expect_eq(figureFor(FigureKind.Warning, true), "⚠");
        expect_eq(figureFor(FigureKind.CheckboxOn, true), "☒");
        expect_eq(figureFor(FigureKind.CheckboxOff, true), "☐");
    });

    test("2. @cap/figures — figureFor ASCII/fallback symbols", () => {
        expect_eq(figureFor(FigureKind.Tick, false), "√");
        expect_eq(figureFor(FigureKind.Cross, false), "×");
        expect_eq(figureFor(FigureKind.Star, false), "*");
        expect_eq(figureFor(FigureKind.Info, false), "i");
        expect_eq(figureFor(FigureKind.Pointer, false), ">");
        expect_eq(figureFor(FigureKind.Ellipsis, false), "...");
        expect_eq(figureFor(FigureKind.RadioOn, false), "(*)");
        expect_eq(figureFor(FigureKind.RadioOff, false), "( )");
        expect_eq(figureFor(FigureKind.CheckboxOff, false), "[ ]");
    });

    test("3. @cap/figures — figureForEnv integrates @cap/is-unicode-supported", () => {
        const posix_env: EnvInfo = EnvInfo.posix();
        const win_legacy: EnvInfo = EnvInfo.windows();
        const win_wt: EnvInfo = EnvInfo.windowsTerminal();
        const linux_tty: EnvInfo = EnvInfo.linuxConsole();

        expect_eq(figureForEnv(FigureKind.Tick, posix_env), "✔");
        expect_eq(figureForEnv(FigureKind.Tick, win_legacy), "√");
        expect_eq(figureForEnv(FigureKind.Tick, win_wt), "✔");
        expect_eq(figureForEnv(FigureKind.Pointer, linux_tty), ">");
    });

    test("4. @cap/figures — replaceSymbols downgrades Unicode symbols when unicode_ok=false", () => {
        let out: [128]u8 = [0; 128];
        const n: usize = replaceSymbols("✔ Done ✖ Failed ❯ Next…", false, out[0..128]);
        expect_eq(out[0..n], "√ Done × Failed > Next...");
    });

    test("5. @cap/figures — replaceSymbols preserves input verbatim when unicode_ok=true", () => {
        let out: [128]u8 = [0; 128];
        const src: string = "✔ Build ❯ Running… ℹ OK";
        const n: usize = replaceSymbols(src, true, out[0..128]);
        expect_eq(out[0..n], src);
    });

    test("6. @cap/figures — common arrows, heart, line & output buffer clamping", () => {
        expect_eq(figureFor(FigureKind.ArrowUp, false), "↑");
        expect_eq(figureFor(FigureKind.ArrowDown, true), "↓");
        expect_eq(figureFor(FigureKind.ArrowLeft, false), "←");
        expect_eq(figureFor(FigureKind.ArrowRight, true), "→");
        expect_eq(figureFor(FigureKind.Heart, false), "♥");
        expect_eq(figureFor(FigureKind.Line, false), "─");

        let small: [4]u8 = [0; 4];
        const n: usize = replaceSymbols("❯……", false, small[0..4]);
        expect_eq(n, 4 as usize);
        expect_eq(small[0..4], ">...");
    });
}
