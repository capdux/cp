// Source: cp/cap/test/test_wat_production_deep.cp (Production deep tests for cap:wat)

import { test, expect_eq } from "cap:test";
import {
    WatTokenKind,
    WatToken,
    WatIter,
    WatExport,
    WatExportIter,
    validateWatStructure,
} from "cap:wat";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("1. WatIter: comments, nested (; ;), $id, strings, hex/float/underscore numbers & keywords") {
    const src =
        "\xEF\xBB\xBF;; line comment\n" ++
        "(; outer (; inner ;) block ;)\n" ++
        "(module $demo\n" ++
        "  (data (i32.const 0x10_00) \"hi\\00\\u{21}\")\n" ++
        "  (global f64 (f64.const -3.14_15e+2))\n" ++
        "  (global f32 (f32.const nan:0x20_0000))\n" ++
        "  (global f32 (f32.const +inf))\n" ++
        ")\n";

    let it = WatIter.init(src);
    expect_eq(it.next().?.kind == .lparen, true);

    const t_mod = it.next().?;
    expect_eq(t_mod.kind == .keyword, true);
    expect_eq(strEq(t_mod.lexeme, "module"), true);

    const t_id = it.next().?;
    expect_eq(t_id.kind == .id, true);
    expect_eq(strEq(t_id.lexeme, "$demo"), true);

    // Verify all remaining tokens are valid and count numbers & strings
    let num_count: usize = 0;
    let str_count: usize = 0;
    while (tok := it.next()) {
        expect_eq(tok.kind != .invalid, true);
        if (tok.kind == .number) num_count += 1;
        if (tok.kind == WatTokenKind.string) str_count += 1;
    }
    expect_eq(num_count, @as(usize, 4));
    expect_eq(str_count, @as(usize, 1));
}

test("2. WatExportIter: standalone (export ...) declarations for func, memory, table & global") {
    const wat =
        "(module\n" ++
        "  (func $add (param i32 i32) (result i32) (i32.add (local.get 0) (local.get 1)))\n" ++
        "  (memory $mem 1)\n" ++
        "  (table 2 funcref)\n" ++
        "  (global $g i32 (i32.const 7))\n" ++
        "  (export \"add\" (func $add))\n" ++
        "  (export \"memory\" (memory $mem))\n" ++
        "  (export \"tbl\" (table 0))\n" ++
        "  (export \"answer\" (global $g))\n" ++
        ")\n";

    expect_eq(validateWatStructure(wat), true);

    let it = WatExportIter.init(wat);

    const e0 = it.next().?;
    expect_eq(strEq(e0.name, "add"), true);
    expect_eq(strEq(e0.kind, "func"), true);
    expect_eq(strEq(e0.target, "$add"), true);

    const e1 = it.next().?;
    expect_eq(strEq(e1.name, "memory"), true);
    expect_eq(strEq(e1.kind, "memory"), true);
    expect_eq(strEq(e1.target, "$mem"), true);

    const e2 = it.next().?;
    expect_eq(strEq(e2.name, "tbl"), true);
    expect_eq(strEq(e2.kind, "table"), true);
    expect_eq(strEq(e2.target, "0"), true);

    const e3 = it.next().?;
    expect_eq(strEq(e3.name, "answer"), true);
    expect_eq(strEq(e3.kind, "global"), true);
    expect_eq(strEq(e3.target, "$g"), true);

    expect_eq(it.next() == null, true);
}

test("3. WatExportIter: inline exports (named, anonymous indexed & multiple aliases)") {
    const wat =
        "(module\n" ++
        "  (func $mul (export \"mul\") (export \"multiply\") (param i32 i32) (result i32)\n" ++
        "    (i32.mul (local.get 0) (local.get 1)))\n" ++
        "  (func (export \"anon_fn\") (result i32) (i32.const 1))\n" ++
        "  (memory (export \"mem\") 1)\n" ++
        ")\n";

    expect_eq(validateWatStructure(wat), true);

    let it = WatExportIter.init(wat);

    const e0 = it.next().?;
    expect_eq(strEq(e0.name, "mul"), true);
    expect_eq(strEq(e0.kind, "func"), true);
    expect_eq(strEq(e0.target, "$mul"), true);

    const e1 = it.next().?;
    expect_eq(strEq(e1.name, "multiply"), true);
    expect_eq(strEq(e1.kind, "func"), true);
    expect_eq(strEq(e1.target, "$mul"), true);

    const e2 = it.next().?;
    expect_eq(strEq(e2.name, "anon_fn"), true);
    expect_eq(strEq(e2.kind, "func"), true);
    expect_eq(strEq(e2.target, "1"), true);

    const e3 = it.next().?;
    expect_eq(strEq(e3.name, "mem"), true);
    expect_eq(strEq(e3.kind, "memory"), true);
    expect_eq(strEq(e3.target, "0"), true);

    expect_eq(it.next() == null, true);
}

test("4. Empty module and module with comments only") {
    expect_eq(validateWatStructure("(module)"), true);
    expect_eq(validateWatStructure(";; comment\n(module (; nested (; comment ;) ;))"), true);

    let it = WatExportIter.init("(module (func $internal (nop)))");
    expect_eq(it.next() == null, true);
}

test("5. Malformed WAT rejection: non-module root, unbalanced parens, unclosed comments/strings & deep nesting") {
    expect_eq(validateWatStructure(""), false);
    expect_eq(validateWatStructure("(func)"), false);
    expect_eq(validateWatStructure("(module"), false);
    expect_eq(validateWatStructure("(module))"), false);
    expect_eq(validateWatStructure("(module) (module)"), false);
    expect_eq(validateWatStructure("(module (; unclosed (; nested ;) )"), false);
    expect_eq(validateWatStructure("(module (data \"unterminated))"), false);
    expect_eq(validateWatStructure("(module (data \"bad\\q\"))"), false);
    expect_eq(validateWatStructure("(module (func $ ))"), false);

    // Deep nesting guard (> 64)
    let deep: [160]u8 = [0; 160];
    const prefix = "(module ";
    let pos: usize = 0;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        deep[pos] = prefix[i];
        pos += 1;
    }
    i = 0;
    while (i < 66) : (i += 1) {
        deep[pos] = '(';
        pos += 1;
    }
    i = 0;
    while (i < 67) : (i += 1) {
        deep[pos] = ')';
        pos += 1;
    }
    expect_eq(validateWatStructure(deep[0..pos]), false);
}
