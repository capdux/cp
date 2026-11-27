// cap/test/testCapKleur.cp
// Comprehensive unit tests for @cap/kleur (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    KLEUR_MAX_DEPTH,
    KleurChain
} from "../../packages/kleur/src/index.cp";

fn main() {
    test("1. KleurChain — single foreground, background & modifier formatting", () => {
        let out: [128]u8 = undefined;

        const n1: usize = KleurChain.on().red().format("fail", out[0..128]);
        expect_eq(out[0..n1], "\x1b[31mfail\x1b[39m");

        const n2: usize = KleurChain.on().bgBlue().format("info", out[0..128]);
        expect_eq(out[0..n2], "\x1b[44minfo\x1b[49m");

        const n3: usize = KleurChain.on().italic().format("note", out[0..128]);
        expect_eq(out[0..n3], "\x1b[3mnote\x1b[23m");

        const n4: usize = KleurChain.on().gray().format("dimmed", out[0..128]);
        expect_eq(out[0..n4], "\x1b[90mdimmed\x1b[39m");
    });

    test("2. KleurChain — multi-layer chain opens in order & closes in reverse order", () => {
        let out: [256]u8 = undefined;
        const chain: KleurChain = KleurChain.new(true).bold().red().bgYellow();
        expect_eq(chain.depthCount(), 3 as usize);

        const n: usize = chain.format("ALERT", out[0..256]);
        expect_eq(out[0..n], "\x1b[1m\x1b[31m\x1b[43mALERT\x1b[49m\x1b[39m\x1b[22m");

        // Immutable branching: base chain remains reusable independently
        const base: KleurChain = KleurChain.on().underline();
        const aLen: usize = base.green().format("OK", out[0..256]);
        expect_eq(out[0..aLen], "\x1b[4m\x1b[32mOK\x1b[39m\x1b[24m");
        const bLen: usize = base.cyan().format("GO", out[0..256]);
        expect_eq(out[0..bLen], "\x1b[4m\x1b[36mGO\x1b[39m\x1b[24m");
    });

    test("3. KleurChain — disabled mode & empty chain pass text through verbatim", () => {
        let out: [128]u8 = undefined;

        const n1: usize = KleurChain.off().bold().red().bgWhite().format("plain", out[0..128]);
        expect_eq(out[0..n1], "plain");

        const n2: usize = KleurChain.on().format("unstyled", out[0..128]);
        expect_eq(out[0..n2], "unstyled");

        // Empty text produces 0 bytes
        const n3: usize = KleurChain.on().green().format("", out[0..128]);
        expect_eq(n3, 0 as usize);
    });

    test("4. KleurChain — nested close-code recovery (red wrapping blue)", () => {
        let inner: [64]u8 = undefined;
        let combined: [128]u8 = undefined;
        let outer: [256]u8 = undefined;

        const bLen: usize = KleurChain.on().blue().format("world", inner[0..64]);
        let pos: usize = 0;
        const pfx = "hello ";
        let i: usize = 0;
        while (i < pfx.len) { combined[pos] = pfx[i]; pos = pos + 1; i = i + 1; }
        i = 0;
        while (i < bLen) { combined[pos] = inner[i]; pos = pos + 1; i = i + 1; }
        const sfx = "!";
        i = 0;
        while (i < sfx.len) { combined[pos] = sfx[i]; pos = pos + 1; i = i + 1; }

        const n: usize = KleurChain.on().red().format(combined[0..pos], outer[0..256]);
        expect_eq(outer[0..n], "\x1b[31mhello \x1b[34mworld\x1b[31m!\x1b[39m");
    });

    test("5. KleurChain — embedded \\x1b[0m reset recovers all active outer styles", () => {
        let out: [256]u8 = undefined;
        const chain: KleurChain = KleurChain.on().bold().green();
        const n: usize = chain.format("a\x1b[0mb", out[0..256]);
        expect_eq(out[0..n], "\x1b[1m\x1b[32ma\x1b[0m\x1b[1m\x1b[32mb\x1b[39m\x1b[22m");
    });

    test("6. KleurChain — 8-layer max depth clamp & tiny output buffer safety", () => {
        const deep: KleurChain = KleurChain.on()
            .bold()
            .dim()
            .italic()
            .underline()
            .inverse()
            .hidden()
            .strikethrough()
            .red()
            .bgBlack() // 9th layer clamped safely
            .white();  // 10th layer clamped safely
        expect_eq(deep.depthCount(), KLEUR_MAX_DEPTH);

        let tiny: [10]u8 = undefined;
        const n: usize = KleurChain.on().bold().magenta().format("truncated", tiny[0..10]);
        expect_eq(n, 10 as usize);
        expect_eq(tiny[0..10], "\x1b[1m\x1b[35mt");
    });
}
