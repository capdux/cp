// cap/test/testCapIsUnicodeSupported.cp
// Comprehensive unit tests for @cap/is-unicode-supported (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    EnvInfo,
    isUnicodeSupported,
    isUnicodeSupportedTerm,
    isUnicodeSupportedTermProgram,
    isUnicodeSupportedPlatform,
    selectFallback,
} from "../../packages/@cap/is-unicode-supported/src/index.cp";

fn main() {
    test("1. @cap/is-unicode-supported — POSIX platforms & Linux kernel console", () => {
        expect_eq(isUnicodeSupported(EnvInfo.posix()), true);
        expect_eq(isUnicodeSupported(EnvInfo.posixWithTerm("")), true);
        expect_eq(isUnicodeSupported(EnvInfo.posixWithTerm("screen-256color")), true);
        expect_eq(isUnicodeSupported(EnvInfo.posixWithTerm("dumb")), true);
        expect_eq(isUnicodeSupported(EnvInfo.linuxConsole()), false);
        expect_eq(isUnicodeSupported(EnvInfo.posixWithTerm("linux")), false);
        // Case-sensitive exact match for "linux"
        expect_eq(isUnicodeSupported(EnvInfo.posixWithTerm("Linux")), true);
    });

    test("2. @cap/is-unicode-supported — Windows bare console vs CI / WT_SESSION / Terminus", () => {
        expect_eq(isUnicodeSupported(EnvInfo.windows()), false);
        expect_eq(isUnicodeSupported(EnvInfo.windowsTerminal()), true);

        let ciEnv = EnvInfo.windows();
        ciEnv.ci = true;
        expect_eq(isUnicodeSupported(ciEnv), true);

        let terminusEnv = EnvInfo.windows();
        terminusEnv.terminus_sublime = true;
        expect_eq(isUnicodeSupported(terminusEnv), true);

        let cmderEnv = EnvInfo.windows();
        cmderEnv.conemu_task = "{cmd::Cmder}";
        expect_eq(isUnicodeSupported(cmderEnv), true);

        let otherConemu = EnvInfo.windows();
        otherConemu.conemu_task = "{PowerShell}";
        expect_eq(isUnicodeSupported(otherConemu), false);
    });

    test("3. @cap/is-unicode-supported — Windows TERM_PROGRAM & JetBrains-JediTerm", () => {
        expect_eq(isUnicodeSupported(EnvInfo.vscodeWindows()), true);

        let tpTerm = EnvInfo.windows();
        tpTerm.term_program = "Terminus-Sublime";
        expect_eq(isUnicodeSupported(tpTerm), true);

        let tpUnknown = EnvInfo.windows();
        tpUnknown.term_program = "Apple_Terminal";
        expect_eq(isUnicodeSupported(tpUnknown), false);

        let jbEnv = EnvInfo.windows();
        jbEnv.terminal_emulator = "JetBrains-JediTerm";
        expect_eq(isUnicodeSupported(jbEnv), true);

        let otherEmu = EnvInfo.windows();
        otherEmu.terminal_emulator = "OtherTerm";
        expect_eq(isUnicodeSupported(otherEmu), false);
    });

    test("4. @cap/is-unicode-supported — Windows TERM allowlist (xterm-256color, alacritty, rxvt-unicode*)", () => {
        let e1 = EnvInfo.windows();
        e1.term = "xterm-256color";
        expect_eq(isUnicodeSupported(e1), true);

        let e2 = EnvInfo.windows();
        e2.term = "alacritty";
        expect_eq(isUnicodeSupported(e2), true);

        let e3 = EnvInfo.windows();
        e3.term = "rxvt-unicode";
        expect_eq(isUnicodeSupported(e3), true);

        let e4 = EnvInfo.windows();
        e4.term = "rxvt-unicode-256color";
        expect_eq(isUnicodeSupported(e4), true);

        let e5 = EnvInfo.windows();
        e5.term = "xterm";
        expect_eq(isUnicodeSupported(e5), false);

        let e6 = EnvInfo.windows();
        e6.term = "linux";
        expect_eq(isUnicodeSupported(e6), false);
    });

    test("5. @cap/is-unicode-supported — helper functions Term / TermProgram / Platform", () => {
        expect_eq(isUnicodeSupportedTerm("xterm-256color"), true);
        expect_eq(isUnicodeSupportedTerm("alacritty"), true);
        expect_eq(isUnicodeSupportedTerm("rxvt-unicode"), true);
        expect_eq(isUnicodeSupportedTerm("rxvt-unicode-256color"), true);
        expect_eq(isUnicodeSupportedTerm("vt100"), false);

        expect_eq(isUnicodeSupportedTermProgram("vscode"), true);
        expect_eq(isUnicodeSupportedTermProgram("Terminus-Sublime"), true);
        expect_eq(isUnicodeSupportedTermProgram("iTerm.app"), false);

        expect_eq(isUnicodeSupportedPlatform(false, "xterm"), true);
        expect_eq(isUnicodeSupportedPlatform(false, "linux"), false);
        expect_eq(isUnicodeSupportedPlatform(true, "alacritty"), true);
        expect_eq(isUnicodeSupportedPlatform(true, "cmd"), false);
    });

    test("6. @cap/is-unicode-supported — selectFallback chooses Unicode vs ASCII symbol", () => {
        expect_eq(selectFallback(EnvInfo.posix(), "✔", "v"), "✔");
        expect_eq(selectFallback(EnvInfo.linuxConsole(), "✔", "v"), "v");
        expect_eq(selectFallback(EnvInfo.windows(), "→", "->"), "->");
        expect_eq(selectFallback(EnvInfo.windowsTerminal(), "→", "->"), "→");
        expect_eq(selectFallback(EnvInfo.vscodeWindows(), "✖", "x"), "✖");
    });
}
