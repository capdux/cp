// Origin: Pure-CP Stdlib Test Suite (lib/prompt/compiler.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Compiled Zero-Alloc Prompt Template Engine:
//   1. Variable interpolation, |fallback defaults, strict missing_variable vs lenient mode
//   2. Nested {{#if}}/{{else}}/{{/if}} conditionals & list truthiness
//   3. {{#each}} list iteration with {{@index}}, {{.}}, empty list skip & nested #if
//   4. Static metadata introspection (`collectRequiredVars` & `collectAllReferencedVars`)
//   5. Compile-time syntax guards: unclosed_tag, empty_tag, mismatched_close_tag, unclosed_block
//   6. Nesting depth boundary (16 ok, 17 -> nesting_too_deep)
//   7. Empty template & undersized output buffer (`buffer_too_small`) guards
//   8. Instruction table overflow (`instruction_overflow`) & context overwrite/limits

import { test, expect_eq } from "cap:test";
import {
    TPL_MAX_INSTRUCTIONS,
    TPL_MAX_NESTING_DEPTH,
    TPL_MAX_CONTEXT_VARS,
    TPL_MAX_CONTEXT_LISTS,
    TPL_MAX_LIST_ITEMS,
    TPL_MAX_MISSING_NAME,
    TplOpKind,
    TplCompileStatus,
    TplExecStatus,
    TplInstruction,
    CompiledPromptTemplate,
    TplRenderContext,
    TplExecResult,
    compilePromptTemplate,
    renderCompiledTemplate,
    collectRequiredVars,
    collectAllReferencedVars,
} from "cap:prompt";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Variables, Defaults & Strict vs Lenient Missing Mode
// ============================================================================

fn testVarsDefaultsAndStrictMode()! {
    const tpl: CompiledPromptTemplate = compilePromptTemplate(
        "Hello {{ user }}, role={{ role|assistant }}, extra={{ missing_key }}!",
    );
    expect_eq(tpl.isValid(), true);

    let ctx = TplRenderContext.init();
    expect_eq(ctx.setVar("user", "Alice"), true);

    let out: [128]u8 = [0; 128];
    let out_s: []u8 = out[0..];

    // Strict mode: missing_key has no fallback -> returns missing_variable with exact name
    const r_strict: TplExecResult = renderCompiledTemplate(tpl, ctx, true, *out_s);
    expect_eq(r_strict.status == TplExecStatus.missing_variable, true);
    expect_eq(bytesEq(r_strict.missingVarSlice(), "missing_key"), true);

    // Lenient mode: missing_key emits empty string, role uses fallback "assistant"
    const r_lenient = renderCompiledTemplate(tpl, ctx, false, *out_s);
    expect_eq(r_lenient.status == TplExecStatus.ok, true);
    expect_eq(r_lenient.substitutions, 2);
    expect_eq(bytesEq(out[0..r_lenient.bytes_written], "Hello Alice, role=assistant, extra=!"), true);

    // Bind role & missing_key -> overrides fallback and passes strict mode
    expect_eq(ctx.setVar("role", "architect"), true);
    expect_eq(ctx.setVar("missing_key", "42"), true);
    const r_full = renderCompiledTemplate(tpl, ctx, true, *out_s);
    expect_eq(r_full.status == TplExecStatus.ok, true);
    expect_eq(r_full.substitutions, 3);
    expect_eq(bytesEq(out[0..r_full.bytes_written], "Hello Alice, role=architect, extra=42!"), true);
}

// ============================================================================
// Test 2: Nested {{#if}}/{{else}}/{{/if}} Conditionals
// ============================================================================

fn testNestedIfElseBlocks()! {
    const tpl = compilePromptTemplate(
        "[{{#if admin}}ADMIN:{{#if debug}}DBG{{else}}PROD{{/if}}{{else}}GUEST{{/if}}]",
    );
    expect_eq(tpl.isValid(), true);
    expect_eq(tpl.max_depth_seen, 2);

    let out: [64]u8 = [0; 64];
    let out_s: []u8 = out[0..];

    // Case 1: admin=true, debug=true -> [ADMIN:DBG]
    let c1 = TplRenderContext.init();
    * = c1.setBool("admin", true);
    * = c1.setBool("debug", true);
    const r1 = renderCompiledTemplate(tpl, c1, true, *out_s);
    expect_eq(r1.status == TplExecStatus.ok, true);
    expect_eq(bytesEq(out[0..r1.bytes_written], "[ADMIN:DBG]"), true);

    // Case 2: admin=true, debug=false -> [ADMIN:PROD]
    * = c1.setBool("debug", false);
    const r2 = renderCompiledTemplate(tpl, c1, true, *out_s);
    expect_eq(r2.status == TplExecStatus.ok, true);
    expect_eq(bytesEq(out[0..r2.bytes_written], "[ADMIN:PROD]"), true);

    // Case 3: admin=false -> [GUEST]
    * = c1.setBool("admin", false);
    const r3 = renderCompiledTemplate(tpl, c1, true, *out_s);
    expect_eq(r3.status == TplExecStatus.ok, true);
    expect_eq(bytesEq(out[0..r3.bytes_written], "[GUEST]"), true);
}

// ============================================================================
// Test 3: {{#each}} List Expansion with {{@index}}, {{.}} & Nested #if
// ============================================================================

fn testEachListExpansionAndNestedIf()! {
    const tpl = compilePromptTemplate(
        "Tools({{owner}}):{{#if tools}}{{#each tools}} [{{@index}}]={{.}}{{/each}}{{else}} none{{/if}}",
    );
    expect_eq(tpl.isValid(), true);

    let ctx = TplRenderContext.init();
    * = ctx.setVar("owner", "bot");
    const list_items: [3][]const u8 = ["search", "calc", "code"];
    expect_eq(ctx.setList("tools", list_items[0..]), true);

    let out: [128]u8 = [0; 128];
    let out_s: []u8 = out[0..];
    const r1 = renderCompiledTemplate(tpl, ctx, true, *out_s);
    expect_eq(r1.status == TplExecStatus.ok, true);
    expect_eq(bytesEq(out[0..r1.bytes_written], "Tools(bot): [0]=search [1]=calc [2]=code"), true);

    // Empty list -> falsy in #if and skipped in #each -> renders "Tools(bot): none"
    const empty_items: [0][]const u8 = [];
    expect_eq(ctx.setList("tools", empty_items[0..]), true);
    const r2 = renderCompiledTemplate(tpl, ctx, true, *out_s);
    expect_eq(r2.status == TplExecStatus.ok, true);
    expect_eq(bytesEq(out[0..r2.bytes_written], "Tools(bot): none"), true);
}

// ============================================================================
// Test 4: Static Metadata Introspection (`collectRequiredVars`)
// ============================================================================

fn testStaticMetadataIntrospection()! {
    const tpl = compilePromptTemplate(
        "Hi {{user}}, tone={{tone|calm}}. {{#if has_tools}}{{#each tools}}{{@index}}:{{.}} by {{user}}{{/each}}{{/if}}",
    );
    expect_eq(tpl.isValid(), true);

    let req: [8][]const u8 = [""; 8];
    let req_s: [][]const u8 = req[0..];
    const n_req = collectRequiredVars(tpl, *req_s);
    // Required: user, has_tools, tools (excludes tone with fallback, @index, ., and duplicate user)
    expect_eq(n_req, 3);
    expect_eq(bytesEq(req[0], "user"), true);
    expect_eq(bytesEq(req[1], "has_tools"), true);
    expect_eq(bytesEq(req[2], "tools"), true);

    let all: [8][]const u8 = [""; 8];
    let all_s: [][]const u8 = all[0..];
    const n_all = collectAllReferencedVars(tpl, *all_s);
    expect_eq(n_all, 4);
    expect_eq(bytesEq(all[0], "user"), true);
    expect_eq(bytesEq(all[1], "tone"), true);
    expect_eq(bytesEq(all[2], "has_tools"), true);
    expect_eq(bytesEq(all[3], "tools"), true);
}

// ============================================================================
// Test 5: Compile-Time Syntax Guards
// ============================================================================

fn testCompileTimeSyntaxGuards()! {
    // Unclosed tag
    const t_unclosed = compilePromptTemplate("hello {{name");
    expect_eq(t_unclosed.status == TplCompileStatus.unclosed_tag, true);
    expect_eq(t_unclosed.error_offset, 6);

    // Empty tags
    expect_eq(compilePromptTemplate("a{{  }}b").status == TplCompileStatus.empty_tag, true);
    expect_eq(compilePromptTemplate("{{#if }}x{{/if}}").status == TplCompileStatus.empty_tag, true);
    expect_eq(compilePromptTemplate("{{#each}}x{{/each}}").status == TplCompileStatus.empty_tag, true);
    expect_eq(compilePromptTemplate("{{|default}}").status == TplCompileStatus.empty_tag, true);

    // Mismatched / stray close tags & duplicate else
    expect_eq(compilePromptTemplate("{{#if a}}x{{/each}}").status == TplCompileStatus.mismatched_close_tag, true);
    expect_eq(compilePromptTemplate("{{#each a}}x{{/if}}").status == TplCompileStatus.mismatched_close_tag, true);
    expect_eq(compilePromptTemplate("{{else}}").status == TplCompileStatus.mismatched_close_tag, true);
    expect_eq(compilePromptTemplate("{{#if a}}1{{else}}2{{else}}3{{/if}}").status == TplCompileStatus.mismatched_close_tag, true);

    // Unclosed block
    expect_eq(compilePromptTemplate("{{#if a}}hello").status == TplCompileStatus.unclosed_block, true);

    // Rendering an invalid template returns invalid_template safely
    let out: [16]u8 = [0; 16];
    let out_s: []u8 = out[0..];
    expect_eq(renderCompiledTemplate(t_unclosed, TplRenderContext.init(), true, *out_s).status == TplExecStatus.invalid_template, true);
}

// ============================================================================
// Test 6: Nesting Depth Boundary (16 OK, 17 Rejected)
// ============================================================================

fn testNestingDepthBoundary()! {
    // 16 nested {{#if a}} ... {{/if}}
    const ok16 = compilePromptTemplate(
        "{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}" ++
        "{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}" ++
        "OK" ++
        "{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}" ++
        "{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}",
    );
    expect_eq(ok16.status == TplCompileStatus.ok, true);
    expect_eq(ok16.max_depth_seen, 16);

    let ctx = TplRenderContext.init();
    * = ctx.setBool("a", true);
    let out: [16]u8 = [0; 16];
    let out_s: []u8 = out[0..];
    const r = renderCompiledTemplate(ok16, ctx, true, *out_s);
    expect_eq(r.status == TplExecStatus.ok, true);
    expect_eq(bytesEq(out[0..r.bytes_written], "OK"), true);

    // 17 nested blocks -> nesting_too_deep
    const bad17 = compilePromptTemplate(
        "{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}" ++
        "{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}{{#if a}}" ++
        "X" ++
        "{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}" ++
        "{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}{{/if}}",
    );
    expect_eq(bad17.status == TplCompileStatus.nesting_too_deep, true);
}

// ============================================================================
// Test 7: Empty Template & Undersized Output Buffer Guards
// ============================================================================

fn testEmptyTemplateAndBufferTooSmall()! {
    const empty = compilePromptTemplate("");
    expect_eq(empty.isValid(), true);
    expect_eq(empty.inst_count, 0);

    let out: [4]u8 = [0; 4];
    let out_s: []u8 = out[0..];
    const r0 = renderCompiledTemplate(empty, TplRenderContext.init(), true, *out_s);
    expect_eq(r0.status == TplExecStatus.ok, true);
    expect_eq(r0.bytes_written, 0);
    expect_eq(r0.substitutions, 0);

    // Output buffer too small -> buffer_too_small without OOB
    const tpl = compilePromptTemplate("Hello {{name}}!");
    let ctx = TplRenderContext.init();
    * = ctx.setVar("name", "World");
    const r_small = renderCompiledTemplate(tpl, ctx, true, *out_s);
    expect_eq(r_small.status == TplExecStatus.buffer_too_small, true);
}

// ============================================================================
// Test 8: Instruction Table Overflow & Constants Sanity
// ============================================================================

fn testInstructionOverflowAndConstants()! {
    // 70 repetitions of "a{{x}}" = 140 instructions > TPL_MAX_INSTRUCTIONS (128)
    const big_src =
        "a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}" ++
        "a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}" ++
        "a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}" ++
        "a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}" ++
        "a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}" ++
        "a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}" ++
        "a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}a{{x}}";
    const res = compilePromptTemplate(big_src);
    expect_eq(res.status == TplCompileStatus.instruction_overflow, true);

    expect_eq(TPL_MAX_INSTRUCTIONS, 128);
    expect_eq(TPL_MAX_NESTING_DEPTH, 16);
    expect_eq(TPL_MAX_CONTEXT_VARS, 32);
    expect_eq(TPL_MAX_CONTEXT_LISTS, 16);
    expect_eq(TPL_MAX_LIST_ITEMS, 32);
    expect_eq(TPL_MAX_MISSING_NAME, 64);
    expect_eq(TplOpKind.literal != TplOpKind.var_lookup, true);
    * = TplInstruction{};
}

// ============================================================================
// Suite Registration
// ============================================================================

test("prompt.compiler: variables, |fallback defaults, strict missing & lenient mode", testVarsDefaultsAndStrictMode);
test("prompt.compiler: nested {{#if}}/{{else}}/{{/if}} conditionals", testNestedIfElseBlocks);
test("prompt.compiler: {{#each}} list expansion with {{@index}}, {{.}} & nested #if", testEachListExpansionAndNestedIf);
test("prompt.compiler: static metadata introspection (collectRequiredVars / collectAllReferencedVars)", testStaticMetadataIntrospection);
test("prompt.compiler: compile-time syntax error guards (unclosed, empty, mismatched, unclosed_block)", testCompileTimeSyntaxGuards);
test("prompt.compiler: nesting depth boundary (16 ok, 17 nesting_too_deep)", testNestingDepthBoundary);
test("prompt.compiler: empty template & undersized output buffer guards", testEmptyTemplateAndBufferTooSmall);
test("prompt.compiler: instruction table overflow (128) & constants sanity", testInstructionOverflowAndConstants);
