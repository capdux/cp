// cap/test/test_bundler_css_import_order_production_deep.cp
// 深度生产级回归测试：cap:bundler CSS @import 依赖排序与 Last-Wins 层叠去重引擎 (lib/bundler/css_import_order.cp)

import { test } from "cap:test";
import {
    CSS_IMPORT_MAX_RULES_PER_FILE,
    CssImportCondition,
    CssConditionStack,
    CssImportEntryKind,
    CssImportGraph,
} from "cap:bundler";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("css_import_order: diamond dependency last-wins deduplication without @layer") {
    // 菱形依赖：A(1) -> B(2), C(3); B(2) -> D(4); C(3) -> D(4)
    // 原始后序：D, B, D, C, A；因 D 无 @layer，首次 D 被剔除 -> B, D, C, A
    let g = CssImportGraph.init();
    const s_a = g.addFile(1, false)?;
    const s_b = g.addFile(2, false)?;
    const s_c = g.addFile(3, false)?;
    * = g.addFile(4, false)?;

    const none = CssImportCondition{};
    g.addImport(s_a, 2, none)?;
    g.addImport(s_a, 3, none)?;
    g.addImport(s_b, 4, none)?;
    g.addImport(s_c, 4, none)?;

    const entries: [1]u32 = [1];
    const res = g.computeCssOrder(entries[0..])?;
    const list = res.entriesSlice();
    if (list.len != 4) return err.DiamondLenMismatch;
    if (list[0].source_index != 2 || list[1].source_index != 4 ||
        list[2].source_index != 3 || list[3].source_index != 1)
    {
        return err.DiamondOrderMismatch;
    }
}

test("css_import_order: @layer declaration preserved as LayersOnly at first occurrence") {
    // 同菱形结构，但 D(4) 声明了 @layer：
    // 首次 D 降级为 LayersOnly，末次 D 保留为 SourceFile -> D(LayersOnly), B, D(SourceFile), C, A
    let g = CssImportGraph.init();
    const s_a = g.addFile(1, false)?;
    const s_b = g.addFile(2, false)?;
    const s_c = g.addFile(3, false)?;
    * = g.addFile(4, true)?; // has_at_layer = true

    const none = CssImportCondition{};
    g.addImport(s_a, 2, none)?;
    g.addImport(s_a, 3, none)?;
    g.addImport(s_b, 4, none)?;
    g.addImport(s_c, 4, none)?;

    const entries: [1]u32 = [1];
    const res = g.computeCssOrder(entries[0..])?;
    const list = res.entriesSlice();
    if (list.len != 5) return err.LayerLenMismatch;
    if (list[0].kind != .LayersOnly || list[0].source_index != 4) return err.ExpectedFirstLayersOnly;
    if (list[1].kind != .SourceFile || list[1].source_index != 2) return err.ExpectedB;
    if (list[2].kind != .SourceFile || list[2].source_index != 4) return err.ExpectedLastD;
    if (list[3].kind != .SourceFile || list[3].source_index != 3) return err.ExpectedC;
    if (list[4].kind != .SourceFile || list[4].source_index != 1) return err.ExpectedA;
}

test("css_import_order: external @import hoisting & external URL deduplication") {
    let g = CssImportGraph.init();
    const s_a = g.addFile(1, false)?;
    const s_b = g.addFile(2, false)?;
    const none = CssImportCondition{};

    // A 导入 B 和重复外链，B 导入同一外链与另一外链
    g.addImport(s_a, 2, none)?;
    g.addExternalImport(s_a, "https://cdn.example.com/fonts.css", none)?;
    g.addExternalImport(s_b, "https://cdn.example.com/fonts.css", none)?;
    g.addExternalImport(s_b, "https://cdn.example.com/reset.css", none)?;

    const entries: [1]u32 = [1];
    const res = g.computeCssOrder(entries[0..])?;
    if (!res.has_external_import) return err.ExpectedExternalFlag;

    const list = res.entriesSlice();
    // 外链去重后共 2 条（reset.css, fonts.css）且全部顶置在内部文件 B(2), A(1) 之前
    if (list.len != 4) return err.HoistedLenMismatch;
    if (list[0].kind != .ExternalUrl || !bytesEq(list[0].external_url, "https://cdn.example.com/reset.css")) {
        return err.External0Mismatch;
    }
    if (list[1].kind != .ExternalUrl || !bytesEq(list[1].external_url, "https://cdn.example.com/fonts.css")) {
        return err.External1Mismatch;
    }
    if (list[2].kind != .SourceFile || list[2].source_index != 2) return err.InternalBMismatch;
    if (list[3].kind != .SourceFile || list[3].source_index != 1) return err.InternalAMismatch;
}

test("css_import_order: conditional @import nesting propagation & redundancy rules") {
    let g = CssImportGraph.init();
    const s_a = g.addFile(1, false)?;
    const s_b = g.addFile(2, false)?;
    * = g.addFile(3, false)?;

    const cond_screen = CssImportCondition{ media: "screen" };
    const cond_print = CssImportCondition{ media: "print" };
    const cond_layer = CssImportCondition{ layer: "theme", supports: "display: grid" };

    // A 在 media="screen" 下导入 B；B 在 layer="theme" supports="display: grid" 下导入 C(3)
    // A 又在 media="print" 下导入 C(3) -> 两个 C(3) 条件互异，不能互相覆盖！
    g.addImport(s_a, 2, cond_screen)?;
    g.addImport(s_b, 3, cond_layer)?;
    g.addImport(s_a, 3, cond_print)?;

    const entries: [1]u32 = [1];
    const res = g.computeCssOrder(entries[0..])?;
    const list = res.entriesSlice();
    if (list.len != 4) return err.ConditionalDistinctCountMismatch;

    // list[0] 是 B->C(3)，应携带 2 层条件栈：[media="screen", layer="theme" + supports="display: grid"]
    if (list[0].source_index != 3 || list[0].conditions.count != 2) return err.NestedCondDepthMismatch;
    if (!bytesEq(list[0].conditions.slice()[0].media, "screen")) return err.Cond0Mismatch;
    if (!bytesEq(list[0].conditions.slice()[1].layer, "theme")) return err.Cond1LayerMismatch;
    if (!bytesEq(list[0].conditions.slice()[1].supports, "display: grid")) return err.Cond1SupportsMismatch;

    // list[2] 是 A->C(3) with media="print"，两者均保留
    if (list[2].source_index != 3 || list[2].conditions.count != 1 ||
        !bytesEq(list[2].conditions.slice()[0].media, "print"))
    {
        return err.PrintCondMismatch;
    }

    // 校验无条件后继覆盖有条件前驱
    let s_cond = CssConditionStack{};
    s_cond.push(cond_screen)?;
    const s_uncond = CssConditionStack{};
    if (!s_cond.isRedundantBefore(s_uncond)) return err.UncondShouldOverrideCond;
    if (s_uncond.isRedundantBefore(s_cond)) return err.CondShouldNotOverrideUncond;
}

test("css_import_order: WebKit parent-chain cycle detection & CSS Modules composes") {
    // 环路 A(1) -> B(2) -> A(1)，且 A 通过 composes 引用 Base(3)
    let g = CssImportGraph.init();
    const s_a = g.addFile(1, false)?;
    const s_b = g.addFile(2, false)?;
    * = g.addFile(3, false)?;
    const none = CssImportCondition{};

    g.addImport(s_a, 2, none)?;
    g.addImport(s_b, 1, none)?; // cycle back to A(1) -> safely bailed!
    g.addComposes(s_a, 3)?;

    const entries: [1]u32 = [1];
    const res = g.computeCssOrder(entries[0..])?;
    const list = res.entriesSlice();
    // 输出应为 B(2) -> Base(3) -> A(1)，无死循环
    if (list.len != 3) return err.CycleLenMismatch;
    if (list[0].source_index != 2 || list[1].source_index != 3 || list[2].source_index != 1) {
        return err.CycleOrderMismatch;
    }
}

test("css_import_order: multi-entrypoint cross-bundle deduplication round-trip") {
    // 两个入口 E1(10), E2(20) 共享工具样式 Shared(30)
    let g = CssImportGraph.init();
    const s_e1 = g.addFile(10, false)?;
    const s_e2 = g.addFile(20, false)?;
    * = g.addFile(30, false)?;
    const none = CssImportCondition{};

    g.addImport(s_e1, 30, none)?;
    g.addImport(s_e2, 30, none)?;

    const entries: [2]u32 = [10, 20];
    const res = g.computeCssOrder(entries[0..])?;
    const list = res.entriesSlice();
    // 原始: 30, 10, 30, 20 -> 去重后: 10, 30, 20
    if (list.len != 3) return err.MultiEntryLenMismatch;
    if (list[0].source_index != 10 || list[1].source_index != 30 || list[2].source_index != 20) {
        return err.MultiEntryOrderMismatch;
    }
}

test("css_import_order: malformed inputs & capacity boundary guards") {
    let g = CssImportGraph.init();
    const s0 = g.addFile(1, false)?;

    // 1. 重复文件注册拒绝
    let caught_dup = false;
    * = g.addFile(1, false) catch {
        caught_dup = true;
    };
    if (!caught_dup) return err.ExpectedDuplicateFile;

    // 2. 超出单文件最大规则数拒绝
    const none = CssImportCondition{};
    let r: usize = 0;
    while (r < CSS_IMPORT_MAX_RULES_PER_FILE) {
        g.addExternalImport(s0, "https://example.com/a.css", none)?;
        r += 1;
    }
    let caught_rules = false;
    g.addExternalImport(s0, "https://example.com/overflow.css", none) catch {
        caught_rules = true;
    };
    if (!caught_rules) return err.ExpectedTooManyRules;

    // 3. 非法入口源索引拒绝
    const bad_entries: [1]u32 = [999];
    let caught_bad = false;
    * = g.computeCssOrder(bad_entries[0..]) catch {
        caught_bad = true;
    };
    if (!caught_bad) return err.ExpectedInvalidSourceIndex;

    // 4. 空入口列表安全返回空结果
    const empty_entries: [0]u32 = [];
    const empty_res = g.computeCssOrder(empty_entries[0..])?;
    if (empty_res.entriesSlice().len != 0 || empty_res.has_external_import) {
        return err.ExpectedEmptyResult;
    }
}
