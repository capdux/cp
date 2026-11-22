import { test, expect } from "cap:test";
import {
    CSS_IMPORT_NO_RECORD,
    CssImportLayer,
    CssImportSupports,
    CssImportConditions,
    CssImportRule,
    CssImportRuleList,
} from "cap:css";

fn eqBytes(a: []const u8, b: []const u8): bool {
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

test("css import_rule: basic URL forms, case-insensitivity and inline comments") {
    const r1 = CssImportRule.parse("@import \"./base.css\";")?;
    expect(eqBytes(r1.url, "./base.css"));
    expect(!r1.hasConditions());
    expect(!r1.hasImportRecord());
    expect(!r1.isExternalUrl());

    const r2 = CssImportRule.parse("  @IMPORT  'tokens.css'  ;")?;
    expect(eqBytes(r2.url, "tokens.css"));

    const r3 = CssImportRule.parse("@import url(\"./theme/dark.css\");")?;
    expect(eqBytes(r3.url, "./theme/dark.css"));

    const r4 = CssImportRule.parse("@import URL('./theme/light.css');")?;
    expect(eqBytes(r4.url, "./theme/light.css"));

    const r5 = CssImportRule.parse("@import /* c1 */ url( https://cdn.example.com/reset.css ) /* c2 */ ;")?;
    expect(eqBytes(r5.url, "https://cdn.example.com/reset.css"));
    expect(r5.isExternalUrl());
}

test("css import_rule: layer, supports and media conditions combination") {
    const r_anon = CssImportRule.parse("@import \"./a.css\" layer;")?;
    expect(r_anon.hasConditions());
    expect(r_anon.conditions.hasAnonymousLayer());
    expect(r_anon.conditions.layer.isPresent());
    expect(!r_anon.conditions.supports.isPresent());
    expect(r_anon.conditions.media.len == 0);

    const r_full = CssImportRule.parse(
        "@import url(\"./grid.css\") layer(framework.layout.grid) supports(display: grid) screen and (min-width: 768px);"
    )?;
    expect(eqBytes(r_full.url, "./grid.css"));
    expect(r_full.conditions.layer.kind == .named);
    expect(eqBytes(r_full.conditions.layer.name, "framework.layout.grid"));
    expect(!r_full.conditions.hasAnonymousLayer());
    expect(r_full.conditions.supports.kind == .declaration);
    expect(eqBytes(r_full.conditions.supports.raw, "display: grid"));
    expect(eqBytes(r_full.conditions.media, "screen and (min-width: 768px)"));

    const r_cond = CssImportRule.parse(
        "@import \"./modern.css\" supports((display: flex) and (gap: 1rem)) print;"
    )?;
    expect(!r_cond.conditions.layer.isPresent());
    expect(r_cond.conditions.supports.kind == .condition);
    expect(eqBytes(r_cond.conditions.supports.raw, "(display: flex) and (gap: 1rem)"));
    expect(eqBytes(r_cond.conditions.media, "print"));

    // layersEql / supportsEql 语义校验
    const r_same_layer = CssImportRule.parse("@import \"./other.css\" layer(framework.layout.grid);")?;
    expect(r_full.conditions.layersEql(r_same_layer.conditions));
    expect(!r_full.conditions.supportsEql(r_same_layer.conditions));
    expect(!r_full.conditions.layersEql(r_anon.conditions));
}

test("css import_rule: constructors, import_record_idx and external URL detection") {
    const r_url = CssImportRule.fromUrl("./local.css");
    expect(eqBytes(r_url.url, "./local.css"));
    expect(r_url.import_record_idx == CSS_IMPORT_NO_RECORD);
    expect(!r_url.hasImportRecord());
    expect(!r_url.isExternalUrl());

    const r_rec = CssImportRule.fromUrlAndImportRecordIdx("pkg/style.css", 7);
    expect(r_rec.hasImportRecord());
    expect(r_rec.import_record_idx == 7);

    const conds = CssImportConditions{
        layer: CssImportLayer.named("vendor.ui")?,
        supports: CssImportSupports.parse("selector(:has(*))")?,
        media: "all",
    };
    const r_conds = CssImportRule.fromConditionsAndUrl("https://fonts.example.com/css", conds);
    expect(r_conds.hasConditions());
    expect(r_conds.isExternalUrl());
    expect(r_conds.conditions.supports.kind == .condition);

    expect(CssImportRule.fromUrl("http://example.com/a.css").isExternalUrl());
    expect(CssImportRule.fromUrl("HTTPS://example.com/a.css").isExternalUrl());
    expect(CssImportRule.fromUrl("//cdn.example.com/a.css").isExternalUrl());
    expect(CssImportRule.fromUrl("data:text/css,body{}").isExternalUrl());
    expect(!CssImportRule.fromUrl("/assets/app.css").isExternalUrl());
    expect(!CssImportRule.fromUrl("../shared.css").isExternalUrl());
}

test("css import_rule: canonical serialization, placeholder rewrite and escaping") {
    const rule = CssImportRule.parse(
        "@import url('./theme.css') layer(base.tokens) supports(display: grid) screen and (prefers-color-scheme: dark);"
    )?;
    let buf: [256]u8 = [0; 256];
    const n1 = rule.toCss(buf[0..])?;
    expect(eqBytes(
        buf[0..n1],
        "@import \"./theme.css\" layer(base.tokens) supports(display: grid) screen and (prefers-color-scheme: dark);"
    ));

    // Bundler dependency placeholder 改写
    const n2 = rule.toCssWithPlaceholder("bun-css-dep://0", buf[0..])?;
    expect(eqBytes(
        buf[0..n2],
        "@import \"bun-css-dep://0\" layer(base.tokens) supports(display: grid) screen and (prefers-color-scheme: dark);"
    ));

    // 引号与反斜杠转义
    const esc_rule = CssImportRule.fromUrl("a\"b\\c.css");
    const n3 = esc_rule.toCss(buf[0..])?;
    expect(eqBytes(buf[0..n3], "@import \"a\\\"b\\\\c.css\";"));

    // 缓冲区过小防护
    let tiny: [8]u8 = [0; 8];
    const small_res = rule.toCss(tiny[0..]) catch {
        expect(true);
        return;
    };
    * = small_res;
    expect(false);
}

test("css import_rule: stylesheet leading extraction and late-import diagnostics") {
    const css =
        "/* header comment */\n" ++
        "@charset \"utf-8\";\n" ++
        "@layer reset, theme, utilities;\n" ++
        "@import \"./reset.css\" layer(reset);\n" ++
        "@import url(\"./theme.css\") layer(theme) supports(color: oklch(0.5 0.2 240));\n" ++
        "@import \"https://cdn.example.com/print.css\" print;\n" ++
        "body { margin: 0; }\n" ++
        "@import \"./late-ignored-1.css\";\n" ++
        ".card { padding: 8px; }\n" ++
        "@import \"./late-ignored-2.css\";\n";

    const list = CssImportRuleList.extractFromStylesheet(css)?;
    expect(list.count() == 3);
    expect(list.ignored_after_rules == 2);

    if (first := list.get(0)) {
        expect(eqBytes(first.url, "./reset.css"));
        expect(eqBytes(first.conditions.layer.name, "reset"));
        expect(first.line == 4);
        expect(first.column == 1);
    } else {
        expect(false);
    }

    if (second := list.get(1)) {
        expect(eqBytes(second.url, "./theme.css"));
        expect(second.conditions.supports.isPresent());
        expect(second.line == 5);
    } else {
        expect(false);
    }

    if (third := list.get(2)) {
        expect(third.isExternalUrl());
        expect(eqBytes(third.conditions.media, "print"));
    } else {
        expect(false);
    }

    // 越界索引返回 null
    expect(list.get(3) == null);
}

test("css import_rule: malformed input and specification order guards") {
    // 非 @import 关键字
    let rejected: usize = 0;

    * = CssImportRule.parse("@media screen {}") catch { rejected += 1; CssImportRule.fromUrl(""); };
    // 空 URL 或未闭合字符串
    * = CssImportRule.parse("@import \"\";") catch { rejected += 1; CssImportRule.fromUrl(""); };
    * = CssImportRule.parse("@import \"unterminated.css;") catch { rejected += 1; CssImportRule.fromUrl(""); };
    * = CssImportRule.parse("@import url();") catch { rejected += 1; CssImportRule.fromUrl(""); };
    * = CssImportRule.parse("@import url(has space.css);") catch { rejected += 1; CssImportRule.fromUrl(""); };
    // 非法 layer 名称（空段、空白、未闭合）
    * = CssImportRule.parse("@import \"a.css\" layer();") catch { rejected += 1; CssImportRule.fromUrl(""); };
    * = CssImportRule.parse("@import \"a.css\" layer(a..b);") catch { rejected += 1; CssImportRule.fromUrl(""); };
    * = CssImportRule.parse("@import \"a.css\" layer(a . b);") catch { rejected += 1; CssImportRule.fromUrl(""); };
    // 非法 supports（空条件、未闭合）
    * = CssImportRule.parse("@import \"a.css\" supports();") catch { rejected += 1; CssImportRule.fromUrl(""); };
    * = CssImportRule.parse("@import \"a.css\" supports((display: grid);") catch { rejected += 1; CssImportRule.fromUrl(""); };
    // 违反规范顺序：supports 之后再出现 layer，或重复 supports
    * = CssImportRule.parse("@import \"a.css\" supports(display: grid) layer(base);") catch { rejected += 1; CssImportRule.fromUrl(""); };
    * = CssImportRule.parse("@import \"a.css\" supports(display: grid) supports(gap: 1px);") catch { rejected += 1; CssImportRule.fromUrl(""); };
    // 非法块规则或分号后尾随垃圾
    * = CssImportRule.parse("@import \"a.css\" { color: red; }") catch { rejected += 1; CssImportRule.fromUrl(""); };
    * = CssImportRule.parse("@import \"a.css\"; extra") catch { rejected += 1; CssImportRule.fromUrl(""); };

    expect(rejected == 14);
}

test("css import_rule: parse -> toCss -> parse round-trip idempotency") {
    const src = "@import url(\"./ui.css\") layer(app.components) supports(not (display: contents)) (min-width: 48rem);";
    const r1 = CssImportRule.parse(src)?;
    let buf1: [256]u8 = [0; 256];
    const n1 = r1.toCss(buf1[0..])?;

    const r2 = CssImportRule.parse(buf1[0..n1])?;
    expect(r1.eql(r2));

    let buf2: [256]u8 = [0; 256];
    const n2 = r2.toCss(buf2[0..])?;
    expect(eqBytes(buf1[0..n1], buf2[0..n2]));
}
