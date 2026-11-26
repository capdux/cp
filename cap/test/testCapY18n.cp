import { test, expect_eq } from "cap:test";
import {
    Y18nCatalog,
    formatPrintf,
} from "../../packages/@cap/y18n/src/index.cp";

fn y18TestEq(actual: []const u8, expected: string): bool {
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

fn main() {
    test("1. @cap/y18n — Basic t() lookup and fallback to original key", () => {
        let cat: Y18nCatalog = Y18nCatalog.new("zh_CN");
        expect_eq(y18TestEq(cat.getLocale(), "zh_CN"), true);
        expect_eq(cat.setEntry("Hello world", "你好世界"), true);
        expect_eq(cat.hasEntry("Hello world"), true);
        expect_eq(cat.hasEntry("Missing"), false);

        let dummy_args: [1]string = [""];
        let out: [128]u8 = undefined;
        const n1: usize = cat.t("Hello world", dummy_args[0..0], &out);
        expect_eq(y18TestEq(out[0..n1], "你好世界"), true);

        const n2: usize = cat.t("Untranslated key", dummy_args[0..0], &out);
        expect_eq(y18TestEq(out[0..n2], "Untranslated key"), true);
    });

    test("2. @cap/y18n — Multi-%s positional substitution and %% literal escape", () => {
        let cat: Y18nCatalog = Y18nCatalog.new("en_US");
        * = cat.setEntry("progress", "Task %s is %s%% done for %s");

        let args: [3]string = ["build", "95", "cap"];
        let out: [128]u8 = undefined;
        const n: usize = cat.t("progress", &args, &out);
        expect_eq(y18TestEq(out[0..n], "Task build is 95% done for cap"), true);
    });

    test("3. @cap/y18n — tn() singular, plural, zero, and negative %d counts", () => {
        let cat: Y18nCatalog = Y18nCatalog.new("en_US");
        * = cat.setPluralEntry("one file in %s", "%d file in %s", "%d files in %s");

        let args: [1]string = ["src/"];
        let out: [128]u8 = undefined;

        const n1: usize = cat.tn("one file in %s", "many files in %s", 1, &args, &out);
        expect_eq(y18TestEq(out[0..n1], "1 file in src/"), true);

        const n2: usize = cat.tn("one file in %s", "many files in %s", 5, &args, &out);
        expect_eq(y18TestEq(out[0..n2], "5 files in src/"), true);

        const n0: usize = cat.tn("one file in %s", "many files in %s", 0, &args, &out);
        expect_eq(y18TestEq(out[0..n0], "0 files in src/"), true);

        const n_neg: usize = cat.tn("one file in %s", "many files in %s", -3, &args, &out);
        expect_eq(y18TestEq(out[0..n_neg], "-3 files in src/"), true);

        // Fallback when key is not in catalog
        const n_fb: usize = cat.tn("%d item", "%d items", 2, &args, &out);
        expect_eq(y18TestEq(out[0..n_fb], "2 items"), true);
    });

    test("4. @cap/y18n — Locale switching and overwriting existing entries", () => {
        let cat: Y18nCatalog = Y18nCatalog.new("en_US");
        * = cat.setEntry("bye", "Goodbye, %s!");

        let args: [1]string = ["Alice"];
        let out: [128]u8 = undefined;
        const n1: usize = cat.t("bye", &args, &out);
        expect_eq(y18TestEq(out[0..n1], "Goodbye, Alice!"), true);

        // Switch locale and overwrite translation in-place without growing count
        cat.setLocale("fr_FR");
        expect_eq(y18TestEq(cat.getLocale(), "fr_FR"), true);
        * = cat.setEntry("bye", "Au revoir, %s !");
        expect_eq(cat.count, 1);

        const n2: usize = cat.t("bye", &args, &out);
        expect_eq(y18TestEq(out[0..n2], "Au revoir, Alice !"), true);
    });

    test("5. @cap/y18n — Missing and extra arguments boundary safety", () => {
        let one_arg: [1]string = ["only"];
        let out: [128]u8 = undefined;

        // Missing second %s preserves literal "%s"
        const n1: usize = formatPrintf("a=%s b=%s", 0, false, &one_arg, &out);
        expect_eq(y18TestEq(out[0..n1], "a=only b=%s"), true);

        // Extra arguments beyond placeholders are safely ignored
        let many_args: [3]string = ["first", "second", "third"];
        const n2: usize = formatPrintf("val=%s", 0, false, &many_args, &out);
        expect_eq(y18TestEq(out[0..n2], "val=first"), true);
    });

    test("6. @cap/y18n — Small output buffer truncation without overflow", () => {
        let args: [1]string = ["world"];
        let tiny: [7]u8 = undefined;
        const n: usize = formatPrintf("Hello, %s!", 0, false, &args, &tiny);
        expect_eq(n, 7);
        expect_eq(y18TestEq(tiny[0..n], "Hello, "), true);

        expect_eq(formatPrintf("abc", 0, false, &args, tiny[0..0]), 0);
    });
}
