import { test, expect, expect_eq } from "cap:test";
import {
    CSS_LAYER_MAX_SEGMENTS,
    CSS_LAYER_MAX_STATEMENT_NAMES,
    CSS_LAYER_MAX_RULES,
    CSS_LAYER_MAX_REGISTRY,
    CssLayerError,
    CssLayerName,
    CssLayerStatementRule,
    CssLayerBlockRule,
    CssLayerRuleKind,
    CssLayerRule,
    CssLayerRuleList,
    CssLayerOrderRegistry,
} from "cap:css";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("css layer_rule: CssLayerName parsing, sublayer hierarchy, hash, and invalid input guards", fn()! {
    expect_eq(CSS_LAYER_MAX_SEGMENTS, 16);
    expect_eq(CSS_LAYER_MAX_STATEMENT_NAMES, 32);
    expect_eq(CSS_LAYER_MAX_RULES, 32);
    expect_eq(CSS_LAYER_MAX_REGISTRY, 64);

    const n = CssLayerName.parse("  framework.theme.dark-v2  ")?;
    expect_eq(n.count(), 3);
    expect(strEql(n.at(0) ?? "", "framework"));
    expect(strEql(n.at(1) ?? "", "theme"));
    expect(strEql(n.at(2) ?? "", "dark-v2"));
    expect(n.at(3) == null);

    let buf: [64]u8 = [0; 64];
    const written = n.toCss(buf[0..])?;
    expect(strEql(buf[0..written], "framework.theme.dark-v2"));

    const p1 = n.parent() ?? return err.TestFailed;
    expect_eq(p1.count(), 2);
    expect(n.isSublayerOf(p1));
    expect(n.isSameOrSublayerOf(p1));
    expect(!p1.isSublayerOf(n));

    const root = p1.parent() ?? return err.TestFailed;
    expect_eq(root.count(), 1);
    expect(root.parent() == null);
    expect(n.isSublayerOf(root));

    // Custom dashed ident & leading hyphen ident are valid CSS <ident>
    const dashed = CssLayerName.parse("--app.-sub_1")?;
    expect_eq(dashed.count(), 2);

    // Hash separation: "a.b" != "ab"
    const ab_dot = CssLayerName.parse("a.b")?;
    const ab_flat = CssLayerName.parse("ab")?;
    expect(!ab_dot.eql(ab_flat));
    expect(ab_dot.hash() != ab_flat.hash());

    // Invalid layer names must be rejected
    let rejected: usize = 0;
    if (_ := CssLayerName.parse("")) {} else { rejected += 1; }
    if (_ := CssLayerName.parse("a . b")) {} else { rejected += 1; }
    if (_ := CssLayerName.parse(".leading")) {} else { rejected += 1; }
    if (_ := CssLayerName.parse("trailing.")) {} else { rejected += 1; }
    if (_ := CssLayerName.parse("a..b")) {} else { rejected += 1; }
    if (_ := CssLayerName.parse("1digit")) {} else { rejected += 1; }
    if (_ := CssLayerName.parse("-")) {} else { rejected += 1; }
    if (_ := CssLayerName.parse("-1bad")) {} else { rejected += 1; }
    expect_eq(rejected, 8);
});

test("css layer_rule: CssLayerStatementRule parsing, pretty/minified serialization, and error guards", fn()! {
    const stmt = CssLayerStatementRule.parse("  @LAYER /* order */ reset, base, theme.dark, utilities ; ")?;
    expect_eq(stmt.count(), 4);
    expect(!stmt.isEmpty());

    const n2 = stmt.at(2) ?? return err.TestFailed;
    expect_eq(n2.count(), 2);
    expect(strEql(n2.at(0) ?? "", "theme"));
    expect(strEql(n2.at(1) ?? "", "dark"));

    let out: [128]u8 = [0; 128];
    const pretty_len = stmt.toCss(out[0..])?;
    expect(strEql(out[0..pretty_len], "@layer reset, base, theme.dark, utilities;"));

    const min_len = stmt.toCssMinified(out[0..])?;
    expect(strEql(out[0..min_len], "@layer reset,base,theme.dark,utilities;"));

    // Default empty statement serializes to "@layer;" (matches Bun LayerStatementRule.toCss)
    const empty_stmt = CssLayerStatementRule{};
    expect(empty_stmt.isEmpty());
    const empty_len = empty_stmt.toCss(out[0..])?;
    expect(strEql(out[0..empty_len], "@layer;"));

    // Invalid statements rejected
    let rejected: usize = 0;
    if (_ := CssLayerStatementRule.parse("@layer;")) {} else { rejected += 1; }
    if (_ := CssLayerStatementRule.parse("@layer a, ;")) {} else { rejected += 1; }
    if (_ := CssLayerStatementRule.parse("@layer a, , b;")) {} else { rejected += 1; }
    if (_ := CssLayerStatementRule.parse("@layer a b;")) {} else { rejected += 1; }
    if (_ := CssLayerStatementRule.parse("@layer a")) {} else { rejected += 1; }
    if (_ := CssLayerStatementRule.parse("@layer a { }")) {} else { rejected += 1; }
    if (_ := CssLayerStatementRule.parse("@layer a; trailing")) {} else { rejected += 1; }
    if (_ := CssLayerStatementRule.parse("@media screen;")) {} else { rejected += 1; }
    expect_eq(rejected, 8);
});

test("css layer_rule: CssLayerBlockRule named, anonymous, nested braces/strings, and invalid prelude", fn()! {
    const named = CssLayerBlockRule.parse(
        "@layer theme.dark { .card { content: \"}\"; /* } */ color: #fff; } }"
    )?;
    expect(!named.isAnonymous());
    expect(named.hasName());
    expect(strEql(named.body, ".card { content: \"}\"; /* } */ color: #fff; }"));

    let buf: [256]u8 = [0; 256];
    const p_len = named.toCss(buf[0..])?;
    expect(strEql(buf[0..p_len], "@layer theme.dark { .card { content: \"}\"; /* } */ color: #fff; } }"));

    const m_len = named.toCssMinified(buf[0..])?;
    expect(strEql(buf[0..m_len], "@layer theme.dark{.card { content: \"}\"; /* } */ color: #fff; }}"));

    // Anonymous block
    const anon = CssLayerBlockRule.parse("@layer { a { color: red; } }")?;
    expect(anon.isAnonymous());
    expect(!anon.hasName());
    const ap_len = anon.toCss(buf[0..])?;
    expect(strEql(buf[0..ap_len], "@layer { a { color: red; } }"));
    const am_len = anon.toCssMinified(buf[0..])?;
    expect(strEql(buf[0..am_len], "@layer{a { color: red; }}"));

    // Empty body block
    const empty_blk = CssLayerBlockRule.parse("@layer empty {   }")?;
    const ep_len = empty_blk.toCss(buf[0..])?;
    expect(strEql(buf[0..ep_len], "@layer empty {}"));

    // Invalid blocks rejected (comma list in block prelude, unclosed brace, trailing tokens)
    let rejected: usize = 0;
    if (_ := CssLayerBlockRule.parse("@layer a, b { p { color: red; } }")) {} else { rejected += 1; }
    if (_ := CssLayerBlockRule.parse("@layer a { p { color: red; }")) {} else { rejected += 1; }
    if (_ := CssLayerBlockRule.parse("@layer a { } extra")) {} else { rejected += 1; }
    if (_ := CssLayerBlockRule.parse("@layerfoo { }")) {} else { rejected += 1; }
    expect_eq(rejected, 4);
});

test("css layer_rule: CssLayerRule unified dispatch and roundtrip", fn()! {
    const r1 = CssLayerRule.parse("@layer reset, components;")?;
    expect(r1.isStatement());
    expect(!r1.isBlock());

    const r2 = CssLayerRule.parse("@layer components.btn { .btn { padding: 8px; } }")?;
    expect(r2.isBlock());
    expect(!r2.isStatement());

    let buf: [128]u8 = [0; 128];
    const n1 = r1.toCssMinified(buf[0..])?;
    expect(strEql(buf[0..n1], "@layer reset,components;"));

    const n2 = r2.toCss(buf[0..])?;
    expect(strEql(buf[0..n2], "@layer components.btn { .btn { padding: 8px; } }"));

    // Roundtrip parse of serialized output
    const r2_rt = CssLayerRule.parse(buf[0..n2])?;
    expect(r2_rt.isBlock());
    expect(r2_rt.block.eql(r2.block));
});

test("css layer_rule: CssLayerRuleList stylesheet extraction skips comments, strings, and other rules", fn()! {
    const sheet =
        "/* @layer commented_out; */\n" ++
        "@import url(\"base.css\");\n" ++
        "@layer reset, theme.base, utilities;\n" ++
        ".note { content: \"@layer inside_string;\"; }\n" ++
        "@media (min-width: 768px) { .grid { display: grid; } }\n" ++
        "@layer reset { * { box-sizing: border-box; } }\n" ++
        "@layer { .anon { opacity: 0.9; } }\n" ++
        "@layer theme.dark { body { background: #111; } }\n";

    const list = CssLayerRuleList.extractFromStylesheet(sheet)?;
    expect_eq(list.count(), 4);

    const item0 = list.at(0) ?? return err.TestFailed;
    expect(item0.isStatement());
    expect_eq(item0.statement.count(), 3);

    const item1 = list.at(1) ?? return err.TestFailed;
    expect(item1.isBlock());
    expect(!item1.block.isAnonymous());

    const item2 = list.at(2) ?? return err.TestFailed;
    expect(item2.isBlock());
    expect(item2.block.isAnonymous());

    const item3 = list.at(3) ?? return err.TestFailed;
    expect(item3.isBlock());
    const n3 = item3.block.name ?? return err.TestFailed;
    expect_eq(n3.count(), 2);
    expect(strEql(n3.at(1) ?? "", "dark"));
});

test("css layer_rule: CssLayerOrderRegistry W3C cascade priority ordering and buffer guard", fn()! {
    const sheet =
        "@layer reset, theme.base, utilities;\n" ++
        "@layer reset { * { margin: 0; } }\n" ++
        "@layer { .anon1 { color: red; } }\n" ++
        "@layer theme.dark { body { color: #eee; } }\n" ++
        "@layer { .anon2 { color: blue; } }\n" ++
        "@layer reset { a { text-decoration: none; } }\n";

    const list = CssLayerRuleList.extractFromStylesheet(sheet)?;
    let reg = CssLayerOrderRegistry.init();
    const added = reg.registerRuleList(list)?;
    // Expected order:
    // 0: reset
    // 1: theme (auto-expanded parent of theme.base)
    // 2: theme.base
    // 3: utilities
    // 4: <anonymous #1>
    // 5: theme.dark (parent `theme` already registered!)
    // 6: <anonymous #2>
    // (duplicate `reset` adds 0 slots and keeps priority 0)
    expect_eq(added, 7);
    expect_eq(reg.count(), 7);

    const l_reset = CssLayerName.parse("reset")?;
    const l_theme = CssLayerName.parse("theme")?;
    const l_base = CssLayerName.parse("theme.base")?;
    const l_util = CssLayerName.parse("utilities")?;
    const l_dark = CssLayerName.parse("theme.dark")?;
    const l_unknown = CssLayerName.parse("nonexistent")?;

    expect_eq(reg.priorityOf(l_reset) ?? 999, 0);
    expect_eq(reg.priorityOf(l_theme) ?? 999, 1);
    expect_eq(reg.priorityOf(l_base) ?? 999, 2);
    expect_eq(reg.priorityOf(l_util) ?? 999, 3);
    expect_eq(reg.priorityOf(l_dark) ?? 999, 5);
    expect(reg.priorityOf(l_unknown) == null);

    expect_eq(reg.comparePriority(l_reset, l_util) ?? 99, -1);
    expect_eq(reg.comparePriority(l_dark, l_base) ?? 99, 1);
    expect_eq(reg.comparePriority(l_reset, l_reset) ?? 99, 0);
    expect(reg.comparePriority(l_reset, l_unknown) == null);

    // BufferTooSmall guard on serialization
    let tiny: [4]u8 = [0; 4];
    let hit_small = false;
    if (_ := l_dark.toCss(tiny[0..])) {} else { hit_small = true; }
    expect(hit_small);
});
