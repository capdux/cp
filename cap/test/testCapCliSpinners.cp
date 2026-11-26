// cap/test/testCapCliSpinners.cp
// Comprehensive unit tests for @cap/cli-spinners

import { test, expect_eq } from "cap:test";
import {
    SpinnerKind,
    SymbolKind,
    intervalMs,
    frameCount,
    frameAt,
    frameAtTime,
    findSpinnerByName,
    logSymbol,
    renderSpinnerLine,
    renderStatusLine,
} from "../../packages/@cap/cli-spinners/src/index.cp";

fn main() {
    test("1. @cap/cli-spinners — Dots, Dots2, Line, AsciiDots frame cycling & wrap-around", () => {
        expect_eq(intervalMs(SpinnerKind.Dots), 80 as u32);
        expect_eq(frameCount(SpinnerKind.Dots), 10 as usize);
        expect_eq(frameAt(SpinnerKind.Dots, 0 as usize), "⠋");
        expect_eq(frameAt(SpinnerKind.Dots, 9 as usize), "⠏");
        expect_eq(frameAt(SpinnerKind.Dots, 10 as usize), "⠋");

        expect_eq(frameCount(SpinnerKind.Dots2), 8 as usize);
        expect_eq(frameAt(SpinnerKind.Dots2, 0 as usize), "⣾");
        expect_eq(frameAt(SpinnerKind.Dots2, 7 as usize), "⣷");

        expect_eq(frameCount(SpinnerKind.Line), 4 as usize);
        expect_eq(frameAt(SpinnerKind.Line, 0 as usize), "-");
        expect_eq(frameAt(SpinnerKind.Line, 1 as usize), "\\");
        expect_eq(frameAt(SpinnerKind.Line, 2 as usize), "|");
        expect_eq(frameAt(SpinnerKind.Line, 3 as usize), "/");
        expect_eq(frameAt(SpinnerKind.Line, 4 as usize), "-");

        expect_eq(frameCount(SpinnerKind.AsciiDots), 3 as usize);
        expect_eq(frameAt(SpinnerKind.AsciiDots, 0 as usize), ".  ");
        expect_eq(frameAt(SpinnerKind.AsciiDots, 1 as usize), ".. ");
        expect_eq(frameAt(SpinnerKind.AsciiDots, 2 as usize), "...");
    });

    test("2. @cap/cli-spinners — Pipe, Arc, Circle, SquareCorners, Bounce, BouncingBar, Arrow, Clock", () => {
        expect_eq(frameAt(SpinnerKind.Pipe, 0 as usize), "┤");
        expect_eq(frameAt(SpinnerKind.Pipe, 7 as usize), "┐");
        expect_eq(frameAt(SpinnerKind.Arc, 0 as usize), "◜");
        expect_eq(frameAt(SpinnerKind.Arc, 5 as usize), "◟");
        expect_eq(frameAt(SpinnerKind.Circle, 0 as usize), "◐");
        expect_eq(frameAt(SpinnerKind.SquareCorners, 0 as usize), "◰");
        expect_eq(frameAt(SpinnerKind.Bounce, 2 as usize), "⠄");
        expect_eq(frameAt(SpinnerKind.BouncingBar, 3 as usize), "[=== ]");
        expect_eq(frameAt(SpinnerKind.Arrow, 4 as usize), "→");
        expect_eq(frameCount(SpinnerKind.Clock), 12 as usize);
        expect_eq(frameAt(SpinnerKind.Clock, 0 as usize), "🕛");
        expect_eq(frameAt(SpinnerKind.Clock, 11 as usize), "🕚");
    });

    test("3. @cap/cli-spinners — frameAtTime deterministic millisecond selection", () => {
        expect_eq(frameAtTime(SpinnerKind.Dots, 0 as u64), "⠋");
        expect_eq(frameAtTime(SpinnerKind.Dots, 79 as u64), "⠋");
        expect_eq(frameAtTime(SpinnerKind.Dots, 80 as u64), "⠙");
        expect_eq(frameAtTime(SpinnerKind.Dots, 800 as u64), "⠋");

        expect_eq(frameAtTime(SpinnerKind.Line, 260 as u64), "|");
        expect_eq(frameAtTime(SpinnerKind.AsciiDots, 450 as u64), "...");
    });

    test("4. @cap/cli-spinners — findSpinnerByName with camelCase, kebab-case & unknown names", () => {
        const kDots = findSpinnerByName("dots");
        expect_eq(kDots != null, true);
        expect_eq((kDots ?? SpinnerKind.Line) == SpinnerKind.Dots, true);

        const kBar = findSpinnerByName("bouncing-bar");
        expect_eq(kBar != null, true);
        expect_eq((kBar ?? SpinnerKind.Line) == SpinnerKind.BouncingBar, true);

        const kSq = findSpinnerByName("squareCorners");
        expect_eq(kSq != null, true);
        expect_eq((kSq ?? SpinnerKind.Line) == SpinnerKind.SquareCorners, true);

        const kAscii = findSpinnerByName("ascii_dots");
        expect_eq(kAscii != null, true);
        expect_eq((kAscii ?? SpinnerKind.Line) == SpinnerKind.AsciiDots, true);

        expect_eq(findSpinnerByName("nonexistent") == null, true);
    });

    test("5. @cap/cli-spinners — logSymbol Unicode & ASCII fallback modes", () => {
        expect_eq(logSymbol(SymbolKind.Success, false), "✔");
        expect_eq(logSymbol(SymbolKind.Error, false), "✖");
        expect_eq(logSymbol(SymbolKind.Warning, false), "⚠");
        expect_eq(logSymbol(SymbolKind.Info, false), "ℹ");

        expect_eq(logSymbol(SymbolKind.Success, true), "√");
        expect_eq(logSymbol(SymbolKind.Error, true), "×");
        expect_eq(logSymbol(SymbolKind.Warning, true), "!");
        expect_eq(logSymbol(SymbolKind.Info, true), "i");
    });

    test("6. @cap/cli-spinners — renderSpinnerLine & renderStatusLine full line rendering", () => {
        let buf1: [128]u8 = undefined;
        expect_eq(renderSpinnerLine(SpinnerKind.Dots, 2 as usize, "Building project...", buf1[0..128]), "⠹ Building project...");

        let buf2: [128]u8 = undefined;
        expect_eq(renderSpinnerLine(SpinnerKind.Line, 3 as usize, "", buf2[0..128]), "/");

        let buf3: [128]u8 = undefined;
        expect_eq(renderStatusLine(SymbolKind.Success, false, "All tests passed", buf3[0..128]), "✔ All tests passed");

        let buf4: [128]u8 = undefined;
        expect_eq(renderStatusLine(SymbolKind.Warning, true, "Deprecated flag", buf4[0..128]), "! Deprecated flag");

        let buf5: [128]u8 = undefined;
        expect_eq(renderStatusLine(SymbolKind.Info, true, "", buf5[0..128]), "i");
    });
}
