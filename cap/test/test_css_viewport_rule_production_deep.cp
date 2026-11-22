import { test, expect, expect_eq } from "cap:test";
import {
    CSS_VIEWPORT_MAX_DECLS,
    CSS_VIEWPORT_MAX_RULES,
    CssViewportVendorPrefix,
    CssViewportDescriptorKind,
    CssViewportUserZoom,
    CssViewportOrientation,
    CssViewportFit,
    CssViewportMinMaxPair,
    CssViewportDeclaration,
    CssViewportRule,
    CssViewportRuleList,
} from "cap:css";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("css viewport_rule: parse standard @viewport with comments and typed descriptors", fn() {
    const input =
        "/* mobile viewport */ @viewport { width: device-width; zoom: 1.0; min-zoom: 0.5; max-zoom: 200%; user-zoom: zoom; orientation: portrait; viewport-fit: cover; }";
    const rule = CssViewportRule.parse(input) catch {
        expect(false);
        return;
    };

    expect(rule.vendor_prefix == CssViewportVendorPrefix.None);
    expect(!rule.vendor_prefix.isPrefixed());
    expect_eq(rule.count(), 7);

    if (ori := rule.effectiveOrientation()) {
        expect(ori == CssViewportOrientation.Portrait);
        expect(strEq(ori.asStr(), "portrait"));
    } else {
        expect(false);
    }

    if (uz := rule.effectiveUserZoom()) {
        expect(uz == CssViewportUserZoom.Zoom);
        expect(strEq(uz.asStr(), "zoom"));
    } else {
        expect(false);
    }

    if (vf := rule.effectiveViewportFit()) {
        expect(vf == CssViewportFit.Cover);
        expect(strEq(vf.asStr(), "cover"));
    } else {
        expect(false);
    }

    let out: [256]u8 = [0; 256];
    const n = rule.toCssMinified(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(
        out[0..n],
        "@viewport{width:device-width;zoom:1.0;min-zoom:0.5;max-zoom:200%;user-zoom:zoom;orientation:portrait;viewport-fit:cover}",
    ));
});

test("css viewport_rule: vendor prefixes, canonical formatting & multi-prefix emitter", fn() {
    const rule = CssViewportRule.parse(
        "@-MS-VIEWPORT { WIDTH: 320px; orientation: LANDSCAPE !important; }",
    ) catch {
        expect(false);
        return;
    };

    expect(rule.vendor_prefix == CssViewportVendorPrefix.Ms);
    expect(rule.vendor_prefix.isPrefixed());
    expect(strEq(rule.vendor_prefix.asStr(), "-ms-"));
    expect(strEq(rule.vendor_prefix.atRuleName(), "@-ms-viewport"));

    let pretty: [256]u8 = [0; 256];
    const pn = rule.toCss(pretty[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(
        pretty[0..pn],
        "@-ms-viewport {\n  width: 320px;\n  orientation: LANDSCAPE !important;\n}",
    ));

    const prefixes = [CssViewportVendorPrefix.Ms, CssViewportVendorPrefix.WebKit, CssViewportVendorPrefix.None];
    let multi: [512]u8 = [0; 512];
    const mn = rule.toCssWithPrefixes(multi[0..], prefixes[0..], true) catch {
        expect(false);
        return;
    };
    expect(strEq(
        multi[0..mn],
        "@-ms-viewport{width:320px;orientation:LANDSCAPE!important}@-webkit-viewport{width:320px;orientation:LANDSCAPE!important}@viewport{width:320px;orientation:LANDSCAPE!important}",
    ));
});

test("css viewport_rule: width/height shorthand expansion including calc() safety", fn() {
    const r1 = CssViewportRule.parse("@viewport { width: 320px 640px; height: device-height; }") catch {
        expect(false);
        return;
    };
    if (wp := r1.expandWidthShorthand()) {
        expect(strEq(wp.min_value, "320px"));
        expect(strEq(wp.max_value, "640px"));
    } else {
        expect(false);
    }
    if (hp := r1.expandHeightShorthand()) {
        expect(strEq(hp.min_value, "device-height"));
        expect(strEq(hp.max_value, "device-height"));
    } else {
        expect(false);
    }

    const r2 = CssViewportRule.parse("@viewport { width: calc(100% - 20px) extend-to-zoom; }") catch {
        expect(false);
        return;
    };
    if (wp2 := r2.expandWidthShorthand()) {
        expect(strEq(wp2.min_value, "calc(100% - 20px)"));
        expect(strEq(wp2.max_value, "extend-to-zoom"));
    } else {
        expect(false);
    }

    // 超过 2 个顶层值属于非法简写
    expect(CssViewportMinMaxPair.parseShorthand("100px 200px 300px") == null);
    expect(CssViewportMinMaxPair.parseShorthand("") == null);
});

test("css viewport_rule: cascade-safe deduplication and !important precedence", fn() {
    let rule = CssViewportRule.parse(
        "@viewport { width: 320px; zoom: 1.0 !important; width: device-width; zoom: 2.0; user-zoom: fixed; }",
    ) catch {
        expect(false);
        return;
    };

    // !important 优先于后续普通声明
    if (z := rule.find("zoom")) {
        expect(strEq(z.value, "1.0"));
        expect(z.important);
    } else {
        expect(false);
    }

    // 普通同级声明后者胜出
    if (w := rule.find("width")) {
        expect(strEq(w.value, "device-width"));
        expect(!w.important);
    } else {
        expect(false);
    }

    const removed = rule.deduplicate();
    expect_eq(removed, 2);
    expect_eq(rule.count(), 3);

    let out: [256]u8 = [0; 256];
    const n = rule.toCssMinified(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n], "@viewport{zoom:1.0!important;width:device-width;user-zoom:fixed}"));
});

test("css viewport_rule: extract multiple rules from full stylesheet safely", fn() {
    const sheet =
        "body::after { content: \"@viewport { width: 1px; }\"; } /* @viewport { zoom: 9; } */\n" ++
        "@-ms-viewport { width: device-width; }\n" ++
        ".card { color: red; }\n" ++
        "@viewport { width: device-width; viewport-fit: contain; }\n" ++
        "@-o-viewport { orientation: auto; }\n";

    const list = CssViewportRuleList.extractFromStylesheet(sheet) catch {
        expect(false);
        return;
    };
    expect_eq(list.count(), 3);

    if (r0 := list.at(0)) {
        expect(r0.vendor_prefix == CssViewportVendorPrefix.Ms);
        expect_eq(r0.count(), 1);
    } else {
        expect(false);
    }

    if (r1 := list.at(1)) {
        expect(r1.vendor_prefix == CssViewportVendorPrefix.None);
        expect_eq(r1.count(), 2);
        if (vf := r1.effectiveViewportFit()) {
            expect(vf == CssViewportFit.Contain);
        } else {
            expect(false);
        }
    } else {
        expect(false);
    }

    if (r2 := list.at(2)) {
        expect(r2.vendor_prefix == CssViewportVendorPrefix.O);
        expect_eq(r2.count(), 1);
    } else {
        expect(false);
    }
    expect(list.at(3) == null);
});

test("css viewport_rule: malformed input, unknown prefix & invalid descriptor value guards", fn() {
    // 非 @viewport 规则
    let ok1 = true;
    * = CssViewportRule.parse("@media screen { width: 100px; }") catch {
        ok1 = false;
    };
    expect(!ok1);

    // 未知供应商前缀
    let ok2 = true;
    * = CssViewportRule.parse("@-ie-viewport { width: device-width; }") catch {
        ok2 = false;
    };
    expect(!ok2);

    // 非法 prelude token
    let ok3 = true;
    * = CssViewportRule.parse("@viewport screen { width: device-width; }") catch {
        ok3 = false;
    };
    expect(!ok3);

    // 未闭合块
    let ok4 = true;
    * = CssViewportRule.parse("@viewport { width: device-width;") catch {
        ok4 = false;
    };
    expect(!ok4);

    // 非法 user-zoom / orientation / viewport-fit / 负 zoom
    let ok5 = true;
    * = CssViewportRule.parse("@viewport { user-zoom: disabled; }") catch {
        ok5 = false;
    };
    expect(!ok5);

    let ok6 = true;
    * = CssViewportRule.parse("@viewport { orientation: diagonal; }") catch {
        ok6 = false;
    };
    expect(!ok6);

    let ok7 = true;
    * = CssViewportRule.parse("@viewport { viewport-fit: stretch; }") catch {
        ok7 = false;
    };
    expect(!ok7);

    let ok8 = true;
    * = CssViewportRule.parse("@viewport { zoom: -1.5; }") catch {
        ok8 = false;
    };
    expect(!ok8);
});

test("css viewport_rule: empty block, capacity bounds, equality & buffer-too-small guard", fn() {
    const empty_rule = CssViewportRule.parse("@viewport { }") catch {
        expect(false);
        return;
    };
    expect(empty_rule.isEmpty());
    expect_eq(empty_rule.count(), 0);
    expect(empty_rule.at(0) == null);

    let out: [32]u8 = [0; 32];
    const n = empty_rule.toCss(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n], "@viewport {}"));

    const mn = empty_rule.toCssMinified(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..mn], "@viewport{}"));

    // 缓冲区不足防护
    let tiny: [4]u8 = [0; 4];
    let buf_ok = true;
    * = empty_rule.toCssMinified(tiny[0..]) catch {
        buf_ok = false;
    };
    expect(!buf_ok);

    // 容量上限防护 (CSS_VIEWPORT_MAX_DECLS = 32)
    let full = CssViewportRule.init(CssViewportVendorPrefix.None);
    let i: usize = 0;
    while (i < CSS_VIEWPORT_MAX_DECLS) {
        * = full.push(CssViewportDeclaration{
            name: "width",
            value: "device-width",
            kind: CssViewportDescriptorKind.Width,
            important: false,
        }) catch {
            expect(false);
            return;
        };
        i += 1;
    }
    let cap_ok = true;
    * = full.push(CssViewportDeclaration{
        name: "zoom",
        value: "1",
        kind: CssViewportDescriptorKind.Zoom,
        important: false,
    }) catch {
        cap_ok = false;
    };
    expect(!cap_ok);

    // 等价比较与常量校验
    expect_eq(CSS_VIEWPORT_MAX_DECLS, 32);
    expect_eq(CSS_VIEWPORT_MAX_RULES, 16);
    const eq_a = CssViewportRule.parse("@viewport { width: 320px; }") catch {
        expect(false);
        return;
    };
    const eq_b = CssViewportRule.parse("@VIEWPORT { WIDTH: 320px }") catch {
        expect(false);
        return;
    };
    expect(eq_a.eql(eq_b));
});
