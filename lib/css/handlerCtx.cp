// Source: Absorbed from /data/dev/bun/src/css/context.cp | Synced: 2026-11-22T12:49:17Z
// Ported from Bun: /data/dev/bun/src/css/context.cp & /data/dev/bun/docs/bundler/css.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation CSS PropertyHandlerContext for Progressive Lowering:
// - Tracks DeclarationContext (None / StyleRule / Keyframes / StyleAttribute) and disables logical lowering inside inline style attributes
// - Aggregates `@supports (...)` conditional fallback declarations grouped by condition with separate normal / `!important` buckets
// - Collects LTR/RTL directional fallback pairs (`:dir(ltr)` / `:dir(rtl)`) and Dark Mode (`@media (prefers-color-scheme: dark)`) rules
// - Tracks unused CSS Modules symbols for dead-rule pruning and formats lowered auxiliary CSS blocks with zero heap allocation.

import { CssDeclarationBlock } from "./declBlock.cp";

/// 单条样式规则最多收集的 `@supports` 条件回退桶数量
pub const CSS_CTX_MAX_SUPPORTS: usize = 16;

/// 上下文最多跟踪的未使用 CSS Modules 符号数量
pub const CSS_CTX_MAX_UNUSED_SYMBOLS: usize = 32;

/// CSS 属性处理上下文错误集
pub err CssHandlerContextError {
    TooManySupportsBuckets,
    TooManyUnusedSymbols,
    EmptyCondition,
    EmptySelector,
    InvalidDeclaration,
    BufferTooSmall,
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isAsciiSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isAsciiSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn appendSlice(buf: []u8, pos: *usize, s: []const u8): CssHandlerContextError!bool {
    if (pos.* + s.len > buf.len) {
        return CssHandlerContextError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        buf[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn appendByte(buf: []u8, pos: *usize, b: u8): CssHandlerContextError!bool {
    if (pos.* + 1 > buf.len) {
        return CssHandlerContextError.BufferTooSmall;
    }
    buf[pos.*] = b;
    pos.* += 1;
    return true;
}

/// 为逗号分隔的选择器列表的每个成员追加 `:dir(ltr)` 或 `:dir(rtl)`（括号内逗号不拆分）
fn appendDirSelectorList(buf: []u8, pos: *usize, selector_list: []const u8, dir_suffix: []const u8, minify: bool): CssHandlerContextError!bool {
    let i: usize = 0;
    let seg_start: usize = 0;
    let paren_depth: usize = 0;
    let wrote_any = false;

    while (i <= selector_list.len) {
        let is_split = false;
        if (i == selector_list.len) {
            is_split = true;
        } else {
            const c = selector_list[i];
            if (c == '(') {
                paren_depth += 1;
            } else if (c == ')' && paren_depth > 0) {
                paren_depth -= 1;
            } else if (c == ',' && paren_depth == 0) {
                is_split = true;
            }
        }

        if (is_split) {
            const part = trimAscii(selector_list[seg_start..i]);
            if (part.len > 0) {
                if (wrote_any) {
                    * = appendSlice(buf, pos, if (minify) "," else ", ")?;
                }
                * = appendSlice(buf, pos, part)?;
                * = appendSlice(buf, pos, dir_suffix)?;
                wrote_any = true;
            }
            seg_start = i + 1;
        }
        i += 1;
    }
    if (!wrote_any) {
        return CssHandlerContextError.EmptySelector;
    }
    return true;
}

/// 声明所在语法上下文（对齐 Bun `DeclarationContext`）
pub enum CssDeclarationContext: u8 {
    None = 0,
    StyleRule = 1,
    Keyframes = 2,
    StyleAttribute = 3,

    /// 行内 `style="..."` 属性无法生成伴生 `:dir(ltr)`/`:dir(rtl)` 规则，因此禁止逻辑属性降级
    fn allowsLogicalLowering(): bool {
        return _ != .StyleAttribute;
    }

    /// 仅普通样式规则允许输出伴生 `@supports` / `:dir()` / `@media` 规则
    fn allowsAuxiliaryRules(): bool {
        return _ == .StyleRule;
    }
}

/// 按 `@supports` 条件聚合的回退声明桶（对齐 Bun `SupportsEntry`）
pub struct CssSupportsFallbackBucket {
    condition: []const u8 = "",
    block: CssDeclarationBlock = CssDeclarationBlock.init(),
}

/// 零堆分配 CSS 属性降级处理上下文（对齐 Bun `PropertyHandlerContext`）
pub struct CssPropertyHandlerContext {
    context: CssDeclarationContext = .None,
    is_important: bool = false,
    compile_logical_enabled: bool = true,
    supports_count: usize = 0,
    supports: [CSS_CTX_MAX_SUPPORTS]CssSupportsFallbackBucket = [CssSupportsFallbackBucket{}; CSS_CTX_MAX_SUPPORTS],
    ltr: CssDeclarationBlock = CssDeclarationBlock.init(),
    rtl: CssDeclarationBlock = CssDeclarationBlock.init(),
    dark: CssDeclarationBlock = CssDeclarationBlock.init(),
    unused_symbol_count: usize = 0,
    unused_symbols: [CSS_CTX_MAX_UNUSED_SYMBOLS]([]const u8) = [""; CSS_CTX_MAX_UNUSED_SYMBOLS],

    static fn init(ctx: CssDeclarationContext): CssPropertyHandlerContext {
        return CssPropertyHandlerContext{
            context: ctx,
        };
    }

    /// 创建子上下文：继承目标编译配置与未使用符号表，清空临时回退规则桶（对齐 Bun `context.cp:60-72`）
    fn child(ctx: CssDeclarationContext): CssPropertyHandlerContext {
        let out = CssPropertyHandlerContext{
            context: ctx,
            is_important: false,
            compile_logical_enabled: _.compile_logical_enabled,
            unused_symbol_count: _.unused_symbol_count,
        };
        let i: usize = 0;
        while (i < _.unused_symbol_count) {
            out.unused_symbols[i] = _.unused_symbols[i];
            i += 1;
        }
        return out;
    }

    /// 当前上下文是否允许执行逻辑属性降级
    fn shouldCompileLogical(): bool {
        return _.compile_logical_enabled && _.context.allowsLogicalLowering();
    }

    /// 登记未使用的 CSS Modules 局部符号（供死规则剪枝查询）
    fn markUnusedSymbol(*_, sym: []const u8): CssHandlerContextError!bool {
        if (sym.len == 0) { return false; }
        if (_.isSymbolUnused(sym)) { return false; }
        if (_.unused_symbol_count >= CSS_CTX_MAX_UNUSED_SYMBOLS) {
            return CssHandlerContextError.TooManyUnusedSymbols;
        }
        _.unused_symbols[_.unused_symbol_count] = sym;
        _.unused_symbol_count += 1;
        return true;
    }

    fn isSymbolUnused(sym: []const u8): bool {
        let i: usize = 0;
        while (i < _.unused_symbol_count) {
            if (eqlBytes(_.unused_symbols[i], sym)) { return true; }
            i += 1;
        }
        return false;
    }

    /// 添加一条 `@supports` 条件回退声明（同条件自动合并到同一桶，遵循当前 `is_important` 状态）
    fn addConditionalProperty(
        *_,
        condition: []const u8,
        prop_name: []const u8,
        prop_value: []const u8,
    ): CssHandlerContextError!bool {
        const clean_cond = trimAscii(condition);
        if (clean_cond.len == 0) {
            return CssHandlerContextError.EmptyCondition;
        }

        let slot: usize = _.supports_count;
        let i: usize = 0;
        while (i < _.supports_count) {
            if (eqlBytes(_.supports[i].condition, clean_cond)) {
                slot = i;
                break;
            }
            i += 1;
        }
        if (slot == _.supports_count) {
            if (_.supports_count >= CSS_CTX_MAX_SUPPORTS) {
                return CssHandlerContextError.TooManySupportsBuckets;
            }
            _.supports[slot] = CssSupportsFallbackBucket{
                condition: clean_cond,
                block: CssDeclarationBlock.init(),
            };
            _.supports_count += 1;
        }

        _.supports[slot].block.push(prop_name, prop_value, _.is_important) catch {
            return CssHandlerContextError.InvalidDeclaration;
        };
        return true;
    }

    /// 添加一对 LTR/RTL 逻辑属性物理降级声明；若当前上下文禁止逻辑降级（如 `StyleAttribute`）返回 `false`
    fn addLogicalRule(
        *_,
        ltr_prop: []const u8,
        ltr_val: []const u8,
        rtl_prop: []const u8,
        rtl_val: []const u8,
    ): CssHandlerContextError!bool {
        if (!_.shouldCompileLogical()) {
            return false;
        }
        _.ltr.push(ltr_prop, ltr_val, _.is_important) catch {
            return CssHandlerContextError.InvalidDeclaration;
        };
        _.rtl.push(rtl_prop, rtl_val, _.is_important) catch {
            return CssHandlerContextError.InvalidDeclaration;
        };
        return true;
    }

    /// 添加一条深色模式（`prefers-color-scheme: dark`）降级声明（用于 `light-dark(...)` 等特性）
    fn addDarkRule(*_, prop_name: []const u8, prop_value: []const u8): CssHandlerContextError!bool {
        _.dark.push(prop_name, prop_value, _.is_important) catch {
            return CssHandlerContextError.InvalidDeclaration;
        };
        return true;
    }

    fn hasAdditionalRules(): bool {
        return _.supports_count > 0 || !_.ltr.isEmpty() || !_.rtl.isEmpty() || !_.dark.isEmpty();
    }

    /// 清空当前规则收集的全部临时回退桶（保留上下文模式、逻辑降级开关与未使用符号表）
    fn reset(*_) {
        _.supports_count = 0;
        _.ltr = CssDeclarationBlock.init();
        _.rtl = CssDeclarationBlock.init();
        _.dark = CssDeclarationBlock.init();
        _.is_important = false;
    }

    /// 序列化所有 `@supports` 回退规则块
    fn formatSupportsRules(selector: []const u8, buf: []u8, minify: bool): CssHandlerContextError!usize {
        const clean_sel = trimAscii(selector);
        if (clean_sel.len == 0) {
            return CssHandlerContextError.EmptySelector;
        }
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.supports_count) {
            const bucket = _.supports[i];
            if (bucket.block.isEmpty()) {
                i += 1;
                continue;
            }
            if (pos > 0 && !minify) {
                * = appendByte(buf, &pos, '\n')?;
            }
            * = appendSlice(buf, &pos, "@supports ")?;
            * = appendSlice(buf, &pos, bucket.condition)?;
            * = appendSlice(buf, &pos, if (minify) "{" else " {\n  ")?;
            * = appendSlice(buf, &pos, clean_sel)?;
            if (!minify) {
                * = appendByte(buf, &pos, ' ')?;
            }
            const decl_len = bucket.block.toCssBlock(buf[pos..buf.len], true) catch {
                return CssHandlerContextError.BufferTooSmall;
            };
            pos += decl_len;
            * = appendSlice(buf, &pos, if (minify) "}" else "\n}")?;
            i += 1;
        }
        return pos;
    }

    /// 按 Bun `getAdditionalRules` 顺序序列化 `:dir(ltr)`、`:dir(rtl)` 与 `@media (prefers-color-scheme: dark)` 规则
    fn formatAdditionalRules(selector: []const u8, buf: []u8, minify: bool): CssHandlerContextError!usize {
        const clean_sel = trimAscii(selector);
        if (clean_sel.len == 0) {
            return CssHandlerContextError.EmptySelector;
        }
        let pos: usize = 0;

        if (!_.ltr.isEmpty()) {
            * = appendDirSelectorList(buf, &pos, clean_sel, ":dir(ltr)", minify)?;
            if (!minify) { * = appendByte(buf, &pos, ' ')?; }
            const n = _.ltr.toCssBlock(buf[pos..buf.len], true) catch {
                return CssHandlerContextError.BufferTooSmall;
            };
            pos += n;
        }

        if (!_.rtl.isEmpty()) {
            if (pos > 0 && !minify) { * = appendByte(buf, &pos, '\n')?; }
            * = appendDirSelectorList(buf, &pos, clean_sel, ":dir(rtl)", minify)?;
            if (!minify) { * = appendByte(buf, &pos, ' ')?; }
            const n = _.rtl.toCssBlock(buf[pos..buf.len], true) catch {
                return CssHandlerContextError.BufferTooSmall;
            };
            pos += n;
        }

        if (!_.dark.isEmpty()) {
            if (pos > 0 && !minify) { * = appendByte(buf, &pos, '\n')?; }
            * = appendSlice(
                buf,
                &pos,
                if (minify) "@media (prefers-color-scheme:dark){" else "@media (prefers-color-scheme: dark) {\n  ",
            )?;
            * = appendSlice(buf, &pos, clean_sel)?;
            if (!minify) { * = appendByte(buf, &pos, ' ')?; }
            const n = _.dark.toCssBlock(buf[pos..buf.len], true) catch {
                return CssHandlerContextError.BufferTooSmall;
            };
            pos += n;
            * = appendSlice(buf, &pos, if (minify) "}" else "\n}")?;
        }

        return pos;
    }
}
