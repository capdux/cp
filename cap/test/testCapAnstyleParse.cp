import { test, expect_eq } from "cap:test";
import {
    TOKEN_TEXT,
    TOKEN_CSI,
    TOKEN_OSC,
    TOKEN_ESC,
    AnsiToken,
    tokenizeAnsi,
    parseCsiParams,
    stripAnsiToBuf,
    hasAnsi,
    visibleByteLen,
} from "../../packages/@cap/anstyle-parse/src/index.cp";

fn strEq(buf: []const u8, expected: string): bool {
    if (buf.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn appendStr(dst: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < dst.len) {
        dst[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn main() {
    test("1. @cap/anstyle-parse — Plain text with no ANSI sequences", () => {
        const plain: string = "Hello, terminal world!";
        expect_eq(hasAnsi(plain), false);
        expect_eq(visibleByteLen(plain), plain.len);

        let toks: [8]AnsiToken = undefined;
        const n = tokenizeAnsi(plain, &toks);
        expect_eq(n, 1);
        expect_eq(toks[0].kind, TOKEN_TEXT);
        expect_eq(toks[0].start, 0);
        expect_eq(toks[0].len, plain.len);

        let out: [64]u8 = undefined;
        const written = stripAnsiToBuf(plain, &out);
        expect_eq(strEq(out[0..written], plain), true);
    });

    test("2. @cap/anstyle-parse — SGR color sequence ESC[1;31mhelloESC[0m tokenization", () => {
        let raw: [32]u8 = undefined;
        let p: usize = 0;
        raw[p] = 27; p += 1;
        p = appendStr(&raw, p, "[1;31mhello");
        raw[p] = 27; p += 1;
        p = appendStr(&raw, p, "[0m");
        const s: string = raw[0..p];

        expect_eq(hasAnsi(s), true);
        let toks: [8]AnsiToken = undefined;
        const n = tokenizeAnsi(s, &toks);
        expect_eq(n, 3);
        expect_eq(toks[0].kind, TOKEN_CSI);
        expect_eq(toks[0].final_byte, 109); // 'm'
        expect_eq(toks[1].kind, TOKEN_TEXT);
        expect_eq(toks[1].len, 5);
        expect_eq(toks[2].kind, TOKEN_CSI);
        expect_eq(toks[2].final_byte, 109); // 'm'
    });

    test("3. @cap/anstyle-parse — parseCsiParams 256-color, truecolor & empty ESC[m", () => {
        let s256: [16]u8 = undefined;
        s256[0] = 27;
        const n256 = appendStr(&s256, 1, "[1;38;5;196m");
        const p256 = parseCsiParams(s256[0..n256]);
        expect_eq(p256.valid, true);
        expect_eq(p256.count, 4);
        expect_eq(p256.params[0], 1);
        expect_eq(p256.params[1], 38);
        expect_eq(p256.params[2], 5);
        expect_eq(p256.params[3], 196);

        let srgb: [24]u8 = undefined;
        srgb[0] = 27;
        const nrgb = appendStr(&srgb, 1, "[38;2;255;128;0m");
        const prgb = parseCsiParams(srgb[0..nrgb]);
        expect_eq(prgb.valid, true);
        expect_eq(prgb.count, 5);
        expect_eq(prgb.params[2], 255);
        expect_eq(prgb.params[3], 128);
        expect_eq(prgb.params[4], 0);

        let sreset: [4]u8 = [27, 91, 109, 0]; // ESC[m
        const preset = parseCsiParams(sreset[0..3]);
        expect_eq(preset.valid, true);
        expect_eq(preset.count, 1);
        expect_eq(preset.params[0], 0);

        expect_eq(parseCsiParams("plain").valid, false);
    });

    test("4. @cap/anstyle-parse — OSC 8 hyperlink ESC]8;;https://...BEL & ST stripping", () => {
        let raw: [64]u8 = undefined;
        let p: usize = 0;
        raw[p] = 27; p += 1;
        p = appendStr(&raw, p, "]8;;https://cap.dev");
        raw[p] = 7; p += 1; // BEL
        p = appendStr(&raw, p, "CapLang");
        raw[p] = 27; p += 1;
        p = appendStr(&raw, p, "]8;;");
        raw[p] = 27; p += 1;
        raw[p] = 92; p += 1; // ST = ESC '\'
        const s: string = raw[0..p];

        let toks: [8]AnsiToken = undefined;
        const n = tokenizeAnsi(s, &toks);
        expect_eq(n, 3);
        expect_eq(toks[0].kind, TOKEN_OSC);
        expect_eq(toks[0].final_byte, 7);
        expect_eq(toks[1].kind, TOKEN_TEXT);
        expect_eq(toks[1].len, 7);
        expect_eq(toks[2].kind, TOKEN_OSC);
        expect_eq(toks[2].final_byte, 92);

        let out: [32]u8 = undefined;
        const written = stripAnsiToBuf(s, &out);
        expect_eq(strEq(out[0..written], "CapLang"), true);
        expect_eq(visibleByteLen(s), 7);
    });

    test("5. @cap/anstyle-parse — stripAnsiToBuf & visibleByteLen on mixed CSI/ESC/Text", () => {
        let raw: [64]u8 = undefined;
        let p: usize = 0;
        raw[p] = 27; p += 1;
        p = appendStr(&raw, p, "[32mOK");
        raw[p] = 27; p += 1;
        p = appendStr(&raw, p, "[0m: ");
        raw[p] = 27; p += 1;
        p = appendStr(&raw, p, "7"); // ESC 7 (DECSC save cursor)
        p = appendStr(&raw, p, "done");
        const s: string = raw[0..p];

        expect_eq(visibleByteLen(s), 8); // "OK: done"
        let out: [32]u8 = undefined;
        const written = stripAnsiToBuf(s, &out);
        expect_eq(written, 8);
        expect_eq(strEq(out[0..written], "OK: done"), true);
    });

    test("6. @cap/anstyle-parse — Truncated/incomplete escape sequences boundary safety", () => {
        // Bare trailing ESC
        let bare: [4]u8 = [65, 66, 27, 0]; // "AB" + ESC
        let toks: [8]AnsiToken = undefined;
        const n1 = tokenizeAnsi(bare[0..3], &toks);
        expect_eq(n1, 2);
        expect_eq(toks[0].kind, TOKEN_TEXT);
        expect_eq(toks[1].kind, TOKEN_ESC);
        expect_eq(toks[1].final_byte, 0);
        expect_eq(visibleByteLen(bare[0..3]), 2);

        // Incomplete CSI: "hi" + ESC[1;3
        let trunc_csi: [8]u8 = [104, 105, 27, 91, 49, 59, 51, 0];
        const n2 = tokenizeAnsi(trunc_csi[0..7], &toks);
        expect_eq(n2, 2);
        expect_eq(toks[1].kind, TOKEN_CSI);
        expect_eq(toks[1].final_byte, 0);
        expect_eq(visibleByteLen(trunc_csi[0..7]), 2);
        expect_eq(parseCsiParams(trunc_csi[2..7]).valid, false);

        // Empty string
        expect_eq(tokenizeAnsi("", &toks), 0);
        expect_eq(hasAnsi(""), false);
        expect_eq(visibleByteLen(""), 0);
    });
}
