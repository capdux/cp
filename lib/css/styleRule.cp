// Source: Absorbed from /data/dev/bun/src/css/rules/style.cp | Synced: 2026-11-22T13:35:00Z
// Pure-CP zero-allocation CSS StyleRule model, rule deduplication hash, declaration merger & nesting flattener.
// Faithfully mirrors Bun/LightningCSS `StyleRule` semantics:
// - `isEmpty()` check (empty selectors or empty declarations & nested rules)
// - `hashKey()` 64-bit deterministic FNV-1a fingerprint over selectors + property names
// - Cascade-aware declaration upsert (`!important` protection, case-insensitive standard props vs case-sensitive `--*`)
// - Native CSS Nesting serialization vs browser-target nesting lowering (`&` substitution & descendant cartesian product)

/// 单条样式规则支持的最大逗号分隔选择器数
pub const CSS_STYLE_RULE_MAX_SELECTORS: usize = 16;
/// 单条样式规则支持的最大声明数
pub const CSS_STYLE_RULE_MAX_DECLS: usize = 32;
/// 单条样式规则支持的最大直接嵌套子规则数
pub const CSS_STYLE_RULE_MAX_NESTED: usize = 16;

/// CSS StyleRule 供应商前缀掩码常量
pub const CSS_STYLE_PREFIX_NONE: u8 = 0;
pub const CSS_STYLE_PREFIX_WEBKIT: u8 = 1 << 0;
pub const CSS_STYLE_PREFIX_MOZ: u8 = 1 << 1;
pub const CSS_STYLE_PREFIX_MS: u8 = 1 << 2;
pub const CSS_STYLE_PREFIX_O: u8 = 1 << 3;

/// CSS StyleRule 操作错误集
pub err CssStyleRuleError {
    EmptySelector,
    EmptyProperty,
    CapacityExceeded,
    BufferTooSmall,
    SelectorMismatch,
}

/// 单条 CSS 属性声明
pub struct CssStyleDecl {
    property: []const u8 = "",
    value: []const u8 = "",
    important: bool = false,
}

/// 单条嵌套子样式规则
pub struct CssStyleNestedRule {
    selectors: [CSS_STYLE_RULE_MAX_SELECTORS][]const u8 = [""; CSS_STYLE_RULE_MAX_SELECTORS],
    selector_count: usize = 0,
    decls: [CSS_STYLE_RULE_MAX_DECLS]CssStyleDecl = [CssStyleDecl{}; CSS_STYLE_RULE_MAX_DECLS],
    decl_count: usize = 0,

    static fn init(): CssStyleNestedRule {
        return CssStyleNestedRule{};
    }

    fn addSelector(*_, sel: []const u8): usize! {
        const trimmed = trimAscii(sel);
        if (trimmed.len == 0) return err.EmptySelector;
        if (_.selector_count >= CSS_STYLE_RULE_MAX_SELECTORS) return err.CapacityExceeded;
        _.selectors[_.selector_count] = trimmed;
        _.selector_count += 1;
        return _.selector_count;
    }

    fn setDecl(*_, property: []const u8, value: []const u8, important: bool): usize! {
        return upsertDeclInto(_.decls[0..], &_.decl_count, property, value, important);
    }

    fn isEmpty(): bool {
        return _.selector_count == 0 || _.decl_count == 0;
    }
}

/// 零堆分配 CSS 样式规则（对齐 Bun `css/rules/style.cp` `StyleRule`）
pub struct CssStyleRule {
    selectors: [CSS_STYLE_RULE_MAX_SELECTORS][]const u8 = [""; CSS_STYLE_RULE_MAX_SELECTORS],
    selector_count: usize = 0,

    decls: [CSS_STYLE_RULE_MAX_DECLS]CssStyleDecl = [CssStyleDecl{}; CSS_STYLE_RULE_MAX_DECLS],
    decl_count: usize = 0,

    nested: [CSS_STYLE_RULE_MAX_NESTED]CssStyleNestedRule = [CssStyleNestedRule{}; CSS_STYLE_RULE_MAX_NESTED],
    nested_count: usize = 0,

    vendor_prefix_mask: u8 = CSS_STYLE_PREFIX_NONE,

    static fn init(): CssStyleRule {
        return CssStyleRule{};
    }

    /// 添加一个顶层选择器（自动裁剪首尾空白）
    fn addSelector(*_, sel: []const u8): usize! {
        const trimmed = trimAscii(sel);
        if (trimmed.len == 0) return err.EmptySelector;
        if (_.selector_count >= CSS_STYLE_RULE_MAX_SELECTORS) return err.CapacityExceeded;
        _.selectors[_.selector_count] = trimmed;
        _.selector_count += 1;
        return _.selector_count;
    }

    /// 按 CSS 级联规则写入声明（普通属性不可覆盖 `!important`，`--*` 区分大小写）
    fn setDecl(*_, property: []const u8, value: []const u8, important: bool): usize! {
        return upsertDeclInto(_.decls[0..], &_.decl_count, property, value, important);
    }

    /// 添加一条嵌套子规则
    fn addNested(*_, child: CssStyleNestedRule): usize! {
        if (_.nested_count >= CSS_STYLE_RULE_MAX_NESTED) return err.CapacityExceeded;
        _.nested[_.nested_count] = child;
        _.nested_count += 1;
        return _.nested_count;
    }

    /// 判断规则是否为空（对齐 Bun `StyleRule.isEmpty()`）
    fn isEmpty(): bool {
        if (_.selector_count == 0) return true;
        if (_.decl_count > 0) return false;
        for (i of 0.._.nested_count) {
            if (!_.nested[i].isEmpty()) return false;
        }
        return true;
    }

    /// 返回选择器列表是否完全相等
    fn selectorsEql(other: CssStyleRule): bool {
        if (_.selector_count != other.selector_count) return false;
        for (i of 0.._.selector_count) {
            if (!bytesEql(_.selectors[i], other.selectors[i])) return false;
        }
        return true;
    }

    /// 计算用于规则去重的 64 位确定性哈希键（包含选择器列表与全部属性名，对齐 Bun `StyleRule.hashKey()`）
    fn hashKey(): u64 {
        let h: u64 = 14695981039346656037;
        for (i of 0.._.selector_count) {
            h = fnv1aUpdate(h, _.selectors[i]);
            h = fnv1aByte(h, 0x1f);
        }
        h = fnv1aByte(h, 0x1e);
        for (i of 0.._.decl_count) {
            const prop = _.decls[i].property;
            if (isCustomProperty(prop)) {
                h = fnv1aUpdate(h, prop);
            } else {
                h = fnv1aUpdateLower(h, prop);
            }
            h = fnv1aByte(h, 0x1d);
        }
        return h;
    }

    /// 将具有相同选择器的另一条规则合并到当前规则（遵守 `!important` 优先级）
    fn mergeFrom(*_, other: CssStyleRule): usize! {
        if (!_.selectorsEql(other)) {
            return err.SelectorMismatch;
        }
        let merged: usize = 0;
        for (i of 0..other.decl_count) {
            const d = other.decls[i];
            * = _.setDecl(d.property, d.value, d.important)?;
            merged += 1;
        }
        for (i of 0..other.nested_count) {
            if (!other.nested[i].isEmpty()) {
                * = _.addNested(other.nested[i])?;
                merged += 1;
            }
        }
        return merged;
    }

    /// 序列化为 CSS 字符串：
    /// - `supports_nesting == true`：保留原生 CSS Nesting 嵌套块；
    /// - `supports_nesting == false`：自动降级展平嵌套规则（支持 `&` 替换与后代空格拼接，无自身声明时省略空父块）。
    fn toCss(out: []u8, minify: bool, supports_nesting: bool): usize! {
        if (_.isEmpty()) return 0;

        let pos: usize = 0;
        let has_live_nested = false;
        for (i of 0.._.nested_count) {
            if (!_.nested[i].isEmpty()) {
                has_live_nested = true;
                break;
            }
        }

        const emit_parent_block = supports_nesting || _.decl_count > 0 || !has_live_nested;
        if (emit_parent_block) {
            pos = writeSelectorList(_.selectors[0.._.selector_count], out, pos, minify)?;
            pos = copyInto(out, pos, if (minify) "{" else " {\n")?;
            pos = writeDeclList(_.decls[0.._.decl_count], out, pos, minify, supports_nesting && has_live_nested, "  ")?;

            if (supports_nesting && has_live_nested) {
                for (i of 0.._.nested_count) {
                    const child = _.nested[i];
                    if (child.isEmpty()) continue;
                    if (!minify) {
                        pos = copyInto(out, pos, "  ")?;
                    }
                    pos = writeSelectorList(child.selectors[0..child.selector_count], out, pos, minify)?;
                    pos = copyInto(out, pos, if (minify) "{" else " {\n")?;
                    pos = writeDeclList(child.decls[0..child.decl_count], out, pos, minify, false, "    ")?;
                    pos = copyInto(out, pos, if (minify) "}" else "  }\n")?;
                }
            }
            pos = copyInto(out, pos, "}")?;
        }

        if (!supports_nesting && has_live_nested) {
            let emitted_any = emit_parent_block;
            let sel_buf: [256]u8 = [0; 256];

            for (ni of 0.._.nested_count) {
                const child = _.nested[ni];
                if (child.isEmpty()) continue;

                if (emitted_any) {
                    pos = copyInto(out, pos, if (minify) "" else "\n")?;
                }
                emitted_any = true;

                // 父子选择器列表笛卡尔积展开
                let first_sel = true;
                for (pi of 0.._.selector_count) {
                    for (ci of 0..child.selector_count) {
                        if (!first_sel) {
                            pos = copyInto(out, pos, if (minify) "," else ", ")?;
                        }
                        first_sel = false;
                        const slen = resolveNestedSelector(_.selectors[pi], child.selectors[ci], sel_buf[0..])?;
                        pos = copyInto(out, pos, sel_buf[0..slen])?;
                    }
                }
                pos = copyInto(out, pos, if (minify) "{" else " {\n")?;
                pos = writeDeclList(child.decls[0..child.decl_count], out, pos, minify, false, "  ")?;
                pos = copyInto(out, pos, "}")?;
            }
        }

        return pos;
    }
}

/// 将父选择器与嵌套子选择器合成为降级后的标准选择器：
/// - 子选择器包含 `&` 时，将所有 `&` 替换为 `parent`（如 `.card` + `&:hover` -> `.card:hover`）；
/// - 子选择器不含 `&` 时，按后代选择器拼接 `parent + " " + child`（如 `.card` + `.title` -> `.card .title`）。
pub fn resolveNestedSelector(parent: []const u8, child: []const u8, out: []u8): usize! {
    const p = trimAscii(parent);
    const c = trimAscii(child);
    if (p.len == 0 || c.len == 0) {
        return err.EmptySelector;
    }

    let has_amp = false;
    for (i of 0..c.len) {
        if (c[i] == '&') {
            has_amp = true;
            break;
        }
    }

    let pos: usize = 0;
    if (has_amp) {
        for (i of 0..c.len) {
            if (c[i] == '&') {
                pos = copyInto(out, pos, p)?;
            } else {
                pos = copyByte(out, pos, c[i])?;
            }
        }
        return pos;
    }

    pos = copyInto(out, pos, p)?;
    pos = copyByte(out, pos, ' ')?;
    pos = copyInto(out, pos, c)?;
    return pos;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len) {
        const c = s[start];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            start += 1;
        } else {
            break;
        }
    }
    let end = s.len;
    while (end > start) {
        const c = s[end - 1];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            end -= 1;
        } else {
            break;
        }
    }
    return s[start..end];
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn isCustomProperty(prop: []const u8): bool {
    return prop.len >= 2 && prop[0] == '-' && prop[1] == '-';
}

fn propNamesMatch(a: []const u8, b: []const u8): bool {
    if (isCustomProperty(a) || isCustomProperty(b)) {
        return bytesEql(a, b);
    }
    return eqlAsciiIgnoreCase(a, b);
}

fn fnv1aByte(h: u64, b: u8): u64 {
    return (h ^ (b as u64)) *% 1099511628211;
}

fn fnv1aUpdate(h_in: u64, data: []const u8): u64 {
    let h = h_in;
    for (i of 0..data.len) {
        h = fnv1aByte(h, data[i]);
    }
    return h;
}

fn fnv1aUpdateLower(h_in: u64, data: []const u8): u64 {
    let h = h_in;
    for (i of 0..data.len) {
        h = fnv1aByte(h, toLowerAscii(data[i]));
    }
    return h;
}

fn copyInto(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) return err.BufferTooSmall;
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

fn copyByte(dst: []u8, offset: usize, b: u8): usize! {
    if (offset + 1 > dst.len) return err.BufferTooSmall;
    dst[offset] = b;
    return offset + 1;
}

fn upsertDeclInto(
    decls: []CssStyleDecl,
    count: *usize,
    property: []const u8,
    value: []const u8,
    important: bool,
): usize! {
    const p = trimAscii(property);
    const v = trimAscii(value);
    if (p.len == 0) return err.EmptyProperty;

    for (i of 0..count.*) {
        if (propNamesMatch(decls[i].property, p)) {
            // 已有 !important 不可被普通声明覆盖
            if (decls[i].important && !important) {
                return count.*;
            }
            decls[i] = CssStyleDecl{ property: p, value: v, important: important };
            return count.*;
        }
    }
    if (count.* >= decls.len) return err.CapacityExceeded;
    decls[count.*] = CssStyleDecl{ property: p, value: v, important: important };
    count.* += 1;
    return count.*;
}

fn writeSelectorList(
    selectors: []const ([]const u8),
    out: []u8,
    offset: usize,
    minify: bool,
): usize! {
    let pos = offset;
    for (i of 0..selectors.len) {
        if (i > 0) {
            pos = copyInto(out, pos, if (minify) "," else ", ")?;
        }
        pos = copyInto(out, pos, selectors[i])?;
    }
    return pos;
}

fn writeDeclList(
    decls: []const CssStyleDecl,
    out: []u8,
    offset: usize,
    minify: bool,
    has_following_nested: bool,
    indent: []const u8,
): usize! {
    let pos = offset;
    for (i of 0..decls.len) {
        const d = decls[i];
        if (!minify) {
            pos = copyInto(out, pos, indent)?;
        }
        pos = copyInto(out, pos, d.property)?;
        pos = copyInto(out, pos, if (minify) ":" else ": ")?;
        pos = copyInto(out, pos, d.value)?;
        if (d.important) {
            pos = copyInto(out, pos, if (minify) "!important" else " !important")?;
        }
        const is_last = (i + 1 == decls.len);
        if (!minify) {
            pos = copyInto(out, pos, ";\n")?;
        } else if (!is_last || has_following_nested) {
            pos = copyByte(out, pos, ';')?;
        }
    }
    return pos;
}
