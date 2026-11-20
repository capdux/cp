// Source: cp/cap/test/test_i18n_production_deep.cp
// Deep Production Test Suite for cap:i18n
// Covers:
//   1. LocaleNegotiator: normalizeLocale, fallbackChain (zh-Hans-CN 3-level), Accept-Language q-value & prefix negotiation
//   2. PluralRules.select: English, French/Portuguese, Chinese/Japanese, Russian, Polish & Arabic
//   3. I18nCatalog: hierarchical fallback (en-GB -> en -> default), single-pass anti-injection format, escaped braces & KeyNotFound
//   4. loadJson nested dictionary import (via cap:json) & tp plural template {count} rendering
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.i18n`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    I18nError,
    PluralCategory,
    I18nParam,
    LocaleNegotiator,
    PluralRules,
    I18nCatalog,
} from "cap:i18n";
import cap from "cap";

fn expectStrEq(alloc: std.mem.Allocator, actual: []u8, expected: string)! {
    defer alloc.free(actual);
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn testLocaleNegotiator()! {
    const alloc = std.heap.page_allocator;

    // 1. normalizeLocale
    expectStrEq(alloc, LocaleNegotiator.normalizeLocale(alloc, "zh_cn")?, "zh-CN")?;
    expectStrEq(alloc, LocaleNegotiator.normalizeLocale(alloc, "en-us")?, "en-US")?;
    expectStrEq(alloc, LocaleNegotiator.normalizeLocale(alloc, "  zh_hans_cn ")?, "zh-Hans-CN")?;
    expectStrEq(alloc, LocaleNegotiator.normalizeLocale(alloc, "FR_ch")?, "fr-CH")?;
    expectStrEq(alloc, LocaleNegotiator.normalizeLocale(alloc, "es_419")?, "es-419")?;

    let invalid_hit = false;
    * = LocaleNegotiator.normalizeLocale(alloc, "") catch |e| {
        if (e == I18nError.InvalidLocale) invalid_hit = true;
    };
    expect_eq(invalid_hit, true);

    let trailing_dash = false;
    * = LocaleNegotiator.normalizeLocale(alloc, "en-") catch |e| {
        if (e == I18nError.InvalidLocale) trailing_dash = true;
    };
    expect_eq(trailing_dash, true);

    // 2. fallbackChain (3-level zh-Hans-CN)
    const chain = LocaleNegotiator.fallbackChain(alloc, "zh_hans_cn")?;
    defer LocaleNegotiator.freeFallbackChain(alloc, chain);
    expect_eq(chain.len, 3 as usize);
    expect_eq(std.mem.eql(u8, chain[0], "zh-Hans-CN"), true);
    expect_eq(std.mem.eql(u8, chain[1], "zh-Hans"), true);
    expect_eq(std.mem.eql(u8, chain[2], "zh"), true);

    // 3. Accept-Language RFC 4647 negotiation
    const supported: [3]string = ["en-US", "fr", "de"];
    expectStrEq(
        alloc,
        LocaleNegotiator.negotiate(
            alloc,
            "fr-CH, fr;q=0.9, en;q=0.8, de;q=0.7, *;q=0.5",
            &supported,
            "en-US",
        )?,
        "fr",
    )?;

    // q-value reordering: de (0.95) beats en (0.8)
    expectStrEq(
        alloc,
        LocaleNegotiator.negotiate(
            alloc,
            "es;q=1.0, en;q=0.8, de;q=0.95",
            &supported,
            "en-US",
        )?,
        "de",
    )?;

    // q=0 explicitly excludes fr; falls back to en -> en-US prefix match
    expectStrEq(
        alloc,
        LocaleNegotiator.negotiate(
            alloc,
            "fr;q=0, en;q=0.7, *;q=0.5",
            &supported,
            "de",
        )?,
        "en-US",
    )?;

    // Wildcard '*' falls back to default_locale
    expectStrEq(
        alloc,
        LocaleNegotiator.negotiate(
            alloc,
            "ja, ko;q=0.9, *;q=0.5",
            &supported,
            "en-US",
        )?,
        "en-US",
    )?;
}

fn testPluralRules()! {
    // English: 1 -> One, 0/2 -> Other
    expect_eq(PluralRules.select("en", 1), PluralCategory.One);
    expect_eq(PluralRules.select("en-US", 0), PluralCategory.Other);
    expect_eq(PluralRules.select("en-GB", 2), PluralCategory.Other);

    // French / Portuguese: 0/1 -> One, 2 -> Other
    expect_eq(PluralRules.select("fr", 0), PluralCategory.One);
    expect_eq(PluralRules.select("fr-CA", 1), PluralCategory.One);
    expect_eq(PluralRules.select("fr", 2), PluralCategory.Other);
    expect_eq(PluralRules.select("pt-BR", 0), PluralCategory.One);

    // Chinese / Japanese: always Other
    expect_eq(PluralRules.select("zh-CN", 0), PluralCategory.Other);
    expect_eq(PluralRules.select("zh-CN", 1), PluralCategory.Other);
    expect_eq(PluralRules.select("ja", 5), PluralCategory.Other);

    // Russian: 1/21 -> One, 2/24 -> Few, 0/5/11/12/25 -> Many
    expect_eq(PluralRules.select("ru", 1), PluralCategory.One);
    expect_eq(PluralRules.select("ru", 21), PluralCategory.One);
    expect_eq(PluralRules.select("ru", 2), PluralCategory.Few);
    expect_eq(PluralRules.select("ru", 24), PluralCategory.Few);
    expect_eq(PluralRules.select("ru", 0), PluralCategory.Many);
    expect_eq(PluralRules.select("ru", 5), PluralCategory.Many);
    expect_eq(PluralRules.select("ru", 11), PluralCategory.Many);
    expect_eq(PluralRules.select("ru", 12), PluralCategory.Many);
    expect_eq(PluralRules.select("ru", 25), PluralCategory.Many);

    // Polish
    expect_eq(PluralRules.select("pl", 1), PluralCategory.One);
    expect_eq(PluralRules.select("pl", 2), PluralCategory.Few);
    expect_eq(PluralRules.select("pl", 21), PluralCategory.Many);

    // Arabic
    expect_eq(PluralRules.select("ar", 0), PluralCategory.Zero);
    expect_eq(PluralRules.select("ar", 1), PluralCategory.One);
    expect_eq(PluralRules.select("ar", 2), PluralCategory.Two);
    expect_eq(PluralRules.select("ar", 5), PluralCategory.Few);
    expect_eq(PluralRules.select("ar", 15), PluralCategory.Many);
    expect_eq(PluralRules.select("ar", 100), PluralCategory.Other);
}

fn testCatalogFallbackAndSafeFormat()! {
    const alloc = std.heap.page_allocator;

    let cat = I18nCatalog.init(alloc, "en")?;
    defer cat.deinit();

    cat.add("en", "greeting", "Hello {name}, {{escaped}} {missing}!")?;
    cat.add("en", "only_default", "Fallback to default")?;
    cat.add("en-GB", "color", "colour")?;
    cat.add("en", "color", "color")?;

    // 1. Hierarchical fallback: en-GB -> en, en-GB overrides color
    expectStrEq(alloc, cat.t(alloc, "en-GB", "color")?, "colour")?;
    expectStrEq(alloc, cat.t(alloc, "en-GB", "only_default")?, "Fallback to default")?;
    expectStrEq(alloc, cat.t(alloc, "fr-FR", "only_default")?, "Fallback to default")?;

    // 2. Single-pass anti-injection interpolation + escaped braces + preserved unknown placeholder
    const params: [2]I18nParam = [
        I18nParam{ key: "name", value: "{secret}" },
        I18nParam{ key: "secret", value: "SHOULD_NOT_EXPAND" },
    ];
    expectStrEq(
        alloc,
        cat.format(alloc, "en-GB", "greeting", &params)?,
        "Hello {secret}, {escaped} {missing}!",
    )?;

    // 3. Missing key returns KeyNotFound
    let key_missing = false;
    * = cat.t(alloc, "en", "nonexistent.key") catch |e| {
        if (e == I18nError.KeyNotFound) key_missing = true;
    };
    expect_eq(key_missing, true);
}

fn testLoadJsonAndPlurals()! {
    const alloc = std.heap.page_allocator;

    let cat = I18nCatalog.init(alloc, "en")?;
    defer cat.deinit();

    const en_json = "{\"auth\": {\"login_title\": \"Sign In\", \"welcome\": \"Welcome, {user}!\"}, \"common\": {\"ok\": \"OK\"}}";
    const zh_json = "{\"auth\": {\"login_title\": \"登录\"}}";

    cat.loadJson("en", en_json)?;
    cat.loadJson("zh-CN", zh_json)?;

    expectStrEq(alloc, cat.t(alloc, "en", "auth.login_title")?, "Sign In")?;
    expectStrEq(alloc, cat.t(alloc, "zh-CN", "auth.login_title")?, "登录")?;
    expectStrEq(alloc, cat.t(alloc, "zh-CN", "common.ok")?, "OK")?;

    const welcome_params: [1]I18nParam = [I18nParam{ key: "user", value: "Alice" }];
    expectStrEq(alloc, cat.format(alloc, "zh-CN", "auth.welcome", &welcome_params)?, "Welcome, Alice!")?;

    // Plural templates with {count}
    cat.addPlural("en", "files", PluralCategory.One, "{count} file in {dir}")?;
    cat.addPlural("en", "files", PluralCategory.Other, "{count} files in {dir}")?;
    cat.addPlural("ru", "files", PluralCategory.One, "{count} файл")?;
    cat.addPlural("ru", "files", PluralCategory.Few, "{count} файла")?;
    cat.addPlural("ru", "files", PluralCategory.Many, "{count} файлов")?;
    cat.addPlural("zh-CN", "files", PluralCategory.Other, "{count} 个文件")?;

    const dir_params: [1]I18nParam = [I18nParam{ key: "dir", value: "/tmp" }];
    expectStrEq(alloc, cat.tp(alloc, "en", "files", 1, &dir_params)?, "1 file in /tmp")?;
    expectStrEq(alloc, cat.tp(alloc, "en", "files", 5, &dir_params)?, "5 files in /tmp")?;

    expectStrEq(alloc, cat.tp(alloc, "ru", "files", 21, [])?, "21 файл")?;
    expectStrEq(alloc, cat.tp(alloc, "ru", "files", 24, [])?, "24 файла")?;
    expectStrEq(alloc, cat.tp(alloc, "ru", "files", 11, [])?, "11 файлов")?;
    expectStrEq(alloc, cat.tp(alloc, "zh-CN", "files", 3, [])?, "3 个文件")?;
}

fn testCapI18nDynamicLazyMount()! {
    const alloc = std.heap.page_allocator;

    const norm = cap.i18n.LocaleNegotiator.normalizeLocale(alloc, "zh_hans_cn")?;
    defer alloc.free(norm);
    expect_eq(std.mem.eql(u8, norm, "zh-Hans-CN"), true);

    expect_eq(cap.i18n.PluralRules.select("fr", 0), cap.i18n.PluralCategory.One);

    let cat = cap.i18n.I18nCatalog.init(alloc, "en")?;
    defer cat.deinit();
    cat.add("en", "hello", "Hi {who}")?;

    const p: [1]cap.i18n.I18nParam = [cap.i18n.I18nParam{ key: "who", value: "CP" }];
    const msg = cat.format(alloc, "en", "hello", &p)?;
    defer alloc.free(msg);
    expect_eq(std.mem.eql(u8, msg, "Hi CP"), true);
}

fn main() {
    test("1. LocaleNegotiator: normalizeLocale, fallbackChain & Accept-Language RFC 4647", () => {
        testLocaleNegotiator()?;
    });

    test("2. PluralRules.select: English, French, Chinese, Russian, Polish & Arabic", () => {
        testPluralRules()?;
    });

    test("3. I18nCatalog: Hierarchical Fallback, Safe Single-Pass Format & KeyNotFound", () => {
        testCatalogFallbackAndSafeFormat()?;
    });

    test("4. I18nCatalog: Nested JSON loadJson & CLDR Plural tp Rendering", () => {
        testLoadJsonAndPlurals()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.i18n`", () => {
        testCapI18nDynamicLazyMount()?;
    });
}
