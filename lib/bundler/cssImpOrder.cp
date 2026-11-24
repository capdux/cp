// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/findImportedFilesInCSSOrder.cp | Synced: 2026-11-23T14:41:08Z
// Ported from Bun: /data/dev/bun/src/bundler/linker_context/findImportedFilesInCSSOrder.cp & /data/dev/bun/docs/bundler/css.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation CSS @import Dependency Ordering & Cascade Deduplication Engine:
// - Evaluates CSS @import graphs in DFS postorder with WebKit parent-chain cycle detection
// - Propagates nested conditional @import stacks (layer / supports / media) down imported subtrees
// - Hoists external @import URLs to the top of the bundle to preserve valid CSS grammar
// - Performs reverse last-wins deduplication while preserving @layer declaration order via LayersOnly placeholders
// - Traverses CSS Modules `composes` edges alongside standard @import rules.

/// 单图支持的最大 CSS 文件数量
pub const CSS_IMPORT_MAX_FILES: usize = 64;

/// 单文件支持的最大 `@import` / `composes` 规则数量
pub const CSS_IMPORT_MAX_RULES_PER_FILE: usize = 16;

/// 条件 `@import` 嵌套栈最大深度
pub const CSS_IMPORT_MAX_CONDITIONS: usize = 8;

/// 线性化结果支持的最大条目数
pub const CSS_IMPORT_MAX_ORDER_ENTRIES: usize = 256;

/// 父链递归访问最大深度
pub const CSS_IMPORT_MAX_DEPTH: usize = 32;

/// CSS `@import` 拓扑排序错误集
pub err CssImportOrderError {
    TooManyFiles,
    TooManyRules,
    TooManyConditions,
    TooManyEntries,
    InvalidSourceIndex,
    DuplicateFile,
    MaxDepthExceeded,
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// 单层 `@import` 包裹条件（`@import url(...) layer(...) supports(...) <media>;`）
pub struct CssImportCondition {
    layer: []const u8 = "",
    supports: []const u8 = "",
    media: []const u8 = "",

    fn isEmpty(): bool {
        return _.layer.len == 0 && _.supports.len == 0 && _.media.len == 0;
    }

    fn eql(other: CssImportCondition): bool {
        return strEq(_.layer, other.layer) &&
            strEq(_.supports, other.supports) &&
            strEq(_.media, other.media);
    }
}

/// 嵌套 `@import` 条件栈（零堆分配定长数组）
pub struct CssConditionStack {
    items: [CSS_IMPORT_MAX_CONDITIONS]CssImportCondition = [CssImportCondition{}; CSS_IMPORT_MAX_CONDITIONS],
    count: usize = 0,

    fn slice(): []const CssImportCondition {
        return _.items[0.._.count];
    }

    fn push(*_, cond: CssImportCondition): ! {
        if (cond.isEmpty()) return;
        if (_.count >= CSS_IMPORT_MAX_CONDITIONS) {
            return err.TooManyConditions;
        }
        _.items[_.count] = cond;
        _.count += 1;
    }

    /// 返回追加新条件后的不可变副本（用于 DFS 嵌套条件分支）
    fn withPushed(cond: CssImportCondition): CssConditionStack! {
        let out = CssConditionStack{
            items: _.items,
            count: _.count,
        };
        if (cond.isEmpty()) return out;
        if (out.count >= CSS_IMPORT_MAX_CONDITIONS) {
            return err.TooManyConditions;
        }
        out.items[out.count] = cond;
        out.count += 1;
        return out;
    }

    fn eql(other: CssConditionStack): bool {
        if (_.count != other.count) return false;
        let i: usize = 0;
        while (i < _.count) {
            if (!_.items[i].eql(other.items[i])) return false;
            i += 1;
        }
        return true;
    }

    /// 判断当前前驱条件栈在后继 `later` 出现时是否冗余（对齐 Bun `isConditionalImportRedundant`）：
    /// - 若后继为无条件导入（`later.count == 0`），则无条件覆盖任意前驱；
    /// - 若两者条件栈完全一致，则前驱冗余；
    /// - 若后继带有不同条件（如仅 `@media print`），则不可覆盖前驱。
    fn isRedundantBefore(later: CssConditionStack): bool {
        if (later.count == 0) return true;
        return _.eql(later);
    }
}

/// CSS 依赖边类型
pub enum CssImportRuleKind: u8 {
    Internal = 0,
    External = 1,
    Composes = 2,
}

/// 单条 CSS 导入/组合规则
pub struct CssImportRule {
    kind: CssImportRuleKind = .Internal,
    target_source_index: u32 = 0,
    external_url: []const u8 = "",
    condition: CssImportCondition = CssImportCondition{},
}

/// CSS 模块文件节点
pub struct CssImportFile {
    source_index: u32 = 0,
    /// 该文件是否包含顶层 `@layer` 声明（若被后继覆盖，需保留 `LayersOnly` 占位以维护层叠序）
    has_at_layer: bool = false,
    rules: [CSS_IMPORT_MAX_RULES_PER_FILE]CssImportRule = [CssImportRule{}; CSS_IMPORT_MAX_RULES_PER_FILE],
    rule_count: usize = 0,
}

/// 最终线性化输出条目类别
pub enum CssImportEntryKind: u8 {
    /// 正常输出该内部 CSS 文件
    SourceFile = 0,
    /// 顶置的外部 `@import url(...)` 语句
    ExternalUrl = 1,
    /// 已被更靠后的同文件声明覆盖，但仅保留其 `@layer` 声明以维持层叠优先级
    LayersOnly = 2,
}

/// 线性化后的单条 CSS 输出记录
pub struct CssImportOrderEntry {
    kind: CssImportEntryKind = .SourceFile,
    source_index: u32 = 0,
    external_url: []const u8 = "",
    conditions: CssConditionStack = CssConditionStack{},
}

/// CSS `@import` 排序与去重最终结果（零动态堆分配）
pub struct CssImportOrderResult {
    entries: [CSS_IMPORT_MAX_ORDER_ENTRIES]CssImportOrderEntry = [CssImportOrderEntry{}; CSS_IMPORT_MAX_ORDER_ENTRIES],
    count: usize = 0,
    has_external_import: bool = false,

    fn entriesSlice(): []const CssImportOrderEntry {
        return _.entries[0.._.count];
    }
}

/// 纯 CP 零堆分配 CSS `@import` 依赖图与层叠顺序解析器
pub struct CssImportGraph {
    files: [CSS_IMPORT_MAX_FILES]CssImportFile = [CssImportFile{}; CSS_IMPORT_MAX_FILES],
    file_count: usize = 0,

    static fn init(): CssImportGraph {
        return CssImportGraph{};
    }

    fn findSlot(source_index: u32): usize? {
        let i: usize = 0;
        while (i < _.file_count) {
            if (_.files[i].source_index == source_index) return i;
            i += 1;
        }
        return null;
    }

    /// 注册一个 CSS 文件节点，返回其内部槽位索引
    fn addFile(*_, source_index: u32, has_at_layer: bool): usize! {
        if (_.findSlot(source_index) != null) {
            return err.DuplicateFile;
        }
        if (_.file_count >= CSS_IMPORT_MAX_FILES) {
            return err.TooManyFiles;
        }
        const slot = _.file_count;
        _.files[slot] = CssImportFile{
            source_index: source_index,
            has_at_layer: has_at_layer,
            rule_count: 0,
        };
        _.file_count += 1;
        return slot;
    }

    /// 追加内部 `@import` 依赖边（可选携带 `layer`/`supports`/`media` 条件）
    fn addImport(*_, slot: usize, target_source_index: u32, condition: CssImportCondition): ! {
        if (slot >= _.file_count) return err.InvalidSourceIndex;
        const rc = _.files[slot].rule_count;
        if (rc >= CSS_IMPORT_MAX_RULES_PER_FILE) return err.TooManyRules;
        _.files[slot].rules[rc] = CssImportRule{
            kind: .Internal,
            target_source_index: target_source_index,
            external_url: "",
            condition: condition,
        };
        _.files[slot].rule_count = rc + 1;
    }

    /// 追加外部 `@import` URL（将被顶置至最终产物头部）
    fn addExternalImport(*_, slot: usize, url: []const u8, condition: CssImportCondition): ! {
        if (slot >= _.file_count) return err.InvalidSourceIndex;
        const rc = _.files[slot].rule_count;
        if (rc >= CSS_IMPORT_MAX_RULES_PER_FILE) return err.TooManyRules;
        _.files[slot].rules[rc] = CssImportRule{
            kind: .External,
            target_source_index: 0,
            external_url: url,
            condition: condition,
        };
        _.files[slot].rule_count = rc + 1;
    }

    /// 追加 CSS Modules `composes` 依赖边
    fn addComposes(*_, slot: usize, target_source_index: u32): ! {
        if (slot >= _.file_count) return err.InvalidSourceIndex;
        const rc = _.files[slot].rule_count;
        if (rc >= CSS_IMPORT_MAX_RULES_PER_FILE) return err.TooManyRules;
        _.files[slot].rules[rc] = CssImportRule{
            kind: .Composes,
            target_source_index: target_source_index,
            external_url: "",
            condition: CssImportCondition{},
        };
        _.files[slot].rule_count = rc + 1;
    }

    fn visitDfs(
        source_index: u32,
        conds: CssConditionStack,
        parent_stack: *[CSS_IMPORT_MAX_DEPTH]u32,
        parent_depth: *usize,
        out: *CssImportOrderResult,
    ): ! {
        // WebKit StyleRuleImport 语义：检查当前父链是否存在相同文件，若存在环路立即退出
        let p: usize = 0;
        while (p < parent_depth.*) {
            if (parent_stack[p] == source_index) {
                return;
            }
            p += 1;
        }

        const slot = _.findSlot(source_index) orelse return err.InvalidSourceIndex;

        if (parent_depth.* >= CSS_IMPORT_MAX_DEPTH) {
            return err.MaxDepthExceeded;
        }
        parent_stack[parent_depth.*] = source_index;
        parent_depth.* += 1;

        const file = _.files[slot];

        // 1. 按声明顺序处理所有 @import 规则（内部递归 / 外部记录）
        let i: usize = 0;
        while (i < file.rule_count) {
            const rule = file.rules[i];
            if (rule.kind == .Internal) {
                const next_conds = conds.withPushed(rule.condition)?;
                _.visitDfs(rule.target_source_index, next_conds, parent_stack, parent_depth, out)?;
            } else if (rule.kind == .External) {
                const ext_conds = conds.withPushed(rule.condition)?;
                if (out.count >= CSS_IMPORT_MAX_ORDER_ENTRIES) {
                    parent_depth.* -= 1;
                    return err.TooManyEntries;
                }
                out.entries[out.count] = CssImportOrderEntry{
                    kind: .ExternalUrl,
                    source_index: 0,
                    external_url: rule.external_url,
                    conditions: ext_conds,
                };
                out.count += 1;
                out.has_external_import = true;
            }
            i += 1;
        }

        // 2. 处理 CSS Modules `composes` 依赖边
        let j: usize = 0;
        while (j < file.rule_count) {
            const rule = file.rules[j];
            if (rule.kind == .Composes) {
                _.visitDfs(rule.target_source_index, conds, parent_stack, parent_depth, out)?;
            }
            j += 1;
        }

        parent_depth.* -= 1;

        // 3. 后序输出当前 CSS 文件
        if (out.count >= CSS_IMPORT_MAX_ORDER_ENTRIES) {
            return err.TooManyEntries;
        }
        out.entries[out.count] = CssImportOrderEntry{
            kind: .SourceFile,
            source_index: source_index,
            external_url: "",
            conditions: conds,
        };
        out.count += 1;
    }

    /// 执行完整 CSS `@import` 深度优先遍历、外链顶置与逆序 Last-Wins 去重（对齐 Bun `findImportedFilesInCSSOrder`）
    fn computeCssOrder(entry_points: []const u32): CssImportOrderResult! {
        let raw = CssImportOrderResult{};
        let parent_stack: [CSS_IMPORT_MAX_DEPTH]u32 = [0; CSS_IMPORT_MAX_DEPTH];
        let parent_depth: usize = 0;
        const empty_conds = CssConditionStack{};

        // Pass 1: 从所有入口点执行 DFS 后序展开
        let e: usize = 0;
        while (e < entry_points.len) {
            _.visitDfs(entry_points[e], empty_conds, &parent_stack, &parent_depth, &raw)?;
            e += 1;
        }

        // Pass 2: 若存在外链 @import，将其顶置至最前端以符合 CSS 语法规范
        if (raw.has_external_import) {
            let hoisted: [CSS_IMPORT_MAX_ORDER_ENTRIES]CssImportOrderEntry = [CssImportOrderEntry{}; CSS_IMPORT_MAX_ORDER_ENTRIES];
            let h_count: usize = 0;
            let i: usize = 0;
            while (i < raw.count) {
                if (raw.entries[i].kind == .ExternalUrl) {
                    hoisted[h_count] = raw.entries[i];
                    h_count += 1;
                }
                i += 1;
            }
            let j: usize = 0;
            while (j < raw.count) {
                if (raw.entries[j].kind != .ExternalUrl) {
                    hoisted[h_count] = raw.entries[j];
                    h_count += 1;
                }
                j += 1;
            }
            let k: usize = 0;
            while (k < h_count) {
                raw.entries[k] = hoisted[k];
                k += 1;
            }
            raw.count = h_count;
        }

        // Pass 3: 逆序 Last-Wins 去重（若前驱文件含 @layer 则降级为 LayersOnly，否则标记删除）
        let dropped: [CSS_IMPORT_MAX_ORDER_ENTRIES]bool = [false; CSS_IMPORT_MAX_ORDER_ENTRIES];
        let idx: usize = raw.count;
        while (idx > 0) {
            idx -= 1;
            const cur = raw.entries[idx];
            let is_redundant = false;

            let later_idx: usize = idx + 1;
            while (later_idx < raw.count) {
                if (!dropped[later_idx]) {
                    const later = raw.entries[later_idx];
                    if (cur.kind == .SourceFile && later.kind == .SourceFile &&
                        cur.source_index == later.source_index)
                    {
                        if (cur.conditions.isRedundantBefore(later.conditions)) {
                            is_redundant = true;
                            break;
                        }
                    } else if (cur.kind == .ExternalUrl && later.kind == .ExternalUrl &&
                        strEq(cur.external_url, later.external_url))
                    {
                        if (cur.conditions.isRedundantBefore(later.conditions)) {
                            is_redundant = true;
                            break;
                        }
                    }
                }
                later_idx += 1;
            }

            if (is_redundant) {
                if (cur.kind == .SourceFile) {
                    if (slot := _.findSlot(cur.source_index)) {
                        if (_.files[slot].has_at_layer) {
                            raw.entries[idx].kind = .LayersOnly;
                            continue;
                        }
                    }
                }
                dropped[idx] = true;
            }
        }

        // Pass 4: 压缩输出最终结果
        let final_res = CssImportOrderResult{
            has_external_import: raw.has_external_import,
        };
        let out_i: usize = 0;
        let scan: usize = 0;
        while (scan < raw.count) {
            if (!dropped[scan]) {
                final_res.entries[out_i] = raw.entries[scan];
                out_i += 1;
            }
            scan += 1;
        }
        final_res.count = out_i;
        return final_res;
    }
}
