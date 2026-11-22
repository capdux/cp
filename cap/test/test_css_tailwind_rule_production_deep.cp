import { test, expect, expect_eq } from "cap:test";
import {
    CSS_TAILWIND_MAX_RULES,
    CSS_TAILWIND_MAX_APPLY_CLASSES,
    CssTailwindStyleName,
    CssTailwindAtRule,
    CssTailwindApplyRule,
    CssTailwindDirectiveList,
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

test("css tailwind_rule: parse all 4 @tailwind layers & serialize canonically", fn() {
    const r_base = CssTailwindAtRule.parse("/* reset */ @TAILWIND base;") catch {
        expect(false);
        return;
    };
    expect(r_base.style_name == CssTailwindStyleName.Base);
    expect_eq(r_base.cascadeOrder(), 0);
    expect(r_base.eql(CssTailwindAtRule.of(CssTailwindStyleName.Base)));

    const r_comp = CssTailwindAtRule.parse("@tailwind components;") catch {
        expect(false);
        return;
    };
    expect(r_comp.style_name == CssTailwindStyleName.Components);
    expect_eq(r_comp.cascadeOrder(), 1);

    const r_util = CssTailwindAtRule.parse("@tailwind utilities;") catch {
        expect(false);
        return;
    };
    expect(r_util.style_name == CssTailwindStyleName.Utilities);
    expect_eq(r_util.cascadeOrder(), 2);

    const r_var = CssTailwindAtRule.parse("@tailwind variants;") catch {
        expect(false);
        return;
    };
    expect(r_var.style_name == CssTailwindStyleName.Variants);
    expect_eq(r_var.cascadeOrder(), 3);

    let out: [64]u8 = [0; 64];
    const n = r_util.toCss(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n], "@tailwind utilities;"));
});

test("css tailwind_rule: parse @apply with arbitrary values and !important", fn() {
    const rule = CssTailwindApplyRule.parse(
        "@apply flex items-center bg-[#1da1f2] grid-cols-[1fr_minmax(0,2fr)] hover:text-white !important;",
    ) catch {
        expect(false);
        return;
    };
    expect_eq(rule.count(), 5);
    expect(rule.important);
    expect(rule.contains("flex"));
    expect(rule.contains("bg-[#1da1f2]"));
    expect(rule.contains("grid-cols-[1fr_minmax(0,2fr)]"));
    expect(!rule.contains("hidden"));

    if (c3 := rule.at(3)) {
        expect(strEq(c3, "grid-cols-[1fr_minmax(0,2fr)]"));
    } else {
        expect(false);
    }
    expect(rule.at(5) == null);

    let out: [256]u8 = [0; 256];
    const n = rule.toCss(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(
        out[0..n],
        "@apply flex items-center bg-[#1da1f2] grid-cols-[1fr_minmax(0,2fr)] hover:text-white !important;",
    ));
});

test("css tailwind_rule: stylesheet extraction, line/col tracking & canonical order check", fn() {
    const sheet =
        "/* @tailwind variants; */\n" ++
        "@tailwind base;\n" ++
        ".note { content: \"@tailwind utilities;\"; }\n" ++
        "  @tailwind components;\n" ++
        "@tailwind utilities;\n";

    const list = CssTailwindDirectiveList.extractFromStylesheet(sheet) catch {
        expect(false);
        return;
    };
    expect_eq(list.count(), 3);
    expect(list.hasLayer(CssTailwindStyleName.Base));
    expect(list.hasLayer(CssTailwindStyleName.Components));
    expect(list.hasLayer(CssTailwindStyleName.Utilities));
    expect(!list.hasLayer(CssTailwindStyleName.Variants));
    expect(!list.hasDuplicateLayers());
    expect(list.isCanonicalOrder());

    if (r0 := list.at(0)) {
        expect_eq(r0.line, 2);
        expect_eq(r0.column, 1);
    } else {
        expect(false);
    }
    if (r1 := list.at(1)) {
        expect_eq(r1.line, 4);
        expect_eq(r1.column, 3);
    } else {
        expect(false);
    }
});

test("css tailwind_rule: detect out-of-order and duplicate @tailwind layers", fn() {
    const bad_order = CssTailwindDirectiveList.extractFromStylesheet(
        "@tailwind utilities;\n@tailwind base;\n",
    ) catch {
        expect(false);
        return;
    };
    expect_eq(bad_order.count(), 2);
    expect(!bad_order.hasDuplicateLayers());
    expect(!bad_order.isCanonicalOrder());

    const dup = CssTailwindDirectiveList.extractFromStylesheet(
        "@tailwind base;\n@tailwind utilities;\n@tailwind base;\n",
    ) catch {
        expect(false);
        return;
    };
    expect_eq(dup.count(), 3);
    expect(dup.hasDuplicateLayers());
    expect(!dup.isCanonicalOrder());
});

test("css tailwind_rule: malformed @tailwind and @apply error guards", fn() {
    // 未知层名
    let ok1 = true;
    * = CssTailwindAtRule.parse("@tailwind screens;") catch {
        ok1 = false;
    };
    expect(!ok1);

    // 缺少分号
    let ok2 = true;
    * = CssTailwindAtRule.parse("@tailwind base") catch {
        ok2 = false;
    };
    expect(!ok2);

    // 非法块语法
    let ok3 = true;
    * = CssTailwindAtRule.parse("@tailwind utilities { a: 1; }") catch {
        ok3 = false;
    };
    expect(!ok3);

    // 空 @apply
    let ok4 = true;
    * = CssTailwindApplyRule.parse("@apply ;") catch {
        ok4 = false;
    };
    expect(!ok4);

    // !important 不在末尾
    let ok5 = true;
    * = CssTailwindApplyRule.parse("@apply !important flex;") catch {
        ok5 = false;
    };
    expect(!ok5);

    // 方括号未闭合
    let ok6 = true;
    * = CssTailwindApplyRule.parse("@apply bg-[#fff;") catch {
        ok6 = false;
    };
    expect(!ok6);
});

test("css tailwind_rule: buffer-too-small and capacity constants verification", fn() {
    expect_eq(CSS_TAILWIND_MAX_RULES, 32);
    expect_eq(CSS_TAILWIND_MAX_APPLY_CLASSES, 32);

    const r = CssTailwindAtRule.of(CssTailwindStyleName.Components);
    let tiny: [6]u8 = [0; 6];
    let ok_buf = true;
    * = r.toCss(tiny[0..]) catch {
        ok_buf = false;
    };
    expect(!ok_buf);

    const ap = CssTailwindApplyRule.parse("@apply flex gap-2;") catch {
        expect(false);
        return;
    };
    let ok_ap = true;
    * = ap.toCss(tiny[0..]) catch {
        ok_ap = false;
    };
    expect(!ok_ap);
});
