import { test, expect_eq } from "cap:test";
import { EjsContext, renderEjs } from "../../packages/@cap/ejs-tpl/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ejs-tpl — Escaped <%= %> vs Raw <%- %> interpolation", () => {
        let ctx = EjsContext.new();
        ctx.setStr("name", "<script>alert(1)</script>");
        ctx.setStr("html", "<b>ok</b>");
        let out: [256]u8 = undefined;
        let n = renderEjs("Hi <%= name %> | <%- html %>", ctx, out[0..256]);
        expect_eq(eqBuf(out[0..n], n, "Hi &lt;script&gt;alert(1)&lt;/script&gt; | <b>ok</b>"), true);
    });

    test("2. @cap/ejs-tpl — Comments <%# %> and literal <%% escaping", () => {
        let ctx = EjsContext.new();
        let out: [128]u8 = undefined;
        let n = renderEjs("A<%# hidden comment %>B <%%= raw %>", ctx, out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "AB <%= raw %>"), true);
    });

    test("3. @cap/ejs-tpl — <% if %> / <% else %> / <% end %> branching", () => {
        let ctx = EjsContext.new();
        ctx.setBool("admin", true);
        ctx.setBool("guest", false);
        let out: [128]u8 = undefined;
        let n1 = renderEjs("<% if admin %>YES<% else %>NO<% end %>", ctx, out[0..128]);
        expect_eq(eqBuf(out[0..n1], n1, "YES"), true);
        let n2 = renderEjs("<% if guest %>YES<% else %>NO<% end %>", ctx, out[0..128]);
        expect_eq(eqBuf(out[0..n2], n2, "NO"), true);
    });

    test("4. @cap/ejs-tpl — <% unless %> and nested <% repeat %> blocks", () => {
        let ctx = EjsContext.new();
        ctx.setBool("muted", false);
        ctx.setRepeatCount("stars", 3);
        ctx.setStr("item", "*");
        let out: [128]u8 = undefined;
        let n = renderEjs("<% unless muted %>[<% repeat stars %><%= item %><% end %>]<% end %>", ctx, out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "[***]"), true);
    });

    test("5. @cap/ejs-tpl — Whitespace slurping <%_ and -%>", () => {
        let ctx = EjsContext.new();
        ctx.setStr("x", "42");
        let out: [128]u8 = undefined;
        let n = renderEjs("A   <%_# trim %>B\n<%= x -%>\nC", ctx, out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "AB\n42C"), true);
    });

    test("6. @cap/ejs-tpl — Missing keys, unclosed tag fallback & small buffer truncation", () => {
        let ctx = EjsContext.new();
        ctx.setStr("k", "1234567890");
        let out: [16]u8 = undefined;
        let n1 = renderEjs("x=<%= missing %>", ctx, out[0..16]);
        expect_eq(eqBuf(out[0..n1], n1, "x="), true);
        let small: [5]u8 = undefined;
        let n2 = renderEjs("hi <%= k %>", ctx, small[0..5]);
        expect_eq(n2, 5);
        expect_eq(eqBuf(small[0..n2], n2, "hi 12"), true);
    });
}
