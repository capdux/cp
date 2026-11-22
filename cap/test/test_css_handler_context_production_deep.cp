// cap/test/test_css_handler_context_production_deep.cp
// 深度生产级回归测试：cap:css 属性渐进降级处理上下文 (lib/css/handler_context.cp)

import { test } from "cap:test";
import {
    CSS_CTX_MAX_SUPPORTS,
    CSS_CTX_MAX_UNUSED_SYMBOLS,
    CssDeclarationContext,
    CssPropertyHandlerContext,
} from "cap:css";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

test("handler_context: @supports buckets merge identical conditions and preserve !important") {
    let ctx = CssPropertyHandlerContext.init(CssDeclarationContext.StyleRule);
    * = ctx.addConditionalProperty("(color: oklch(0% 0 0))", "color", "oklch(0.7 0.15 240)")?;
    ctx.is_important = true;
    * = ctx.addConditionalProperty("(color: oklch(0% 0 0))", "border-color", "oklch(0.5 0.1 240)")?;
    ctx.is_important = false;
    * = ctx.addConditionalProperty("(backdrop-filter: blur(1px))", "backdrop-filter", "blur(8px)")?;

    if (ctx.supports_count != 2 || !ctx.hasAdditionalRules()) {
        return err.SupportsBucketCountMismatch;
    }

    let out: [512]u8 = [0; 512];
    const n = ctx.formatSupportsRules(".btn", out[0..], true)?;
    const expected = "@supports (color: oklch(0% 0 0)){.btn{color:oklch(0.7 0.15 240);border-color:oklch(0.5 0.1 240)!important}}@supports (backdrop-filter: blur(1px)){.btn{backdrop-filter:blur(8px)}}";
    if (!bytesEq(out[0..n], expected)) {
        return err.SupportsFormatMismatch;
    }
}

test("handler_context: logical LTR/RTL and dark mode rules format with selector list splitting") {
    let ctx = CssPropertyHandlerContext.init(CssDeclarationContext.StyleRule);
    if (!ctx.shouldCompileLogical()) { return err.StyleRuleShouldCompileLogical; }

    * = ctx.addLogicalRule("padding-left", "12px", "padding-right", "12px")?;
    * = ctx.addDarkRule("background-color", "#18181b")?;

    // 逗号选择器列表（含括号内逗号）应仅在顶层逗号拆分并为每项附加 :dir(...)
    let out: [512]u8 = [0; 512];
    const n = ctx.formatAdditionalRules(".a, :is(.b, .c)", out[0..], true)?;
    const expected = ".a:dir(ltr),:is(.b, .c):dir(ltr){padding-left:12px}.a:dir(rtl),:is(.b, .c):dir(rtl){padding-right:12px}@media (prefers-color-scheme:dark){.a, :is(.b, .c){background-color:#18181b}}";
    if (!bytesEq(out[0..n], expected)) {
        return err.AdditionalRulesFormatMismatch;
    }
}

test("handler_context: StyleAttribute disables logical lowering while StyleRule allows it") {
    let attr_ctx = CssPropertyHandlerContext.init(CssDeclarationContext.StyleAttribute);
    if (attr_ctx.shouldCompileLogical() || CssDeclarationContext.StyleAttribute.allowsAuxiliaryRules()) {
        return err.StyleAttributeLogicalShouldBeDisabled;
    }
    const added = attr_ctx.addLogicalRule("margin-left", "4px", "margin-right", "4px")?;
    if (added || attr_ctx.hasAdditionalRules()) {
        return err.StyleAttributeShouldIgnoreLogicalRule;
    }

    // 显式关闭 compile_logical_enabled 时 StyleRule 也跳过逻辑降级
    let rule_ctx = CssPropertyHandlerContext.init(CssDeclarationContext.StyleRule);
    rule_ctx.compile_logical_enabled = false;
    if (rule_ctx.addLogicalRule("margin-left", "4px", "margin-right", "4px")?) {
        return err.DisabledLogicalFlagShouldSkip;
    }
}

test("handler_context: child() inherits config and unused symbols while isolating rule buckets") {
    let parent = CssPropertyHandlerContext.init(CssDeclarationContext.StyleRule);
    * = parent.markUnusedSymbol("unusedBtn")?;
    * = parent.markUnusedSymbol("deadModal")?;
    // 重复登记幂等返回 false
    if (parent.markUnusedSymbol("unusedBtn")?) { return err.DupUnusedSymShouldBeFalse; }

    * = parent.addDarkRule("color", "#eee")?;

    const child = parent.child(CssDeclarationContext.Keyframes);
    if (child.context != CssDeclarationContext.Keyframes || child.hasAdditionalRules()) {
        return err.ChildShouldStartClean;
    }
    if (!child.isSymbolUnused("unusedBtn") || !child.isSymbolUnused("deadModal") || child.isSymbolUnused("activeBtn")) {
        return err.ChildDidNotInheritUnusedSymbols;
    }
    if (!parent.hasAdditionalRules()) {
        return err.ParentBucketsShouldRemainIntact;
    }
}

test("handler_context: reset() clears temporary buckets while preserving config and unused symbols") {
    let ctx = CssPropertyHandlerContext.init(CssDeclarationContext.StyleRule);
    * = ctx.markUnusedSymbol("legacyClass")?;
    ctx.is_important = true;
    * = ctx.addConditionalProperty("(display: grid)", "display", "grid")?;
    * = ctx.addLogicalRule("left", "0", "right", "0")?;
    * = ctx.addDarkRule("color", "white")?;

    if (!ctx.hasAdditionalRules()) { return err.PreResetShouldHaveRules; }
    ctx.reset();
    if (ctx.hasAdditionalRules() || ctx.is_important) { return err.PostResetShouldBeClean; }
    if (!ctx.isSymbolUnused("legacyClass") || ctx.context != CssDeclarationContext.StyleRule) {
        return err.ResetCorruptedPersistentState;
    }
}

test("handler_context: pretty-printed formatting for supports and additional rules") {
    let ctx = CssPropertyHandlerContext.init(CssDeclarationContext.StyleRule);
    * = ctx.addConditionalProperty("(color: lab(0% 0 0))", "color", "lab(50% 40 30)")?;
    * = ctx.addDarkRule("color", "#fafafa")?;

    let sbuf: [256]u8 = [0; 256];
    const sn = ctx.formatSupportsRules(".box", sbuf[0..], false)?;
    if (!bytesEq(sbuf[0..sn], "@supports (color: lab(0% 0 0)) {\n  .box {color:lab(50% 40 30)}\n}")) {
        return err.PrettySupportsMismatch;
    }

    let abuf: [256]u8 = [0; 256];
    const an = ctx.formatAdditionalRules(".box", abuf[0..], false)?;
    if (!bytesEq(abuf[0..an], "@media (prefers-color-scheme: dark) {\n  .box {color:#fafafa}\n}")) {
        return err.PrettyDarkMismatch;
    }
}

test("handler_context: malformed inputs, capacity limits and tiny buffer guards") {
    let ctx = CssPropertyHandlerContext.init(CssDeclarationContext.StyleRule);

    // 空条件拒绝
    let empty_cond = false;
    ctx.addConditionalProperty("   ", "color", "red") catch {
        empty_cond = true;
    };
    if (!empty_cond) { return err.EmptyConditionNotRejected; }

    // 空属性名或空属性值拒绝
    let bad_decl = false;
    ctx.addDarkRule("", "red") catch {
        bad_decl = true;
    };
    if (!bad_decl) { return err.EmptyPropNotRejected; }

    // 空选择器拒绝
    let empty_sel = false;
    let buf: [64]u8 = [0; 64];
    ctx.formatSupportsRules("  ", buf[0..], true) catch {
        empty_sel = true;
    };
    if (!empty_sel) { return err.EmptySelectorNotRejected; }

    // @supports 桶容量超限拦截
    const conds: [17]([]const u8) = [
        "(a:1)", "(a:2)", "(a:3)", "(a:4)", "(a:5)", "(a:6)", "(a:7)", "(a:8)",
        "(a:9)", "(a:10)", "(a:11)", "(a:12)", "(a:13)", "(a:14)", "(a:15)", "(a:16)",
        "(a:17)",
    ];
    let si: usize = 0;
    while (si < CSS_CTX_MAX_SUPPORTS) {
        * = ctx.addConditionalProperty(conds[si], "color", "red")?;
        si += 1;
    }
    let supp_overflow = false;
    ctx.addConditionalProperty(conds[16], "color", "red") catch {
        supp_overflow = true;
    };
    if (!supp_overflow) { return err.SupportsOverflowNotRejected; }

    // 未使用符号容量超限拦截
    const syms: [33]([]const u8) = [
        "s01", "s02", "s03", "s04", "s05", "s06", "s07", "s08",
        "s09", "s10", "s11", "s12", "s13", "s14", "s15", "s16",
        "s17", "s18", "s19", "s20", "s21", "s22", "s23", "s24",
        "s25", "s26", "s27", "s28", "s29", "s30", "s31", "s32",
        "s33",
    ];
    let ui: usize = 0;
    while (ui < CSS_CTX_MAX_UNUSED_SYMBOLS) {
        * = ctx.markUnusedSymbol(syms[ui])?;
        ui += 1;
    }
    let sym_overflow = false;
    ctx.markUnusedSymbol(syms[32]) catch {
        sym_overflow = true;
    };
    if (!sym_overflow) { return err.UnusedSymOverflowNotRejected; }

    // 输出缓冲区过小拦截 BufferTooSmall
    let tiny: [8]u8 = [0; 8];
    let buf_small = false;
    ctx.formatSupportsRules(".x", tiny[0..], true) catch {
        buf_small = true;
    };
    if (!buf_small) { return err.BufferTooSmallNotRejected; }
}
