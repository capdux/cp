import { test, expect_eq } from "cap:test";
import { strEq } from "cap:ascii";
import {
    isWindowsReservedName,
    isSafeLegacyComponent,
    hasWindowsVerbatimPrefix,
    hasWindowsVerbatimUncPrefix,
    canStripVerbatimPrefix,
    canSimplifyUncPath,
    simplifiedSlice,
    simplifiedLen,
    simplified,
    simplifiedUnc,
    simplifiedWithUnc,
} from "../../packages/dunce/src/index.cp";

fn main() {
    test("1. dunce — Normal verbatim drive path strips '\\\\?\\' safely", () => {
        const p: string = "\\\\?\\C:\\Users\\alice\\file.txt";
        expect_eq(hasWindowsVerbatimPrefix(p), true);
        expect_eq(canStripVerbatimPrefix(p), true);
        expect_eq(strEq(simplifiedSlice(p), "C:\\Users\\alice\\file.txt"), true);

        let buf: [128]u8 = undefined;
        const n = simplified(p, &buf);
        expect_eq(n, simplifiedLen(p));
        expect_eq(strEq(buf[0..n], "C:\\Users\\alice\\file.txt"), true);

        // Drive root \\?\C:\ simplifies to C:\
        const root: string = "\\\\?\\C:\\";
        const nr = simplified(root, &buf);
        expect_eq(strEq(buf[0..nr], "C:\\"), true);
    });

    test("2. dunce — Reserved device names (CON.txt, NUL, COM1, lpt9) refuse stripping", () => {
        expect_eq(isWindowsReservedName("CON"), true);
        expect_eq(isWindowsReservedName("con.txt"), true);
        expect_eq(isWindowsReservedName("nul.log"), true);
        expect_eq(isWindowsReservedName("COM1"), true);
        expect_eq(isWindowsReservedName("lpt9.bin"), true);
        expect_eq(isWindowsReservedName("COM0"), false);
        expect_eq(isWindowsReservedName("CONSOLE"), false);

        const p1: string = "\\\\?\\C:\\tmp\\CON.txt";
        const p2: string = "\\\\?\\C:\\tmp\\NUL";
        expect_eq(canStripVerbatimPrefix(p1), false);
        expect_eq(canStripVerbatimPrefix(p2), false);

        let buf: [128]u8 = undefined;
        const n1 = simplified(p1, &buf);
        expect_eq(strEq(buf[0..n1], p1), true);
    });

    test("3. dunce — Trailing dot or space on components refuses stripping", () => {
        const p_dot: string = "\\\\?\\C:\\foo.\\bar";
        const p_spc: string = "\\\\?\\C:\\foo \\bar";
        expect_eq(canStripVerbatimPrefix(p_dot), false);
        expect_eq(canStripVerbatimPrefix(p_spc), false);

        let buf: [128]u8 = undefined;
        const n = simplified(p_dot, &buf);
        expect_eq(strEq(buf[0..n], p_dot), true);
    });

    test("4. dunce — Overlong path (> 259 bytes after stripping) refuses stripping", () => {
        let long_path: [270]u8 = undefined;
        long_path[0] = 92; // '\'
        long_path[1] = 92; // '\'
        long_path[2] = 63; // '?'
        long_path[3] = 92; // '\'
        long_path[4] = 67; // 'C'
        long_path[5] = 58; // ':'
        long_path[6] = 92; // '\'
        let i: usize = 7;
        while (i < 264) {
            long_path[i] = 97; // 'a'
            i += 1;
        }
        // Length 263 -> stripped 259 (allowed!)
        expect_eq(canStripVerbatimPrefix(long_path[0..263]), true);
        // Length 264 -> stripped 260 (rejected!)
        expect_eq(canStripVerbatimPrefix(long_path[0..264]), false);
    });

    test("5. dunce — UNC network path, relative '..' & illegal chars refuse drive stripping", () => {
        const unc: string = "\\\\?\\UNC\\server\\share\\file.txt";
        const rel: string = "\\\\?\\C:\\a\\..\\b";
        const fwd: string = "\\\\?\\C:\\a/b";
        const bad: string = "\\\\?\\C:\\a\\b*c";
        expect_eq(canStripVerbatimPrefix(unc), false);
        expect_eq(canStripVerbatimPrefix(rel), false);
        expect_eq(canStripVerbatimPrefix(fwd), false);
        expect_eq(canStripVerbatimPrefix(bad), false);

        let buf: [128]u8 = undefined;
        const n = simplified(unc, &buf);
        expect_eq(strEq(buf[0..n], unc), true);
    });

    test("6. dunce — Non-verbatim paths pass through unchanged & empty/buffer safety", () => {
        const normal: string = "/usr/local/bin/cap";
        expect_eq(hasWindowsVerbatimPrefix(normal), false);
        expect_eq(canStripVerbatimPrefix(normal), false);

        let buf: [64]u8 = undefined;
        const n = simplified(normal, &buf);
        expect_eq(strEq(buf[0..n], normal), true);

        expect_eq(simplified("", &buf), 0 as usize);
        const empty_out: []u8 = buf[0..0];
        expect_eq(simplified("\\\\?\\C:\\a", empty_out), 0 as usize);
    });

    test("7. dunce — UNC verbatim path simplification (\\\\?\\UNC\\server\\share -> \\\\server\\share)", () => {
        const unc: string = "\\\\?\\UNC\\filesrv\\public\\report.pdf";
        expect_eq(hasWindowsVerbatimUncPrefix(unc), true);
        expect_eq(canSimplifyUncPath(unc), true);

        let buf: [128]u8 = undefined;
        const n1 = simplifiedUnc(unc, &buf);
        expect_eq(strEq(buf[0..n1], "\\\\filesrv\\public\\report.pdf"), true);

        // simplifiedWithUnc handles both drive and UNC paths
        const n2 = simplifiedWithUnc(unc, &buf);
        expect_eq(strEq(buf[0..n2], "\\\\filesrv\\public\\report.pdf"), true);

        const drv: string = "\\\\?\\D:\\work\\main.cp";
        const n3 = simplifiedWithUnc(drv, &buf);
        expect_eq(strEq(buf[0..n3], "D:\\work\\main.cp"), true);
    });

    test("8. dunce — Unsafe UNC paths & isSafeLegacyComponent validation", () => {
        // Missing share or reserved component refuses UNC simplification
        expect_eq(canSimplifyUncPath("\\\\?\\UNC\\onlyserver"), false);
        expect_eq(canSimplifyUncPath("\\\\?\\UNC\\srv\\share\\CON.txt"), false);
        expect_eq(canSimplifyUncPath("\\\\?\\UNC\\srv\\share\\bad.\\file"), false);
        expect_eq(canSimplifyUncPath("\\\\?\\UNC\\srv\\share\\..\\x"), false);

        let buf: [128]u8 = undefined;
        const unsafe_unc: string = "\\\\?\\UNC\\srv\\share\\NUL";
        const n = simplifiedUnc(unsafe_unc, &buf);
        expect_eq(strEq(buf[0..n], unsafe_unc), true);

        expect_eq(isSafeLegacyComponent("normal.txt"), true);
        expect_eq(isSafeLegacyComponent(".."), false);
        expect_eq(isSafeLegacyComponent("AUX.log"), false);
        expect_eq(isSafeLegacyComponent("trail "), false);
    });
}
