// Source: cp/cap/test/test_rego_production_deep.cp (Production deep tests for cap:rego)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    REGO_MAX_DEPTH,
    RegoTokenKind,
    RegoToken,
    RegoIter,
    RegoImportIter,
    RegoRuleIter,
    extractRegoPackage,
    validateRegoDelimiters,
} from "cap:auth";

test("1. RegoIter tokenizes strings, raw strings, :=, numbers, bools, null, and skips # comments", () => {
    const src =
        "# OPA Rego policy header\n" ++
        "x := \"hello \\\"world\\\"\"\n" ++
        "re := `^/api/v[0-9]+/\\w+$\nline2`\n" ++
        "ok := true\n" ++
        "empty := null\n" ++
        "ratio := 3.14e-2 == 0.0314\n";

    let it = RegoIter.init(src);

    const t0 = it.next() ?? unreachable;
    expect_eq(t0.kind == RegoTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t0.lexeme, "x"), true);

    const t1 = it.next() ?? unreachable;
    expect_eq(t1.kind == RegoTokenKind.assign, true);
    expect_eq(std.mem.eql(u8, t1.lexeme, ":="), true);

    const t2 = it.next() ?? unreachable;
    expect_eq(t2.kind == RegoTokenKind.string, true);
    expect_eq(std.mem.eql(u8, t2.lexeme, "\"hello \\\"world\\\"\""), true);

    let raw_count: usize = 0;
    let bool_count: usize = 0;
    let null_count: usize = 0;
    let num_count: usize = 0;
    let assign_count: usize = 1; // already saw first ':='

    while (tok := it.next()) {
        expect_eq(tok.kind != RegoTokenKind.invalid, true);
        if (tok.kind == RegoTokenKind.raw_string) raw_count += 1;
        if (tok.kind == RegoTokenKind.bool_lit) bool_count += 1;
        if (tok.kind == RegoTokenKind.null_lit) null_count += 1;
        if (tok.kind == RegoTokenKind.number) num_count += 1;
        if (tok.kind == RegoTokenKind.assign) assign_count += 1;
    }

    expect_eq(raw_count, 1);
    expect_eq(bool_count, 1);
    expect_eq(null_count, 1);
    expect_eq(num_count, 2);
    expect_eq(assign_count, 5);

    // Malformed literals yield invalid
    let bad1 = RegoIter.init("\"unterminated");
    expect_eq((bad1.next() ?? unreachable).kind == RegoTokenKind.invalid, true);

    let bad2 = RegoIter.init("`unterminated raw");
    expect_eq((bad2.next() ?? unreachable).kind == RegoTokenKind.invalid, true);

    let bad3 = RegoIter.init("1.5e+");
    expect_eq((bad3.next() ?? unreachable).kind == RegoTokenKind.invalid, true);
});

test("2. extractRegoPackage extracts dotted package paths and handles comments/invalid cases", () => {
    const p1 = extractRegoPackage("# leading comment\npackage authz.api.v1\nimport rego.v1\n") ?? unreachable;
    expect_eq(std.mem.eql(u8, p1, "authz.api.v1"), true);

    const p2 = extractRegoPackage("package main") ?? unreachable;
    expect_eq(std.mem.eql(u8, p2, "main"), true);

    // Package inside comment is ignored; real package is found
    const p3 = extractRegoPackage("# package fake.pkg\npackage real.service.auth\n") ?? unreachable;
    expect_eq(std.mem.eql(u8, p3, "real.service.auth"), true);

    // Missing or malformed package declarations return null
    expect_eq(extractRegoPackage("import rego.v1\nallow := true") == null, true);
    expect_eq(extractRegoPackage("package authz.") == null, true);
    expect_eq(extractRegoPackage("package 123") == null, true);
});

test("3. RegoImportIter extracts all top-level imports and ignores aliases, strings, and comments", () => {
    const policy =
        "package authz.gateway\n" ++
        "# import data.commented.out\n" ++
        "import rego.v1\n" ++
        "import data.foo.bar\n" ++
        "import data.authz.roles as user_roles\n" ++
        "import future.keywords.if\n" ++
        "allow if {\n" ++
        "    msg := \"import data.inside.string\"\n" ++
        "    input.user == \"alice\"\n" ++
        "}\n";

    let it = RegoImportIter.init(policy);

    const i0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, i0, "rego.v1"), true);

    const i1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, i1, "data.foo.bar"), true);

    const i2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, i2, "data.authz.roles"), true);

    const i3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, i3, "future.keywords.if"), true);

    expect_eq(it.next() == null, true);
});

test("4. RegoRuleIter extracts default, v1 if, assignment, contains, indexed, and function rules", () => {
    const policy =
        "package authz.api.v1\n" ++
        "import rego.v1\n" ++
        "import data.users as u\n" ++
        "\n" ++
        "default allow := false\n" ++
        "allow if {\n" ++
        "    is_admin(input.user)\n" ++
        "}\n" ++
        "decision := \"permit\" if {\n" ++
        "    allow\n" ++
        "} else := \"deny\" if {\n" ++
        "    not allow\n" ++
        "}\n" ++
        "roles contains r if {\n" ++
        "    some r in input.roles\n" ++
        "}\n" ++
        "permissions[k] := v if {\n" ++
        "    k := input.resource\n" ++
        "    v := true\n" ++
        "}\n" ++
        "is_admin(user) if {\n" ++
        "    user == \"root\"\n" ++
        "}\n";

    let rit = RegoRuleIter.init(policy);

    const r0 = rit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r0, "allow"), true);

    const r1 = rit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r1, "allow"), true);

    const r2 = rit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r2, "decision"), true);

    const r3 = rit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r3, "roles"), true);

    const r4 = rit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r4, "permissions"), true);

    const r5 = rit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r5, "is_admin"), true);

    expect_eq(rit.next() == null, true);
});

test("5. validateRegoDelimiters ignores strings/comments/raw strings and enforces 64-level depth cap", () => {
    expect_eq(validateRegoDelimiters("allow if { x := \"{[(\"; y := `])}`; # {\n a := [1, (2 + 3)] }"), true);
    expect_eq(validateRegoDelimiters("allow if { x := \"escaped \\\" }\" }"), true);

    // Mismatches and unclosed structures
    expect_eq(validateRegoDelimiters("allow if { x := [1, 2) }"), false);
    expect_eq(validateRegoDelimiters("allow if { x := [1, 2]"), false);
    expect_eq(validateRegoDelimiters("allow if { x := 1 }} "), false);
    expect_eq(validateRegoDelimiters("allow if { x := \"unclosed }"), false);
    expect_eq(validateRegoDelimiters("allow if { x := `unclosed }"), false);

    // Exact 64-level nesting passes; 65-level nesting is rejected
    let ok_buf: [128]u8 = std.mem.zeroes([128]u8);
    let i: usize = 0;
    while (i < REGO_MAX_DEPTH) {
        ok_buf[i] = '[';
        ok_buf[REGO_MAX_DEPTH * 2 - 1 - i] = ']';
        i += 1;
    }
    expect_eq(validateRegoDelimiters(ok_buf[0..128]), true);

    let deep_buf: [130]u8 = std.mem.zeroes([130]u8);
    i = 0;
    while (i < REGO_MAX_DEPTH + 1) {
        deep_buf[i] = '[';
        deep_buf[(REGO_MAX_DEPTH + 1) * 2 - 1 - i] = ']';
        i += 1;
    }
    expect_eq(validateRegoDelimiters(deep_buf[0..130]), false);
});
