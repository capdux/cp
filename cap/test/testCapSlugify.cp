// cap/test/testCapSlugify.cp
// Comprehensive unit tests for @cap/slugify

import { test, expect_eq } from "cap:test";
import {
    slugify,
    slugifyWithSep,
    slugifyTruncate,
    isValidSlug,
} from "../../packages/@cap/slugify/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/slugify — Standard English title with multi-space & punctuation collapse", () => {
        let buf: [64]u8 = undefined;
        const n = slugify("  Hello,   World!!  CP 2026 -- Fast & Safe  ", buf[0..64]);
        expect_eq(sliceEq(buf[0..n], "hello-world-cp-2026-fast-safe"), true);
        expect_eq(isValidSlug(buf[0..n]), true);
    });

    test("2. @cap/slugify — UTF-8 Latin diacritic folding (Café Résumé naïve über Straße)", () => {
        let buf: [64]u8 = undefined;
        const n = slugify("Café Résumé naïve über Straße", buf[0..64]);
        expect_eq(sliceEq(buf[0..n], "cafe-resume-naive-uber-strasse"), true);

        const n2 = slugify("Piñata Façade Øresund", buf[0..64]);
        expect_eq(sliceEq(buf[0..n2], "pinata-facade-oresund"), true);
    });

    test("3. @cap/slugify — slugifyWithSep custom '_' separator & case preservation", () => {
        let buf: [64]u8 = undefined;
        const n1 = slugifyWithSep("Hello World 42", '_' as u8, true, buf[0..64]);
        expect_eq(sliceEq(buf[0..n1], "hello_world_42"), true);

        const n2 = slugifyWithSep("My API v2!", '-' as u8, false, buf[0..64]);
        expect_eq(sliceEq(buf[0..n2], "My-API-v2"), true);
    });

    test("4. @cap/slugify — slugifyTruncate respects max_len without trailing '-'", () => {
        let buf: [64]u8 = undefined;
        // "hello-world-cp" -> max_len 6 would land on "hello-", must trim to "hello"
        const n1 = slugifyTruncate("Hello World CP", 6, buf[0..64]);
        expect_eq(sliceEq(buf[0..n1], "hello"), true);

        const n2 = slugifyTruncate("Hello World CP", 11, buf[0..64]);
        expect_eq(sliceEq(buf[0..n2], "hello-world"), true);
    });

    test("5. @cap/slugify — isValidSlug accepts canonical slugs & rejects invalid forms", () => {
        expect_eq(isValidSlug("hello-world-123"), true);
        expect_eq(isValidSlug("a"), true);

        expect_eq(isValidSlug(""), false);
        expect_eq(isValidSlug("-leading"), false);
        expect_eq(isValidSlug("trailing-"), false);
        expect_eq(isValidSlug("double--dash"), false);
        expect_eq(isValidSlug("Upper-Case"), false);
        expect_eq(isValidSlug("has_underscore"), false);
    });

    test("6. @cap/slugify — Pure symbols, empty string & small buffer protection", () => {
        let buf: [32]u8 = undefined;
        expect_eq(slugify("", buf[0..32]), 0 as usize);
        expect_eq(slugify("!!! --- ???", buf[0..32]), 0 as usize);

        let tiny: [5]u8 = undefined;
        const nt = slugify("hello world", tiny[0..5]);
        expect_eq(nt, 5 as usize);
        expect_eq(sliceEq(tiny[0..nt], "hello"), true);
    });
}
