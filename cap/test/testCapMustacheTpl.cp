import { test, expect_eq } from "cap:test";
import {
    MustacheContext,
    renderMustache,
} from "../../packages/mustache/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/mustache-tpl — {{var}} HTML escaping via @cap/escape-html & {{{raw}}}/{{& raw}}", () => {
        let ctx = MustacheContext.new();
        ctx.setStr("html", "<script>alert(\"x&y\")</script>");

        let out: [256]u8 = undefined;
        const n1 = renderMustache("Safe: {{html}}", ctx, out[0..256]);
        expect_eq(eqBytes(out[0..256], n1, "Safe: &lt;script&gt;alert(&quot;x&amp;y&quot;)&lt;/script&gt;"), true);

        const n2 = renderMustache("Raw1: {{{html}}}", ctx, out[0..256]);
        expect_eq(eqBytes(out[0..256], n2, "Raw1: <script>alert(\"x&y\")</script>"), true);

        const n3 = renderMustache("Raw2: {{& html}}", ctx, out[0..256]);
        expect_eq(eqBytes(out[0..256], n3, "Raw2: <script>alert(\"x&y\")</script>"), true);
    });

    test("2. @cap/mustache-tpl — {{#flag}} truthy & {{^flag}} inverted sections", () => {
        let ctx = MustacheContext.new();
        ctx.setBool("loggedIn", true);
        ctx.setStr("user", "Alice");
        ctx.setBool("banned", false);

        let out: [256]u8 = undefined;
        const tpl = "{{#loggedIn}}Hi {{user}}!{{/loggedIn}}{{^loggedIn}}Guest{{/loggedIn}}{{#banned}}[BANNED]{{/banned}}{{^banned}}[OK]{{/banned}}";
        const n = renderMustache(tpl, ctx, out[0..256]);
        expect_eq(eqBytes(out[0..256], n, "Hi Alice![OK]"), true);
    });

    test("3. @cap/mustache-tpl — Nested sections & list count repetition", () => {
        let ctx = MustacheContext.new();
        ctx.setBool("show", true);
        ctx.setListCount("items", 3);
        ctx.setListCount("empty", 0);

        let out: [256]u8 = undefined;
        const tpl = "{{#show}}<ul>{{#items}}<li>*</li>{{/items}}</ul>{{^empty}}none{{/empty}}{{/show}}";
        const n = renderMustache(tpl, ctx, out[0..256]);
        expect_eq(eqBytes(out[0..256], n, "<ul><li>*</li><li>*</li><li>*</li></ul>none"), true);
    });

    test("4. @cap/mustache-tpl — {{! comment }} stripping & missing variable fallback", () => {
        let ctx = MustacheContext.new();
        ctx.setStr("title", "CP");

        let out: [128]u8 = undefined;
        const tpl = "{{! ignore me }}{{title}}:{{missing}}!{{! end }}";
        const n = renderMustache(tpl, ctx, out[0..128]);
        expect_eq(eqBytes(out[0..128], n, "CP:!"), true);
    });

    test("5. @cap/mustache-tpl — Whitespace inside tags & literal single braces pass-through", () => {
        let ctx = MustacheContext.new();
        ctx.setStr("name", "Bob");

        let out: [128]u8 = undefined;
        const tpl = "{json: \"{{  name  }}\"} {{unclosed";
        const n = renderMustache(tpl, ctx, out[0..128]);
        expect_eq(eqBytes(out[0..128], n, "{json: \"Bob\"} {{unclosed"), true);
    });

    test("6. @cap/mustache-tpl — Empty template & small buffer safe truncation", () => {
        let ctx = MustacheContext.new();
        ctx.setStr("msg", "Hello World");

        let small: [8]u8 = undefined;
        expect_eq(renderMustache("", ctx, small[0..8]), 0);

        const n = renderMustache("Say: {{msg}}!", ctx, small[0..8]);
        expect_eq(n, 8);
        expect_eq(eqBytes(small[0..8], n, "Say: Hel"), true);
    });
}
