// cap/test/testCapTemplate.cp
// Comprehensive unit tests for @cap/template:
// - Default HTML XSS auto-escaping (`& < > " '`) & raw `{{{ key }}}` output
// - Chained pipeline filters (`upper`, `lower`, `trim`, `len`) & `UnknownFilter`
// - Comment stripping (`{{! ... }}` and `{{!-- ... --}}`)
// - Nested `#if`/`else`/`#unless` conditionals & falsy semantics (`""`, `"false"`, `"0"`, undefined)
// - `#each` list iteration with `this`, `@index`, `@first`, `@last` and `{{else}}` fallback
// - Error guards: `UnclosedTag`, `UnclosedBlock`, `MismatchedBlock`, `DepthExceeded`, `ContextFull`, `BufferTooSmall`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MAX_BLOCK_DEPTH,
    MAX_VARS,
    TemplateError,
    TemplateContext,
    render,
    escapeHtml,
} from "../../packages/tinytemplate/src/index.cp";

fn expectRenderError(
    tpl: []const u8,
    ctx: *const TemplateContext,
    buf_len: usize,
    expected: TemplateError,
): bool {
    let buf: [512]u8 = undefined;
    * = render(tpl, ctx, buf[0..buf_len]) catch |e| {
        return e == expected;
    };
    return false;
}

fn expectEscapeTooSmall(input: []const u8, buf_len: usize): bool {
    let buf: [64]u8 = undefined;
    * = escapeHtml(input, buf[0..buf_len]) catch |e| {
        return e == TemplateError.BufferTooSmall;
    };
    return false;
}

fn expectContextFull(): bool {
    let full_ctx = TemplateContext.init();
    let keys: [65][4]u8 = undefined;
    let i: usize = 0;
    while (i < MAX_VARS) : (i += 1) {
        keys[i][0] = 'k';
        keys[i][1] = ('0' as u8) + ((i / 10) as u8);
        keys[i][2] = ('0' as u8) + ((i % 10) as u8);
        keys[i][3] = 0;
        full_ctx.set(keys[i][0..3], "v") catch return false;
    }
    let hit_full = false;
    full_ctx.set("overflow", "v") catch |e| {
        hit_full = (e == TemplateError.ContextFull);
    };
    return hit_full;
}

fn main() {
    test("1. @cap/template — Auto HTML XSS Escaping & Triple-Brace Raw Output", () => {
        let esc_buf: [128]u8 = undefined;
        const escaped = escapeHtml("<a href=\"x\" title='y'>&</a>", esc_buf[0..]) catch unreachable;
        expect_eq(
            std.mem.eql(u8, escaped, "&lt;a href=&quot;x&quot; title=&#39;y&#39;&gt;&amp;&lt;/a&gt;"),
            true,
        );
        expect_eq(expectEscapeTooSmall("<tag>", 6), true);

        let ctx = TemplateContext.init();
        ctx.set("user.name", "<script>alert('xss') & \"hi\"</script>") catch unreachable;
        ctx.set("raw_html", "<b>Safe</b>") catch unreachable;

        let out: [256]u8 = undefined;
        const res = render("User: {{ user.name }} | {{{ raw_html }}}", &ctx, out[0..]) catch unreachable;
        expect_eq(
            std.mem.eql(
                u8,
                res,
                "User: &lt;script&gt;alert(&#39;xss&#39;) &amp; &quot;hi&quot;&lt;/script&gt; | <b>Safe</b>",
            ),
            true,
        );
    });

    test("2. @cap/template — Context Types (String, Bool, Int, Dotted Keys & Update)", () => {
        let ctx = TemplateContext.init();
        ctx.set("user.name", "Alice") catch unreachable;
        ctx.set("user.name", "Bob") catch unreachable; // overwrite existing key
        ctx.setBool("active", true) catch unreachable;
        ctx.setBool("deleted", false) catch unreachable;
        ctx.setInt("count", 42) catch unreachable;
        ctx.setInt("neg", -1024) catch unreachable;
        ctx.setInt("zero", 0) catch unreachable;

        expect_eq(ctx.varCount(), 6 as usize);
        expect_eq(ctx.has("user.name"), true);
        expect_eq(ctx.has("missing"), false);

        let out: [256]u8 = undefined;
        const res = render(
            "{{user.name}}|{{active}}|{{deleted}}|{{count}}|{{neg}}|{{zero}}|{{missing}}",
            &ctx,
            out[0..],
        ) catch unreachable;
        expect_eq(std.mem.eql(u8, res, "Bob|true|false|42|-1024|0|"), true);

        ctx.reset();
        expect_eq(ctx.varCount(), 0 as usize);
        expect_eq(ctx.get("user.name") == null, true);
    });

    test("3. @cap/template — Chained Pipeline Filters & UnknownFilter Guard", () => {
        let ctx = TemplateContext.init();
        ctx.set("name", "  \t Hello, CP! \n ") catch unreachable;
        ctx.set("mixed", "AbCdEf") catch unreachable;
        ctx.set("raw_tag", "  <em>ok</em>  ") catch unreachable;

        let out: [256]u8 = undefined;
        const res1 = render(
            "{{ name | trim | upper }} / {{ mixed | lower }} / {{ name | trim | len }}",
            &ctx,
            out[0..],
        ) catch unreachable;
        expect_eq(std.mem.eql(u8, res1, "HELLO, CP! / abcdef / 10"), true);

        // Chained filter with triple-brace raw output
        const res2 = render("{{{ raw_tag | trim | upper }}}", &ctx, out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, res2, "<EM>OK</EM>"), true);

        // Unknown filter returns UnknownFilter
        expect_eq(expectRenderError("{{ name | bogus }}", &ctx, 128, TemplateError.UnknownFilter), true);
        expect_eq(expectRenderError("{{ name | trim | }}", &ctx, 128, TemplateError.UnknownFilter), true);
    });

    test("4. @cap/template — Comment Stripping", () => {
        let ctx = TemplateContext.init();
        ctx.set("who", "CP") catch unreachable;

        let out: [128]u8 = undefined;
        const res = render(
            "Hello{{! inline comment }}, {{who}}{{!-- multi {{ignored}} comment --}}!",
            &ctx,
            out[0..],
        ) catch unreachable;
        expect_eq(std.mem.eql(u8, res, "Hello, CP!"), true);
    });

    test("5. @cap/template — Nested #if / else / #unless & Falsy Semantics", () => {
        let ctx = TemplateContext.init();
        ctx.setBool("is_admin", true) catch unreachable;
        ctx.setBool("is_guest", false) catch unreachable;
        ctx.setInt("errors", 0) catch unreachable;
        ctx.set("empty_str", "") catch unreachable;
        ctx.set("role", "owner") catch unreachable;

        let out: [256]u8 = undefined;
        const tpl1 = "{{#if is_admin}}A:{{#if role}}{{role}}{{else}}none{{/if}}{{#unless errors}}[OK]{{/unless}}{{else}}Denied{{/if}}";
        const res1 = render(tpl1, &ctx, out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, res1, "A:owner[OK]"), true);

        // Falsy values: false, 0, empty string, undefined
        const tpl2 = "{{#if is_guest}}Y{{else}}N{{/if}}|{{#if errors}}Y{{else}}N{{/if}}|{{#if empty_str}}Y{{else}}N{{/if}}|{{#if undef_var}}Y{{else}}N{{/if}}";
        const res2 = render(tpl2, &ctx, out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, res2, "N|N|N|N"), true);

        // #unless with else branch
        const tpl3 = "{{#unless is_admin}}NoAdmin{{else}}YesAdmin{{/unless}}|{{#unless is_guest}}NotGuest{{else}}Guest{{/unless}}";
        const res3 = render(tpl3, &ctx, out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, res3, "YesAdmin|NotGuest"), true);

        // Inactive outer branch must suppress inner branches
        const tpl4 = "{{#if is_guest}}{{#if is_admin}}BAD{{/if}}{{#unless errors}}BAD2{{/unless}}{{else}}GOOD{{/if}}";
        const res4 = render(tpl4, &ctx, out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, res4, "GOOD"), true);
    });

    test("6. @cap/template — #each Iteration with this, @index, @first, @last & else", () => {
        let ctx = TemplateContext.init();
        ctx.pushListItem("items", "alpha") catch unreachable;
        ctx.pushListItem("items", "<beta>") catch unreachable;
        ctx.pushListItem("items", "gamma") catch unreachable;

        expect_eq(ctx.listLen("items"), 3 as usize);
        expect_eq(ctx.listLen("empty_list"), 0 as usize);

        let out: [256]u8 = undefined;
        const tpl = "{{#each items}}{{#if @first}}[{{/if}}{{@index}}={{this | upper}}({{@first}}/{{@last}}){{#unless @last}}, {{/unless}}{{#if @last}}]{{/if}}{{/each}}";
        const res = render(tpl, &ctx, out[0..]) catch unreachable;
        expect_eq(
            std.mem.eql(u8, res, "[0=ALPHA(true/false), 1=&lt;BETA&gt;(false/false), 2=GAMMA(false/true)]"),
            true,
        );

        // #each with {{else}} when list is non-empty vs empty
        const tpl_else_nonempty = "{{#each items}}{{this}};{{else}}EMPTY{{/each}}";
        const res_ne = render(tpl_else_nonempty, &ctx, out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, res_ne, "alpha;&lt;beta&gt;;gamma;"), true);

        const tpl_else_empty = "{{#each missing_list}}{{this}}{{else}}NO_ITEMS{{/each}}";
        const res_e = render(tpl_else_empty, &ctx, out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, res_e, "NO_ITEMS"), true);
    });

    test("7. @cap/template — Error Guards (UnclosedTag, UnclosedBlock, MismatchedBlock, DepthExceeded, BufferTooSmall, ContextFull)", () => {
        let ctx = TemplateContext.init();
        ctx.setBool("ok", true) catch unreachable;
        ctx.set("msg", "hello world") catch unreachable;

        // UnclosedTag
        expect_eq(expectRenderError("hi {{ msg", &ctx, 128, TemplateError.UnclosedTag), true);
        expect_eq(expectRenderError("hi {{{ msg }}", &ctx, 128, TemplateError.UnclosedTag), true);
        expect_eq(expectRenderError("hi {{! comment", &ctx, 128, TemplateError.UnclosedTag), true);
        expect_eq(expectRenderError("hi {{!-- comment }}", &ctx, 128, TemplateError.UnclosedTag), true);

        // UnclosedBlock
        expect_eq(expectRenderError("{{#if ok}}hello", &ctx, 128, TemplateError.UnclosedBlock), true);
        expect_eq(expectRenderError("{{#each items}}x", &ctx, 128, TemplateError.UnclosedBlock), true);

        // MismatchedBlock
        expect_eq(expectRenderError("{{#if ok}}x{{/unless}}", &ctx, 128, TemplateError.MismatchedBlock), true);
        expect_eq(expectRenderError("{{/if}}", &ctx, 128, TemplateError.MismatchedBlock), true);
        expect_eq(expectRenderError("{{else}}", &ctx, 128, TemplateError.MismatchedBlock), true);
        expect_eq(expectRenderError("{{#if ok}}a{{else}}b{{else}}c{{/if}}", &ctx, 128, TemplateError.MismatchedBlock), true);

        // DepthExceeded (17 nested #if blocks > MAX_BLOCK_DEPTH = 16)
        expect_eq(MAX_BLOCK_DEPTH, 16 as usize);
        const deep_tpl = "{{#if ok}}{{#if ok}}{{#if ok}}{{#if ok}}"
            ++ "{{#if ok}}{{#if ok}}{{#if ok}}{{#if ok}}"
            ++ "{{#if ok}}{{#if ok}}{{#if ok}}{{#if ok}}"
            ++ "{{#if ok}}{{#if ok}}{{#if ok}}{{#if ok}}"
            ++ "{{#if ok}}too deep{{/if}}"
            ++ "{{/if}}{{/if}}{{/if}}{{/if}}"
            ++ "{{/if}}{{/if}}{{/if}}{{/if}}"
            ++ "{{/if}}{{/if}}{{/if}}{{/if}}"
            ++ "{{/if}}{{/if}}{{/if}}{{/if}}";
        expect_eq(expectRenderError(deep_tpl, &ctx, 128, TemplateError.DepthExceeded), true);

        // BufferTooSmall
        expect_eq(expectRenderError("hello {{msg}}", &ctx, 8, TemplateError.BufferTooSmall), true);

        // ContextFull when exceeding MAX_VARS
        expect_eq(expectContextFull(), true);
    });
}
