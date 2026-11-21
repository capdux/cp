import { test, expect } from "cap:test";
import {
    ImportLayerKind,
    ImportRule,
    ScopePrelude,
    LayerStatement,
} from "cap:css";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("css_cascade_rules: @import with layer, supports, and media", () => {
    let buf: [256]u8 = [0; 256];

    const r1 = ImportRule.parse("@import url(\"theme.css\") layer(framework.base) supports(display: grid) screen and (min-width: 600px);");
    expect(r1 != null);
    expect(r1.?.hasConditions());
    expect(r1.?.layer_kind == ImportLayerKind.named);
    expect(strEq(r1.?.layer_name, "framework.base"));
    expect(strEq(r1.?.supports, "display: grid"));
    expect(strEq(r1.?.media, "screen and (min-width: 600px)"));
    expect(strEq(
        r1.?.toCss(buf[0..]),
        "@import \"theme.css\" layer(framework.base) supports(display: grid) screen and (min-width: 600px);",
    ));

    const r2 = ImportRule.parse("@import 'reset.css' layer;");
    expect(r2 != null && r2.?.layer_kind == ImportLayerKind.anonymous);
    expect(strEq(r2.?.toCss(buf[0..]), "@import \"reset.css\" layer;"));

    expect(ImportRule.parse("@import ;") == null);
});

test("css_cascade_rules: @scope prelude and @layer statement", () => {
    let buf: [128]u8 = [0; 128];

    const sc1 = ScopePrelude.parse("@scope (.card) to (.card-content)");
    expect(sc1 != null && !sc1.?.isImplicit() && sc1.?.hasLimit());
    expect(strEq(sc1.?.toCss(buf[0..]), "@scope (.card) to (.card-content)"));

    const sc2 = ScopePrelude.parse("@scope to (.nested)");
    expect(sc2 != null && sc2.?.isImplicit() && sc2.?.hasLimit());
    expect(strEq(sc2.?.toCss(buf[0..]), "@scope to (.nested)"));

    expect(ScopePrelude.parse("@scope (.card) from (.x)") == null);

    const ly = LayerStatement.parse("@layer reset, theme.dark, utilities;");
    expect(ly != null && ly.?.len == 3);
    expect(strEq(ly.?.toCss(buf[0..]), "@layer reset, theme.dark, utilities;"));

    expect(LayerStatement.parse("@layer bad..name;") == null);
    expect(LayerStatement.parse("@layer ;") == null);
});
