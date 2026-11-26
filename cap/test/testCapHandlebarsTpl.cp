// cap/test/testCapHandlebarsTpl.cp
// Comprehensive unit tests for @cap/handlebars-tpl (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    HbsContext,
    renderHandlebars,
} from "../../packages/@cap/handlebars-tpl/src/index.cp";

fn main() {
    test("1. @cap/handlebars-tpl — {{var}} HTML escaping vs {{{var}}} raw output", () => {
        let ctx = HbsContext.new();
        ctx.setStr("user", "<script>alert('xss')</script>");
        ctx.setStr("badge", "<b>PRO</b>");
        ctx.setCount("ver", 4);
        expect_eq(ctx.size(), 3 as usize);

        let out: [512]u8 = undefined;
        const n = renderHandlebars("Hi {{ user }} | {{{ badge }}} v{{ver}}", ctx, out[0..512]);
        expect_eq(
            out[0..n],
            "Hi &lt;script&gt;alert(&#39;xss&#39;)&lt;/script&gt; | <b>PRO</b> v4"
        );
    });

    test("2. @cap/handlebars-tpl — {{! comment }} and {{!-- block comment --}} stripping", () => {
        let ctx = HbsContext.new();
        ctx.setStr("msg", "ok");

        let out: [256]u8 = undefined;
        const tpl = "A{{! inline comment }}B{{!-- block {{msg}} comment --}}C:{{msg}}";
        const n = renderHandlebars(tpl, ctx, out[0..256]);
        expect_eq(out[0..n], "ABC:ok");
    });

    test("3. @cap/handlebars-tpl — {{#if}}...{{else}}...{{/if}} and {{#unless}} blocks", () => {
        let ctx = HbsContext.new();
        ctx.setBool("logged_in", true);
        ctx.setBool("banned", false);
        ctx.setStr("name", "Alice");

        let out: [256]u8 = undefined;
        const tpl1 = "{{#if logged_in}}Hi {{name}}{{else}}Guest{{/if}} | {{#unless banned}}Active{{else}}Blocked{{/unless}}";
        const n1 = renderHandlebars(tpl1, ctx, out[0..256]);
        expect_eq(out[0..n1], "Hi Alice | Active");

        // Flip booleans (updates existing entries in-place without growing size)
        ctx.setBool("logged_in", false);
        ctx.setBool("banned", true);
        expect_eq(ctx.size(), 3 as usize);

        const n2 = renderHandlebars(tpl1, ctx, out[0..256]);
        expect_eq(out[0..n2], "Guest | Blocked");
    });

    test("4. @cap/handlebars-tpl — {{#repeat}} and nested blocks", () => {
        let ctx = HbsContext.new();
        ctx.setCount("stars", 3);
        ctx.setBool("show", true);
        ctx.setBool("inner", false);

        let out: [256]u8 = undefined;
        const tpl = "{{#if show}}[{{#repeat stars}}{{#if inner}}x{{else}}*{{/if}}{{/repeat}}]{{else}}none{{/if}}";
        const n = renderHandlebars(tpl, ctx, out[0..256]);
        expect_eq(out[0..n], "[***]");
    });

    test("5. @cap/handlebars-tpl — built-in inline helpers {{upper}}, {{lower}}, and {{default}}", () => {
        let ctx = HbsContext.new();
        ctx.setStr("city", "Shanghai");
        ctx.setStr("empty_str", "");

        let out: [256]u8 = undefined;
        const tpl = "{{upper city}} / {{lower city}} / {{default missing \"Guest\"}} / {{default empty_str Fallback}} / {{default city None}}";
        const n = renderHandlebars(tpl, ctx, out[0..256]);
        expect_eq(out[0..n], "SHANGHAI / shanghai / Guest / Fallback / Shanghai");
    });

    test("6. @cap/handlebars-tpl — missing keys, empty template, and tight buffer safety", () => {
        const ctx = HbsContext.new();
        let out: [128]u8 = undefined;

        expect_eq(renderHandlebars("", ctx, out[0..128]), 0 as usize);
        const n1 = renderHandlebars("Hello {{unknown}}!", ctx, out[0..128]);
        expect_eq(out[0..n1], "Hello !");

        // Tight output buffer truncates cleanly without overflow
        let tiny: [5]u8 = undefined;
        const n2 = renderHandlebars("Hello World", ctx, tiny[0..5]);
        expect_eq(n2, 5 as usize);
        expect_eq(tiny[0..n2], "Hello");
    });
}
