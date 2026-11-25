// cap/test/testSlugifyDeep.cp
// Deep unit tests for cap:slugify (Rust `slug`/`deunicode` parity: Unicode Latin transliteration,
// SlugOptions, symbol expansion, word-boundary truncation, and custom-separator validation).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SlugOptions,
    deunicodeChar,
    deunicode,
    slugifyWithOptions,
    slugifyWithSep,
    slugify,
    slugifyBounded,
    isValidSlugWithSep,
    isValidSlug,
} from "cap:slugify";

fn main() {
    test("1. cap:slugify — Basic ASCII, Apostrophes & Curly Quotes", () => {
        let buf: [128]u8 = [0; 128];
        const s1 = slugify("Don't Panic: CP Lang v1.0!", buf[0..]).?;
        expect_eq(std.mem.eql(u8, s1, "dont-panic-cp-lang-v1-0"), true);
        expect_eq(isValidSlug(s1), true);

        const s2 = slugify("Don’t Stop — Moving!", buf[0..]).?;
        expect_eq(std.mem.eql(u8, s2, "dont-stop-moving"), true);

        const empty = slugify("   ---!!!   ", buf[0..]).?;
        expect_eq(empty.len, 0 as usize);
        expect_eq(isValidSlug(empty), false);
    });

    test("2. cap:slugify — Unicode Latin-1 & Latin Extended-A Transliteration", () => {
        let buf: [128]u8 = [0; 128];
        const s1 = slugify("Café Crème — Über Straße", buf[0..]).?;
        expect_eq(std.mem.eql(u8, s1, "cafe-creme-uber-strasse"), true);

        const s2 = slugify("Łódź Ægir Œuvre Þór François", buf[0..]).?;
        expect_eq(std.mem.eql(u8, s2, "lodz-aegir-oeuvre-thor-francois"), true);

        const s3 = slugify("Crème brûlée in Zürich", buf[0..]).?;
        expect_eq(std.mem.eql(u8, s3, "creme-brulee-in-zurich"), true);

        expect_eq(std.mem.eql(u8, deunicodeChar(0x00DF), "ss"), true);
        expect_eq(std.mem.eql(u8, deunicodeChar(0x0141), "L"), true);
        expect_eq(std.mem.eql(u8, deunicodeChar(0x4E2D), ""), true);
    });

    test("3. cap:slugify — deunicode Plain Text Preservation", () => {
        let buf: [128]u8 = [0; 128];
        const plain = deunicode("Crème brûlée in Zürich (Ægir)!", buf[0..]).?;
        expect_eq(std.mem.eql(u8, plain, "Creme brulee in Zurich (AEgir)!"), true);
    });

    test("4. cap:slugify — SlugOptions (Case Preservation, Symbol Expansion, Custom Sep)", () => {
        let buf: [128]u8 = [0; 128];
        const sym = slugifyWithOptions(
            "C++ & Rust @ 100%",
            SlugOptions{ separator: '_', expand_symbols: true },
            buf[0..],
        ).?;
        expect_eq(std.mem.eql(u8, sym, "c_plus_plus_and_rust_at_100_percent"), true);
        expect_eq(isValidSlugWithSep(sym, '_'), true);
        expect_eq(isValidSlug(sym), false);

        const cased = slugifyWithOptions(
            "Hello World From CP",
            SlugOptions{ lowercase: false, max_len: 15 },
            buf[0..],
        ).?;
        expect_eq(std.mem.eql(u8, cased, "Hello-World"), true);

        const snake = slugifyWithSep("Hello World 2026", '_', buf[0..]).?;
        expect_eq(std.mem.eql(u8, snake, "hello_world_2026"), true);
    });

    test("5. cap:slugify — Word-Boundary Bounded Truncation & Buffer Safety", () => {
        let buf: [64]u8 = [0; 64];
        const bounded = slugifyBounded("hello world from cp lang", 12, buf[0..]).?;
        expect_eq(std.mem.eql(u8, bounded, "hello-world"), true);
        expect_eq(isValidSlug(bounded), true);

        let tiny: [3]u8 = [0; 3];
        expect_eq(slugify("hello-world", tiny[0..]) == null, true);

        expect_eq(isValidSlug("valid-slug-123"), true);
        expect_eq(isValidSlug("-leading"), false);
        expect_eq(isValidSlug("trailing-"), false);
        expect_eq(isValidSlug("double--dash"), false);
        expect_eq(isValidSlug("UpperCase"), false);
    });
}
