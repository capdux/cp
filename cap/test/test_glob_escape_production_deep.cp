// Source: cp/cap/test/test_glob_escape_production_deep.cp (Production deep tests for cap:glob_escape)

import { test, expect_eq } from "cap:test";
import {
    isGlobMagic,
    hasGlobMagic,
    escapeGlob,
    unescapeGlob,
    staticBaseDir,
    isNegatedPattern,
    stripNegation,
} from "cap:text";

test("glob_escape: isGlobMagic and hasGlobMagic with escape awareness") {
    expect_eq(isGlobMagic('*'), true);
    expect_eq(isGlobMagic('?'), true);
    expect_eq(isGlobMagic('['), true);
    expect_eq(isGlobMagic(']'), true);
    expect_eq(isGlobMagic('{'), true);
    expect_eq(isGlobMagic('}'), true);
    expect_eq(isGlobMagic('('), true);
    expect_eq(isGlobMagic(')'), true);
    expect_eq(isGlobMagic('!'), true);
    expect_eq(isGlobMagic('\\'), true);
    expect_eq(isGlobMagic('/'), false);
    expect_eq(isGlobMagic('a'), false);

    expect_eq(hasGlobMagic("src/**/*.cp"), true);
    expect_eq(hasGlobMagic("src/[id]/page.cp"), true);
    expect_eq(hasGlobMagic("file?.txt"), true);
    expect_eq(hasGlobMagic("src/plain/file.cp"), false);
    expect_eq(hasGlobMagic("src/\\[id\\]/file\\*.cp"), false);
    expect_eq(hasGlobMagic(""), false);
}

test("glob_escape: escapeGlob and unescapeGlob exact round-trip") {
    let esc_buf: [128]u8 = [0; 128];
    let raw_buf: [128]u8 = [0; 128];

    const literal = "pages/[...slug]/a*b?c{1,2}(x)!\\end";
    const escaped = escapeGlob(literal, esc_buf[0..]).?;
    expect_eq(hasGlobMagic(escaped), false);
    expect_eq(
        std.mem.eql(u8, escaped, "pages/\\[...slug\\]/a\\*b\\?c\\{1,2\\}\\(x\\)\\!\\\\end"),
        true
    );

    const restored = unescapeGlob(escaped, raw_buf[0..]).?;
    expect_eq(std.mem.eql(u8, restored, literal), true);
}

test("glob_escape: staticBaseDir extracts pruning prefix across relative, absolute and escaped paths") {
    expect_eq(std.mem.eql(u8, staticBaseDir("src/http/*.cp"), "src/http"), true);
    expect_eq(std.mem.eql(u8, staticBaseDir("*.cp"), ""), true);
    expect_eq(std.mem.eql(u8, staticBaseDir("a/b/c.cp"), "a/b/c.cp"), true);
    expect_eq(std.mem.eql(u8, staticBaseDir("/var/log/**/*.log"), "/var/log"), true);
    expect_eq(std.mem.eql(u8, staticBaseDir("/*.cp"), "/"), true);
    expect_eq(std.mem.eql(u8, staticBaseDir("src/\\[id\\]/sub/*.cp"), "src/\\[id\\]/sub"), true);
    expect_eq(std.mem.eql(u8, staticBaseDir(""), ""), true);
}

test("glob_escape: isNegatedPattern and stripNegation distinguish ! from \\!") {
    expect_eq(isNegatedPattern("!dist/**"), true);
    expect_eq(std.mem.eql(u8, stripNegation("!dist/**"), "dist/**"), true);

    expect_eq(isNegatedPattern("\\!important.txt"), false);
    expect_eq(std.mem.eql(u8, stripNegation("\\!important.txt"), "\\!important.txt"), true);

    expect_eq(isNegatedPattern("normal/*.cp"), false);
    expect_eq(isNegatedPattern(""), false);
    expect_eq(stripNegation("").len, 0);
}

test("glob_escape: small output buffer overflow guards") {
    let tiny: [4]u8 = [0; 4];
    expect_eq(escapeGlob("a*b*c", tiny[0..]) == null, true);
    expect_eq(unescapeGlob("a\\*b\\*c\\*d", tiny[0..]) == null, true);

    let exact: [2]u8 = [0; 2];
    const ok = escapeGlob("*", exact[0..]).?;
    expect_eq(std.mem.eql(u8, ok, "\\*"), true);
}
