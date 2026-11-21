// Source: cp/cap/test/test_glsl_production_deep.cp (Production deep tests for cap:glsl)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    GlslVersion,
    GlslQualifier,
    GlslDecl,
    GlslTokenKind,
    GlslToken,
    GlslIter,
    parseGlslVersion,
    GlslDeclIter,
    validateGlslBraces,
} from "cap:glsl";

const VERT_SHADER: []const u8 =
    "// Vertex shader\n" ++
    "#version 450 core\n" ++
    "#extension GL_ARB_separate_shader_objects : enable\n" ++
    "/* multi-line\n comment */\n" ++
    "layout(location = 0) in vec3 a_pos;\n" ++
    "layout(location = 1) in highp vec2 a_uv;\n" ++
    "layout(set = 0, binding = 2) uniform sampler2D u_albedo;\n" ++
    "uniform mat4 u_mvp;\n" ++
    "layout(location = 0) out vec2 v_uv;\n" ++
    "\n" ++
    "void helper(in vec3 p, out vec3 q) {\n" ++
    "    q = p * 2.0f;\n" ++
    "}\n" ++
    "void main() {\n" ++
    "    v_uv = a_uv;\n" ++
    "    gl_Position = u_mvp * vec4(a_pos, 1.0);\n" ++
    "}\n";

test("1. parseGlslVersion parses core, es, bare version, and rejects missing/invalid version", () => {
    const v1 = parseGlslVersion(VERT_SHADER) ?? unreachable;
    expect_eq(v1.number, 450);
    expect_eq(std.mem.eql(u8, v1.profile, "core"), true);

    const v2 = parseGlslVersion("#version 300 es\nprecision mediump float;") ?? unreachable;
    expect_eq(v2.number, 300);
    expect_eq(std.mem.eql(u8, v2.profile, "es"), true);

    const v3 = parseGlslVersion("  # version 330 // comment\nvoid main(){}") ?? unreachable;
    expect_eq(v3.number, 330);
    expect_eq(std.mem.eql(u8, v3.profile, ""), true);

    expect_eq(parseGlslVersion("void main() {}") == null, true);
    expect_eq(parseGlslVersion("#version 0 core") == null, true);
    expect_eq(parseGlslVersion("#version abc") == null, true);
});

test("2. GlslIter lexes preproc lines, hex/float/uint numbers, comments, and operators", () => {
    const src = "#define MAX 16\n/* skip */ float x = 1.5e-2f + 0x10u; // tail";
    let it = GlslIter.init(src);

    const t0 = it.next() ?? unreachable;
    expect_eq(t0.kind == GlslTokenKind.preproc, true);
    expect_eq(std.mem.eql(u8, t0.lexeme, "#define MAX 16"), true);

    let saw_float = false;
    let saw_hex = false;
    while (tok := it.next()) {
        if (tok.kind == GlslTokenKind.number && std.mem.eql(u8, tok.lexeme, "1.5e-2f")) saw_float = true;
        if (tok.kind == GlslTokenKind.number && std.mem.eql(u8, tok.lexeme, "0x10u")) saw_hex = true;
    }
    expect_eq(saw_float, true);
    expect_eq(saw_hex, true);
});

test("3. GlslDeclIter extracts top-level uniform/in/out with layout location/binding and ignores function params", () => {
    let it = GlslDeclIter.init(VERT_SHADER);

    const d0 = it.next() ?? unreachable;
    expect_eq(d0.qualifier == GlslQualifier.input, true);
    expect_eq(std.mem.eql(u8, d0.ty, "vec3"), true);
    expect_eq(std.mem.eql(u8, d0.name, "a_pos"), true);
    expect_eq(d0.location ?? 99, 0);
    expect_eq(d0.binding == null, true);

    const d1 = it.next() ?? unreachable;
    expect_eq(d1.qualifier == GlslQualifier.input, true);
    expect_eq(std.mem.eql(u8, d1.ty, "vec2"), true);
    expect_eq(std.mem.eql(u8, d1.name, "a_uv"), true);
    expect_eq(d1.location ?? 99, 1);

    const d2 = it.next() ?? unreachable;
    expect_eq(d2.qualifier == GlslQualifier.uniform, true);
    expect_eq(std.mem.eql(u8, d2.ty, "sampler2D"), true);
    expect_eq(std.mem.eql(u8, d2.name, "u_albedo"), true);
    expect_eq(d2.location == null, true);
    expect_eq(d2.binding ?? 99, 2);

    const d3 = it.next() ?? unreachable;
    expect_eq(d3.qualifier == GlslQualifier.uniform, true);
    expect_eq(std.mem.eql(u8, d3.ty, "mat4"), true);
    expect_eq(std.mem.eql(u8, d3.name, "u_mvp"), true);
    expect_eq(d3.location == null, true);
    expect_eq(d3.binding == null, true);

    const d4 = it.next() ?? unreachable;
    expect_eq(d4.qualifier == GlslQualifier.output, true);
    expect_eq(std.mem.eql(u8, d4.ty, "vec2"), true);
    expect_eq(std.mem.eql(u8, d4.name, "v_uv"), true);
    expect_eq(d4.location ?? 99, 0);

    // Function parameters (in vec3 p, out vec3 q) must NOT be yielded
    expect_eq(it.next() == null, true);
});

test("4. validateGlslBraces handles comments/preproc and rejects unbalanced delimiters", () => {
    expect_eq(validateGlslBraces(VERT_SHADER), true);
    expect_eq(validateGlslBraces("#define LBRACE {\nvoid main() { /* ( [ */ float a[4]; }"), true);

    expect_eq(validateGlslBraces("void main() { float a[4); }"), false);
    expect_eq(validateGlslBraces("void main() { "), false);
    expect_eq(validateGlslBraces("void main() }"), false);
    expect_eq(validateGlslBraces("/* unclosed comment void main() {}"), false);
});

test("5. Interface blocks, array uniforms, and invalid tokens are handled cleanly", () => {
    const ubo_src =
        "#version 450\n" ++
        "layout(binding = 0) uniform CameraBlock { mat4 view; mat4 proj; } cam;\n" ++
        "layout(location = 3) out vec4 frag_color;\n";
    let it = GlslDeclIter.init(ubo_src);
    const d = it.next() ?? unreachable;
    expect_eq(d.qualifier == GlslQualifier.output, true);
    expect_eq(std.mem.eql(u8, d.name, "frag_color"), true);
    expect_eq(d.location ?? 99, 3);
    expect_eq(it.next() == null, true);

    // Invalid token '@' in GLSL
    let lex = GlslIter.init("@bad");
    const bad = lex.next() ?? unreachable;
    expect_eq(bad.kind == GlslTokenKind.invalid, true);
    expect_eq(validateGlslBraces("@bad"), false);
});
