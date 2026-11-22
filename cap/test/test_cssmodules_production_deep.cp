// Source: cp/cap/test/test_cssmodules_production_deep.cp (Production deep tests for cap:cssmodules)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CssModuleConfig,
    Pattern,
    ReferenceKind,
    CssModuleRef,
    isValidCssIdent,
    extractFileStemSanitized,
    computeModuleHash,
    scopeClassName,
    parseComposesValue,
} from "cap:cssmodules";

test("1. Default config, file stem dot-sanitization & CSS identifier validation", () => {
    const cfg = CssModuleConfig.default();
    expect_eq(cfg.dashed_idents, false);
    expect_eq(cfg.animation, true);
    expect_eq(cfg.grid, true);
    expect_eq(cfg.custom_idents, true);

    let buf: [64]u8 = [0; 64];
    let s: []u8 = buf[0..];
    if (stem := extractFileStemSanitized("src/components/Button.module.css", *s)) {
        expect_eq(std.mem.eql(u8, stem, "Button-module"), true);
    } else { expect_eq(true, false); }

    if (stem2 := extractFileStemSanitized("C:\\ui\\App.v2.module.scss", *s)) {
        expect_eq(std.mem.eql(u8, stem2, "App-v2-module"), true);
    } else { expect_eq(true, false); }

    expect_eq(isValidCssIdent("btn_primary-1"), true);
    expect_eq(isValidCssIdent("_9abc"), true);
    expect_eq(isValidCssIdent("--custom-var"), true);
    expect_eq(isValidCssIdent("1invalid"), false);
    expect_eq(isValidCssIdent("-2invalid"), false);
    expect_eq(isValidCssIdent(""), false);
});

test("2. Deterministic computeModuleHash & at_start leading-digit CSS safety across 64 paths", () => {
    let h1_b: [8]u8 = [0; 8];
    let h2_b: [8]u8 = [0; 8];
    let h1_s: []u8 = h1_b[0..];
    let h2_s: []u8 = h2_b[0..];

    const a = computeModuleHash("src/Button.module.css", false, *h1_s);
    const b = computeModuleHash("src/Button.module.css", false, *h2_s);
    expect_eq(a != null && b != null, true);
    if (ha := a) {
        if (hb := b) {
            expect_eq(ha.len, 6);
            expect_eq(std.mem.eql(u8, ha, hb), true);
        }
    }

    // Verify at_start == true ALWAYS produces a valid CSS identifier (never starts with digit or -<digit>)
    let path_buf: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 64) {
        path_buf[0] = 'f';
        path_buf[1] = ('0' as u8) + ((i / 10) as u8);
        path_buf[2] = ('0' as u8) + ((i % 10) as u8);
        path_buf[3] = '.';
        path_buf[4] = 'c';
        path_buf[5] = 's';
        path_buf[6] = 's';
        if (hs := computeModuleHash(path_buf[0..7], true, *h1_s)) {
            expect_eq(hs.len == 6 || hs.len == 7, true);
            expect_eq(isValidCssIdent(hs), true);
        } else {
            expect_eq(true, false);
        }
        i += 1;
    }
});

test("3. Pattern parsing & scopeClassName formatting ([local]_[hash], [name]__[local]--[hash], [hash]_[local])", () => {
    let out_b: [128]u8 = [0; 128];
    let out_s: []u8 = out_b[0..];

    // Default pattern: [local]_[hash]
    const def_pat = Pattern.default();
    expect_eq(def_pat.startsWithHash(), false);
    if (scoped := scopeClassName(def_pat, "src/Card.module.css", "header", *out_s)) {
        expect_eq(std.mem.startsWith(u8, scoped, "header_"), true);
        expect_eq(scoped.len, 7 + 6);
        expect_eq(isValidCssIdent(scoped), true);
    } else { expect_eq(true, false); }

    // Custom pattern: [name]__[local]--[hash]
    if (pat := Pattern.parse("[name]__[local]--[hash]")) {
        expect_eq(pat.startsWithHash(), false);
        if (scoped := scopeClassName(pat, "ui/Button.module.css", "primary", *out_s)) {
            expect_eq(std.mem.startsWith(u8, scoped, "Button-module__primary--"), true);
            expect_eq(isValidCssIdent(scoped), true);
        } else { expect_eq(true, false); }
    } else { expect_eq(true, false); }

    // Hash-first pattern: [hash]_[local] must always be a valid CSS identifier
    if (hpat := Pattern.parse("[hash]_[local]")) {
        expect_eq(hpat.startsWithHash(), true);
        if (scoped := scopeClassName(hpat, "a.css", "box", *out_s)) {
            expect_eq(std.mem.endsWith(u8, scoped, "_box"), true);
            expect_eq(isValidCssIdent(scoped), true);
        } else { expect_eq(true, false); }
    } else { expect_eq(true, false); }
});

test("4. parseComposesValue parses local, global & cross-file dependency references", () => {
    let refs_b: [4]CssModuleRef = [{ kind: ReferenceKind.local, name: "", specifier: "" }; 4];
    let refs: []CssModuleRef = refs_b[0..];

    // Local composition
    if (n := parseComposesValue("  btn   primary  ", *refs)) {
        expect_eq(n, 2);
        expect_eq(refs[0].kind, ReferenceKind.local);
        expect_eq(std.mem.eql(u8, refs[0].name, "btn"), true);
        expect_eq(refs[1].kind, ReferenceKind.local);
        expect_eq(std.mem.eql(u8, refs[1].name, "primary"), true);
    } else { expect_eq(true, false); }

    // Global composition
    if (n := parseComposesValue("reset-btn focus-ring from global", *refs)) {
        expect_eq(n, 2);
        expect_eq(refs[0].kind, ReferenceKind.global);
        expect_eq(std.mem.eql(u8, refs[0].name, "reset-btn"), true);
        expect_eq(refs[1].kind, ReferenceKind.global);
        expect_eq(std.mem.eql(u8, refs[1].name, "focus-ring"), true);
    } else { expect_eq(true, false); }

    // Dependency composition (single & double quotes)
    if (n := parseComposesValue("card shadow from './base.module.css'", *refs)) {
        expect_eq(n, 2);
        const expected0: CssModuleRef = { kind: ReferenceKind.dependency, name: "card", specifier: "./base.module.css" };
        expect_eq(refs[0].eql(expected0), true);
        expect_eq(std.mem.eql(u8, refs[1].name, "shadow"), true);
        expect_eq(std.mem.eql(u8, refs[1].specifier, "./base.module.css"), true);
    } else { expect_eq(true, false); }

    if (n := parseComposesValue("elevated from \"../theme/tokens.css\"", *refs)) {
        expect_eq(n, 1);
        expect_eq(refs[0].kind, ReferenceKind.dependency);
        expect_eq(std.mem.eql(u8, refs[0].specifier, "../theme/tokens.css"), true);
    } else { expect_eq(true, false); }
});

test("5. Boundary defense: malformed patterns, invalid composes & short buffers", () => {
    // Pattern errors
    expect_eq(Pattern.parse("") == null, true);
    expect_eq(Pattern.parse("[unclosed") == null, true);
    expect_eq(Pattern.parse("stray]") == null, true);
    expect_eq(Pattern.parse("[unknown]_[local]") == null, true);

    // Composes errors
    let refs_b: [2]CssModuleRef = [{ kind: ReferenceKind.local, name: "", specifier: "" }; 2];
    let refs: []CssModuleRef = refs_b[0..];
    expect_eq(parseComposesValue("", *refs) == null, true);
    expect_eq(parseComposesValue("from global", *refs) == null, true);
    expect_eq(parseComposesValue("btn from", *refs) == null, true);
    expect_eq(parseComposesValue("btn from './unclosed.css", *refs) == null, true);
    expect_eq(parseComposesValue("btn from ''", *refs) == null, true);
    expect_eq(parseComposesValue("btn from unknown_ident", *refs) == null, true);
    expect_eq(parseComposesValue("btn from global extra", *refs) == null, true);
    expect_eq(parseComposesValue("1badident", *refs) == null, true);
    expect_eq(parseComposesValue("a b c", *refs) == null, true); // exceeds capacity 2

    // Short output buffers
    let tiny_b: [3]u8 = [0; 3];
    let tiny: []u8 = tiny_b[0..];
    expect_eq(computeModuleHash("a.css", false, *tiny) == null, true);
    expect_eq(extractFileStemSanitized("Button.module.css", *tiny) == null, true);
    expect_eq(scopeClassName(Pattern.default(), "a.css", "btn", *tiny) == null, true);
    expect_eq(scopeClassName(Pattern.default(), "a.css", "", *tiny) == null, true);
});
