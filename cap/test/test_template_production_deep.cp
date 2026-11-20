// Source: cp/cap/test/test_template_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:template

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TemplateValueTag,
    TemplateEntry,
    TemplateValue,
    TemplateContext,
    TemplateError,
    TemplateOpKind,
    TemplateOp,
    CompiledTemplate,
    TemplateEngine,
    DEFAULT_MAX_DEPTH,
} from "cap:template";

fn eql(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

fn testEscapeRawAndDotPaths(): !void {
    const alloc = std.heap.page_allocator;
    let engine = TemplateEngine.init(alloc);
    defer engine.deinit();

    // Verify TemplateValue factories & isTruthy()
    expect_eq(TemplateValue.fromNull().isTruthy(), false);
    expect_eq(TemplateValue.fromBool(false).isTruthy(), false);
    expect_eq(TemplateValue.fromBool(true).isTruthy(), true);
    expect_eq(TemplateValue.fromInt(0).isTruthy(), false);
    expect_eq(TemplateValue.fromInt(42).isTruthy(), true);
    expect_eq(TemplateValue.fromFloat(0.0).isTruthy(), false);
    expect_eq(TemplateValue.fromFloat(3.5).isTruthy(), true);
    expect_eq(TemplateValue.fromStr("").isTruthy(), false);
    expect_eq(TemplateValue.fromStr("ok").isTruthy(), true);

    const empty_list: []const TemplateValue = [];
    expect_eq(TemplateValue.fromList(empty_list).isTruthy(), false);
    const non_empty_list: []const TemplateValue = [TemplateValue.fromInt(1)];
    expect_eq(TemplateValue.fromList(non_empty_list).isTruthy(), true);

    const empty_map: []const TemplateEntry = [];
    expect_eq(TemplateValue.fromMap(empty_map).isTruthy(), true);

    // Build nested context: user.profile.name, items.0.title, xss payload
    const profile_entries: []const TemplateEntry = [
        TemplateEntry.init("name", TemplateValue.fromStr("Alice <Admin> & \"Bob\" 'Eve'")),
        TemplateEntry.init("level", TemplateValue.fromInt(7)),
    ];
    const user_entries: []const TemplateEntry = [
        TemplateEntry.init("profile", TemplateValue.fromMap(profile_entries)),
    ];
    const item0_entries: []const TemplateEntry = [
        TemplateEntry.init("title", TemplateValue.fromStr("<b>First</b>")),
    ];
    const items: []const TemplateValue = [
        TemplateValue.fromMap(item0_entries),
    ];
    const root_entries: []const TemplateEntry = [
        TemplateEntry.init("user", TemplateValue.fromMap(user_entries)),
        TemplateEntry.init("items", TemplateValue.fromList(items)),
        TemplateEntry.init("raw_html", TemplateValue.fromStr("<span class=\"tag\">CP & Zig</span>")),
        TemplateEntry.init("missing_val", TemplateValue.fromNull()),
    ];

    const ctx = TemplateContext.init(TemplateValue.fromMap(root_entries));

    // Default {{var}} escapes &, <, >, ", '
    const out1 = engine.render(
        "Name={{user.profile.name}}; Level={{ user.profile.level }}; Item0={{items.0.title}}; Missing=[{{missing_val}}][{{none}}]",
        ctx,
    )?;
    defer alloc.free(out1);
    expect_eq(
        eql(
            out1,
            "Name=Alice &lt;Admin&gt; &amp; &quot;Bob&quot; &#39;Eve&#39;; Level=7; Item0=&lt;b&gt;First&lt;/b&gt;; Missing=[][]",
        ),
        true,
    );

    // Triple mustache {{{raw}}} and {{& raw}} output unescaped
    const out2 = engine.render(
        "A={{{raw_html}}}|B={{& raw_html}}|C={{{ user.profile.name }}}",
        ctx,
    )?;
    defer alloc.free(out2);
    expect_eq(
        eql(
            out2,
            "A=<span class=\"tag\">CP & Zig</span>|B=<span class=\"tag\">CP & Zig</span>|C=Alice <Admin> & \"Bob\" 'Eve'",
        ),
        true,
    );
}

fn testSectionsLoopsScopeAndInverted(): !void {
    const alloc = std.heap.page_allocator;
    let engine = TemplateEngine.init(alloc);
    defer engine.deinit();

    const tags: []const TemplateValue = [
        TemplateValue.fromStr("ai"),
        TemplateValue.fromStr("cp"),
        TemplateValue.fromStr("zig"),
    ];
    const empty_users: []const TemplateValue = [];

    const u1_entries: []const TemplateEntry = [
        TemplateEntry.init("name", TemplateValue.fromStr("Neo")),
        TemplateEntry.init("role", TemplateValue.fromStr("Architect")),
    ];
    const u2_entries: []const TemplateEntry = [
        TemplateEntry.init("name", TemplateValue.fromStr("Trinity")),
    ];
    const users: []const TemplateValue = [
        TemplateValue.fromMap(u1_entries),
        TemplateValue.fromMap(u2_entries),
    ];

    const meta_entries: []const TemplateEntry = [
        TemplateEntry.init("env", TemplateValue.fromStr("prod")),
        TemplateEntry.init("active", TemplateValue.fromBool(true)),
    ];

    const root_entries: []const TemplateEntry = [
        TemplateEntry.init("site", TemplateValue.fromStr("CP-Cloud")),
        TemplateEntry.init("role", TemplateValue.fromStr("Guest")),
        TemplateEntry.init("show_banner", TemplateValue.fromBool(true)),
        TemplateEntry.init("hide_banner", TemplateValue.fromBool(false)),
        TemplateEntry.init("tags", TemplateValue.fromList(tags)),
        TemplateEntry.init("empty_users", TemplateValue.fromList(empty_users)),
        TemplateEntry.init("users", TemplateValue.fromList(users)),
        TemplateEntry.init("meta", TemplateValue.fromMap(meta_entries)),
    ];
    const ctx = TemplateContext.init(TemplateValue.fromMap(root_entries));

    // 1. Boolean section, inverted section, and comment ignoring
    const out1 = engine.render(
        "{{! Banner section }}{{#show_banner}}[ON:{{site}}]{{/show_banner}}{{#hide_banner}}[HIDDEN]{{/hide_banner}}{{^hide_banner}}[FALLBACK]{{/hide_banner}}",
        ctx,
    )?;
    defer alloc.free(out1);
    expect_eq(eql(out1, "[ON:CP-Cloud][FALLBACK]"), true);

    // 2. List iteration with {{.}} and empty list inverted fallback
    const out2 = engine.render(
        "Tags:{{#tags}}({{.}}){{/tags}}|Empty:{{#empty_users}}X{{/empty_users}}{{^empty_users}}NONE{{/empty_users}}",
        ctx,
    )?;
    defer alloc.free(out2);
    expect_eq(eql(out2, "Tags:(ai)(cp)(zig)|Empty:NONE"), true);

    // 3. Map scope push + parent context fallback (u2 has no role, inherits Guest; u1 shadows role -> Architect)
    const out3 = engine.render(
        "{{#users}}[{{name}}:{{role}}@{{site}}]{{/users}}",
        ctx,
    )?;
    defer alloc.free(out3);
    expect_eq(eql(out3, "[Neo:Architect@CP-Cloud][Trinity:Guest@CP-Cloud]"), true);

    // 4. Standalone section lines & comments trim surrounding line whitespace cleanly
    const tpl_multiline =
        "Header\n" ++
        "  {{! ignored comment }}\n" ++
        "  {{#meta}}\n" ++
        "  env={{env}},site={{site}}\n" ++
        "  {{/meta}}\n" ++
        "Footer";
    const out4 = engine.render(tpl_multiline, ctx)?;
    defer alloc.free(out4);
    expect_eq(eql(out4, "Header\n  env=prod,site=CP-Cloud\nFooter"), true);
}

fn testPartialsAndRecursionGuard(): !void {
    const alloc = std.heap.page_allocator;
    let engine = TemplateEngine.init(alloc);
    defer engine.deinit();

    expect_eq(engine.max_depth, DEFAULT_MAX_DEPTH);

    * = engine.registerPartial("badge", "<span class=\"badge\">{{.}}</span>")?;
    * = engine.registerPartial("user_card", "<div class=\"card\">{{name}}: {{#badges}}{{> badge}}{{/badges}}</div>")?;
    // Overwrite partial test
    * = engine.registerPartial("footer", "v1")?;
    * = engine.registerPartial("footer", "v2-{{year}}")?;
    expect_eq(engine.hasPartial("badge"), true);
    expect_eq(engine.partialCount(), 3);

    const badges: []const TemplateValue = [
        TemplateValue.fromStr("pro"),
        TemplateValue.fromStr("dev"),
    ];
    const root_entries: []const TemplateEntry = [
        TemplateEntry.init("name", TemplateValue.fromStr("Alice")),
        TemplateEntry.init("badges", TemplateValue.fromList(badges)),
        TemplateEntry.init("year", TemplateValue.fromInt(2026)),
    ];
    const ctx = TemplateContext.init(TemplateValue.fromMap(root_entries));

    const out = engine.render("{{> user_card}}|{{> footer}}", ctx)?;
    defer alloc.free(out);
    expect_eq(
        eql(
            out,
            "<div class=\"card\">Alice: <span class=\"badge\">pro</span><span class=\"badge\">dev</span></div>|v2-2026",
        ),
        true,
    );

    // Missing partial triggers TemplateError.PartialNotFound
    const missing_res = engine.render("{{> does_not_exist}}", ctx);
    const missing_tag = switch (missing_res) {
        TemplateError.PartialNotFound => "partial_not_found",
        err(e) => "other_err",
        ok => "unexpected_ok",
    };
    expect_eq(eql(missing_tag, "partial_not_found"), true);

    // Recursive partial triggers TemplateError.MaxDepthExceeded (depth 32 default & custom depth)
    * = engine.registerPartial("self_loop", "x{{> self_loop}}")?;
    const loop_res = engine.render("{{> self_loop}}", ctx);
    const loop_tag = switch (loop_res) {
        TemplateError.MaxDepthExceeded => "max_depth",
        err(e) => "other_err",
        ok => "unexpected_ok",
    };
    expect_eq(eql(loop_tag, "max_depth"), true);

    // Custom depth limit (e.g. max_depth = 3)
    engine.setMaxDepth(3);
    * = engine.registerPartial("countdown", "{{#next}}{{> countdown}}{{/next}}{{^next}}end{{/next}}")?;
    const n4_entries: []const TemplateEntry = [TemplateEntry.init("next", TemplateValue.fromNull())];
    const n3_entries: []const TemplateEntry = [TemplateEntry.init("next", TemplateValue.fromMap(n4_entries))];
    const n2_entries: []const TemplateEntry = [TemplateEntry.init("next", TemplateValue.fromMap(n3_entries))];
    const n1_entries: []const TemplateEntry = [TemplateEntry.init("next", TemplateValue.fromMap(n2_entries))];
    const deep_ctx = TemplateContext.init(TemplateValue.fromMap(n1_entries));
    const deep_res = engine.render("{{> countdown}}", deep_ctx);
    const deep_tag = switch (deep_res) {
        TemplateError.MaxDepthExceeded => "max_depth",
        err(e) => "other_err",
        ok => "unexpected_ok",
    };
    expect_eq(eql(deep_tag, "max_depth"), true);
}

fn testErrorsAndAiPromptRenderRaw(): !void {
    const alloc = std.heap.page_allocator;
    let engine = TemplateEngine.init(alloc);
    defer engine.deinit();
    const ctx = TemplateContext.initEmpty();

    // 1. Unclosed {{ tag
    const r_unclosed = engine.render("Hello {{name", ctx);
    const tag_unclosed = switch (r_unclosed) {
        TemplateError.UnclosedTag => "unclosed_tag",
        else => "other",
    };
    expect_eq(eql(tag_unclosed, "unclosed_tag"), true);

    // 2. Unclosed {{{ triple tag
    const r_unclosed3 = engine.render("Hello {{{raw}}", ctx);
    const tag_unclosed3 = switch (r_unclosed3) {
        TemplateError.UnclosedTag => "unclosed_tag",
        else => "other",
    };
    expect_eq(eql(tag_unclosed3, "unclosed_tag"), true);

    // 3. Unclosed section {{#sec}}
    const r_unclosed_sec = engine.render("{{#sec}}body", ctx);
    const tag_unclosed_sec = switch (r_unclosed_sec) {
        TemplateError.UnclosedSection => "unclosed_sec",
        else => "other",
    };
    expect_eq(eql(tag_unclosed_sec, "unclosed_sec"), true);

    // 4. Mismatched section {{#a}}...{{/b}}
    const r_mismatch = engine.render("{{#a}}body{{/b}}", ctx);
    const tag_mismatch = switch (r_mismatch) {
        TemplateError.MismatchedSection => "mismatch",
        else => "other",
    };
    expect_eq(eql(tag_mismatch, "mismatch"), true);

    // 5. Unexpected closing section {{/orphan}}
    const r_orphan = engine.render("text {{/orphan}}", ctx);
    const tag_orphan = switch (r_orphan) {
        TemplateError.UnexpectedClosingSection => "unexpected_close",
        else => "other",
    };
    expect_eq(eql(tag_orphan, "unexpected_close"), true);

    // 6. Empty tag {{ }}
    const r_empty = engine.render("{{  }}", ctx);
    const tag_empty = switch (r_empty) {
        TemplateError.EmptyTag => "empty_tag",
        else => "other",
    };
    expect_eq(eql(tag_empty, "empty_tag"), true);

    // 7. AI Prompt generation with renderRaw (no HTML escaping on {{var}}) & CompiledTemplate reuse
    const tool1_entries: []const TemplateEntry = [
        TemplateEntry.init("name", TemplateValue.fromStr("read_file")),
        TemplateEntry.init("sig", TemplateValue.fromStr("fn(path: string) -> Result<string>")),
    ];
    const tools: []const TemplateValue = [TemplateValue.fromMap(tool1_entries)];
    const prompt_entries: []const TemplateEntry = [
        TemplateEntry.init("system_role", TemplateValue.fromStr("You are <Agent-01> & \"System\"")),
        TemplateEntry.init("tools", TemplateValue.fromList(tools)),
        TemplateEntry.init("query", TemplateValue.fromStr("Check `a < b && c > d` in 'main.cp'")),
    ];
    const prompt_ctx = TemplateContext.init(TemplateValue.fromMap(prompt_entries));

    const prompt_tpl =
        "System: {{system_role}}\n" ++
        "Tools:\n" ++
        "{{#tools}}- {{name}}: {{sig}}\n{{/tools}}" ++
        "User: {{query}}";

    const prompt_out = engine.renderRaw(prompt_tpl, prompt_ctx)?;
    defer alloc.free(prompt_out);
    expect_eq(
        eql(
            prompt_out,
            "System: You are <Agent-01> & \"System\"\n" ++
            "Tools:\n" ++
            "- read_file: fn(path: string) -> Result<string>\n" ++
            "User: Check `a < b && c > d` in 'main.cp'",
        ),
        true,
    );

    // CompiledTemplate reuse
    let compiled = engine.compile("Hi {{system_role}}")?;
    defer compiled.deinit();
    const c_out = engine.renderCompiledRaw(&compiled, prompt_ctx)?;
    defer alloc.free(c_out);
    expect_eq(eql(c_out, "Hi You are <Agent-01> & \"System\""), true);
}

fn testCapDynamicHubMount(): !void {
    const alloc = std.heap.page_allocator;
    let eng = cap.template.TemplateEngine.init(alloc);
    defer eng.deinit();

    expect_eq(cap.template.DEFAULT_MAX_DEPTH, 32 as usize);
    * = eng.registerPartial("greeting", "Hello, {{user}}!")?;

    let entries: [2]cap.template.TemplateEntry = undefined;
    entries[0] = cap.template.TemplateEntry.init("user", cap.template.TemplateValue.fromStr("CP & <Agent>"));
    entries[1] = cap.template.TemplateEntry.init("code", cap.template.TemplateValue.fromStr("x < y && y > 0"));
    const ctx = cap.template.TemplateContext.init(
        cap.template.TemplateValue.fromMap(&entries),
    );

    const html_out = eng.render("{{> greeting}} | {{code}}", ctx)?;
    defer alloc.free(html_out);
    expect_eq(
        eql(html_out, "Hello, CP &amp; &lt;Agent&gt;! | x &lt; y &amp;&amp; y &gt; 0"),
        true,
    );

    const raw_out = eng.renderRaw("{{> greeting}} | {{code}}", ctx)?;
    defer alloc.free(raw_out);
    expect_eq(
        eql(raw_out, "Hello, CP & <Agent>! | x < y && y > 0"),
        true,
    );
}

fn main() {
    test("cap:template {{var}} HTML escape, {{{raw}}}/{{&raw}} and dot-path resolution", () => {
        testEscapeRawAndDotPaths()?;
    });

    test("cap:template {{#section}} bool/list/map scope, {{^inverted}} and comments", () => {
        testSectionsLoopsScopeAndInverted()?;
    });

    test("cap:template {{> partial}} nesting, missing partial and max recursion depth guard", () => {
        testPartialsAndRecursionGuard()?;
    });

    test("cap:template syntax error detection and AI Prompt renderRaw mode", () => {
        testErrorsAndAiPromptRenderRaw()?;
    });

    test("cap.template lazy dynamic hub mount via import cap from \"cap\"", () => {
        testCapDynamicHubMount()?;
    });
}
