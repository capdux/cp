// Source: cp/cap/test/test_wgsl_production_deep.cp (Production deep tests for cap:wgsl)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    WgslStage,
    WgslEntryPoint,
    WgslBinding,
    WgslTokenKind,
    WgslToken,
    WgslIter,
    WgslEntryPointIter,
    WgslBindingIter,
    validateWgslDelimiters,
} from "cap:compiler";

const SAMPLE_SHADER: []const u8 =
    "// WebGPU compute + render shader\n" ++
    "/* outer /* nested block comment */ still comment */\n" ++
    "@group(0) @binding(0) var<uniform> params: Params;\n" ++
    "@binding(2) @group(1) var<storage, read_write> buf_out: array<f32>;\n" ++
    "@group(0) @binding(1) var tex_sampler: sampler;\n" ++
    "\n" ++
    "@vertex\n" ++
    "fn vs_main(@builtin(vertex_index) idx: u32) -> @builtin(position) vec4<f32> {\n" ++
    "    return vec4<f32>(0.0f, 1.0f, 0.0f, 1.0f);\n" ++
    "}\n" ++
    "\n" ++
    "@fragment\n" ++
    "fn fs_main() -> @location(0) vec4<f32> {\n" ++
    "    return vec4<f32>(1.0f, 0.5f, 0.0f, 1.0f);\n" ++
    "}\n" ++
    "\n" ++
    "@compute @workgroup_size(64, 1, 1)\n" ++
    "fn cs_kernel(@builtin(global_invocation_id) gid: vec3<u32>) {\n" ++
    "    let mask = 0x10u;\n" ++
    "}\n";

test("1. WgslIter lexes nested comments, @attributes, suffixed/hex numbers, and multi-char punct", () => {
    const src = "/* a /* b */ c */ @compute fn f() -> u32 { let x = 1.5f; let y = 0x10i; return 0u; }";
    let it = WgslIter.init(src);

    const t0 = it.next() ?? unreachable;
    expect_eq(t0.kind == WgslTokenKind.at_attr, true);
    expect_eq(std.mem.eql(u8, t0.lexeme, "@compute"), true);

    let saw_arrow = false;
    let saw_float = false;
    let saw_hex = false;
    let saw_u = false;

    while (tok := it.next()) {
        if (tok.kind == WgslTokenKind.punct && std.mem.eql(u8, tok.lexeme, "->")) saw_arrow = true;
        if (tok.kind == WgslTokenKind.number && std.mem.eql(u8, tok.lexeme, "1.5f")) saw_float = true;
        if (tok.kind == WgslTokenKind.number && std.mem.eql(u8, tok.lexeme, "0x10i")) saw_hex = true;
        if (tok.kind == WgslTokenKind.number && std.mem.eql(u8, tok.lexeme, "0u")) saw_u = true;
    }

    expect_eq(saw_arrow, true);
    expect_eq(saw_float, true);
    expect_eq(saw_hex, true);
    expect_eq(saw_u, true);
});

test("2. WgslEntryPointIter discovers vertex, fragment, and compute entry points with @workgroup_size", () => {
    let it = WgslEntryPointIter.init(SAMPLE_SHADER);

    const ep0 = it.next() ?? unreachable;
    expect_eq(ep0.stage == WgslStage.vertex, true);
    expect_eq(std.mem.eql(u8, ep0.name, "vs_main"), true);

    const ep1 = it.next() ?? unreachable;
    expect_eq(ep1.stage == WgslStage.fragment, true);
    expect_eq(std.mem.eql(u8, ep1.name, "fs_main"), true);

    const ep2 = it.next() ?? unreachable;
    expect_eq(ep2.stage == WgslStage.compute, true);
    expect_eq(std.mem.eql(u8, ep2.name, "cs_kernel"), true);

    expect_eq(it.next() == null, true);
});

test("3. WgslBindingIter extracts @group/@binding in any order with or without address-space template", () => {
    let it = WgslBindingIter.init(SAMPLE_SHADER);

    const b0 = it.next() ?? unreachable;
    expect_eq(b0.group, 0);
    expect_eq(b0.binding, 0);
    expect_eq(std.mem.eql(u8, b0.var_name, "params"), true);

    const b1 = it.next() ?? unreachable;
    expect_eq(b1.group, 1);
    expect_eq(b1.binding, 2);
    expect_eq(std.mem.eql(u8, b1.var_name, "buf_out"), true);

    const b2 = it.next() ?? unreachable;
    expect_eq(b2.group, 0);
    expect_eq(b2.binding, 1);
    expect_eq(std.mem.eql(u8, b2.var_name, "tex_sampler"), true);

    expect_eq(it.next() == null, true);
});

test("4. validateWgslDelimiters validates nested block comments and balanced delimiters", () => {
    expect_eq(validateWgslDelimiters(SAMPLE_SHADER), true);
    expect_eq(validateWgslDelimiters("/* outer /* inner { [ ( */ */ fn f() { let a = arr[0]; }"), true);

    // Mismatched / unclosed delimiters
    expect_eq(validateWgslDelimiters("fn f() { let a = (1 + 2]; }"), false);
    expect_eq(validateWgslDelimiters("fn f() { let a = [0; }"), false);
    expect_eq(validateWgslDelimiters("fn f() { "), false);
    expect_eq(validateWgslDelimiters("fn f() ) {}"), false);

    // Unclosed nested block comment
    expect_eq(validateWgslDelimiters("/* outer /* inner */ fn f() {}"), false);
});

test("5. Invalid tokens and empty/non-entry-point helper functions are safely handled", () => {
    const helper_only = "fn helper(x: f32) -> f32 { return x * 2.0f; }";
    let eps = WgslEntryPointIter.init(helper_only);
    expect_eq(eps.next() == null, true);

    let binds = WgslBindingIter.init(helper_only);
    expect_eq(binds.next() == null, true);

    // Bare '@' is invalid
    let lex = WgslIter.init("@ 123");
    const bad = lex.next() ?? unreachable;
    expect_eq(bad.kind == WgslTokenKind.invalid, true);
    expect_eq(validateWgslDelimiters("@ 123"), false);
});
