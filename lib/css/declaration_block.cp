// Source: Absorbed from /data/dev/bun/src/css/declaration.cp | Synced: 2026-11-22T04:54:41Z
// Ported from Bun: /data/dev/bun/src/css/declaration.cp & /data/dev/bun/docs/bundler/css.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation CSS DeclarationBlock Parser, Deduplicating Minifier & Serializer:
// - Separates normal declarations and `!important` declarations into dedicated contiguous arrays (mirroring LightningCSS/Bun layout)
// - Parses CSS declaration lists with string/paren/comment awareness, optional `{ ... }` wrapper stripping, and error_recovery mode
// - Performs cascade-safe deduplication (`minifyDeduplicate`), last-wins effective lookup, and FNV-1a `propertyIdHash`
// - Serializes inline (`toCss`) and braced (`toCssBlock`) output in both minified and pretty-printed modes with zero heap allocation.

/// 单个声明块支持的最大普通声明数量
pub const CSS_DECL_MAX_NORMAL: usize = 64;

/// 单个声明块支持的最大 `!important` 声明数量
pub const CSS_DECL_MAX_IMPORTANT: usize = 32;

/// CSS 声明块解析与序列化错误集
pub err CssDeclBlockError {
    TooManyDeclarations,
    TooManyImportantDeclarations,
    EmptyPropertyName,
    MissingColon,
    EmptyPropertyValue,
    UnclosedString,
    UnclosedComment,
    BufferTooSmall,
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') { return c + 32; }
    return c;
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

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (asciiLower(a[i]) != asciiLower(b[i])) { return false; }
        i += 1;
    }
    return true;
}

fn isCustomPropertyName(name: []const u8): bool {
    return name.len >= 2 && name[0] == '-' && name[1] == '-';
}

fn appendSlice(buf: []u8, pos: *usize, s: []const u8): CssDeclBlockError!bool {
    if (pos.* + s.len > buf.len) {
        return CssDeclBlockError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        buf[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn appendByte(buf: []u8, pos: *usize, b: u8): CssDeclBlockError!bool {
    if (pos.* + 1 > buf.len) {
        return CssDeclBlockError.BufferTooSmall;
    }
    buf[pos.*] = b;
    pos.* += 1;
    return true;
}

/// 剥离属性值尾部的 `!important`（允许 `!` 与 `important` 间存在空白且大小写无关）
struct ImportantStripResult {
    value: []const u8 = "",
    is_important: bool = false,
}

fn stripTrailingImportant(raw_val: []const u8): ImportantStripResult {
    const trimmed = trimAscii(raw_val);
    if (trimmed.len < 10) {
        return ImportantStripResult{ value: trimmed, is_important: false };
    }
    // 检查末尾 9 字节是否为 "important"（大小写无关）
    const kw_start = trimmed.len - 9;
    if (!eqlIgnoreCaseAscii(trimmed[kw_start..trimmed.len], "important")) {
        return ImportantStripResult{ value: trimmed, is_important: false };
    }
    let bang_pos = kw_start;
    while (bang_pos > 0 && isAsciiSpace(trimmed[bang_pos - 1])) {
        bang_pos -= 1;
    }
    if (bang_pos == 0 || trimmed[bang_pos - 1] != '!') {
        return ImportantStripResult{ value: trimmed, is_important: false };
    }
    return ImportantStripResult{
        value: trimAscii(trimmed[0..(bang_pos - 1)]),
        is_important: true,
    };
}

/// 单条 CSS 属性声明
pub struct CssDeclItem {
    name: []const u8 = "",
    value: []const u8 = "",
    is_custom: bool = false,

    /// 按 CSS 规范比较属性名：自定义属性 `--*` 区分大小写，标准属性 ASCII 大小写无关
    fn eqlName(other_name: []const u8): bool {
        if (_.is_custom || isCustomPropertyName(other_name)) {
            return eqlBytes(_.name, other_name);
        }
        return eqlIgnoreCaseAscii(_.name, other_name);
    }

    fn eql(other: CssDeclItem): bool {
        return _.eqlName(other.name) && eqlBytes(_.value, other.value);
    }
}

/// 声明块解析选项（对齐 Bun `ParserOptions.error_recovery`）
pub struct CssDeclParseOptions {
    /// 开启时跳过畸形声明并记录 `recovered_errors`，关闭时遇到错误立即返回
    error_recovery: bool = false,
}

/// 零堆分配 CSS 声明块（分离普通声明与 `!important` 声明，对齐 Bun `DeclarationBlock`）
pub struct CssDeclarationBlock {
    normal_count: usize = 0,
    declarations: [CSS_DECL_MAX_NORMAL]CssDeclItem = [CssDeclItem{}; CSS_DECL_MAX_NORMAL],
    important_count: usize = 0,
    important_declarations: [CSS_DECL_MAX_IMPORTANT]CssDeclItem = [CssDeclItem{}; CSS_DECL_MAX_IMPORTANT],
    recovered_errors: u32 = 0,

    static fn init(): CssDeclarationBlock {
        return CssDeclarationBlock{};
    }

    fn isEmpty(): bool {
        return _.normal_count == 0 && _.important_count == 0;
    }

    fn len(): usize {
        return _.normal_count + _.important_count;
    }

    fn normalSlice(): []const CssDeclItem {
        return _.declarations[0.._.normal_count];
    }

    fn importantSlice(): []const CssDeclItem {
        return _.important_declarations[0.._.important_count];
    }

    /// 手动追加一条声明到普通桶或 `!important` 桶
    fn push(*_, name: []const u8, value: []const u8, is_important: bool): CssDeclBlockError!bool {
        const clean_name = trimAscii(name);
        const clean_val = trimAscii(value);
        if (clean_name.len == 0) { return CssDeclBlockError.EmptyPropertyName; }
        if (clean_val.len == 0) { return CssDeclBlockError.EmptyPropertyValue; }

        const item = CssDeclItem{
            name: clean_name,
            value: clean_val,
            is_custom: isCustomPropertyName(clean_name),
        };
        if (is_important) {
            if (_.important_count >= CSS_DECL_MAX_IMPORTANT) {
                return CssDeclBlockError.TooManyImportantDeclarations;
            }
            _.important_declarations[_.important_count] = item;
            _.important_count += 1;
        } else {
            if (_.normal_count >= CSS_DECL_MAX_NORMAL) {
                return CssDeclBlockError.TooManyDeclarations;
            }
            _.declarations[_.normal_count] = item;
            _.normal_count += 1;
        }
        return true;
    }

    /// 解析 CSS 声明列表（支持可选外层 `{ ... }`、引号/括号感知、注释校验与 `error_recovery`）
    static fn parse(raw: []const u8, opts: CssDeclParseOptions): CssDeclBlockError!CssDeclarationBlock {
        let block = CssDeclarationBlock.init();
        let src = trimAscii(raw);
        if (src.len >= 2 && src[0] == '{' && src[src.len - 1] == '}') {
            src = trimAscii(src[1..(src.len - 1)]);
        }
        if (src.len == 0) { return block; }

        let i: usize = 0;
        while (i < src.len) {
            // 跳过前导空白、多余分号和块注释
            while (i < src.len) {
                if (isAsciiSpace(src[i]) || src[i] == ';') {
                    i += 1;
                    continue;
                }
                if (i + 1 < src.len && src[i] == '/' && src[i + 1] == '*') {
                    i += 2;
                    let closed = false;
                    while (i + 1 < src.len) {
                        if (src[i] == '*' && src[i + 1] == '/') {
                            i += 2;
                            closed = true;
                            break;
                        }
                        i += 1;
                    }
                    if (!closed) {
                        return CssDeclBlockError.UnclosedComment;
                    }
                    continue;
                }
                break;
            }
            if (i >= src.len) { break; }

            // 扫描当前声明直至顶层分号或末尾
            const decl_start = i;
            let colon_pos: usize = src.len;
            let paren_depth: usize = 0;
            while (i < src.len) {
                const c = src[i];
                if (c == '"' || c == '\'') {
                    const quote = c;
                    i += 1;
                    let closed_q = false;
                    while (i < src.len) {
                        if (src[i] == '\\' && i + 1 < src.len) {
                            i += 2;
                            continue;
                        }
                        if (src[i] == quote) {
                            i += 1;
                            closed_q = true;
                            break;
                        }
                        i += 1;
                    }
                    if (!closed_q) {
                        return CssDeclBlockError.UnclosedString;
                    }
                    continue;
                }
                if (c == '/' && i + 1 < src.len && src[i + 1] == '*') {
                    i += 2;
                    let closed_c = false;
                    while (i + 1 < src.len) {
                        if (src[i] == '*' && src[i + 1] == '/') {
                            i += 2;
                            closed_c = true;
                            break;
                        }
                        i += 1;
                    }
                    if (!closed_c) {
                        return CssDeclBlockError.UnclosedComment;
                    }
                    continue;
                }
                if (c == '(') {
                    paren_depth += 1;
                    i += 1;
                    continue;
                }
                if (c == ')' && paren_depth > 0) {
                    paren_depth -= 1;
                    i += 1;
                    continue;
                }
                if (c == ':' && paren_depth == 0 && colon_pos == src.len) {
                    colon_pos = i;
                    i += 1;
                    continue;
                }
                if (c == ';' && paren_depth == 0) {
                    break;
                }
                i += 1;
            }
            const decl_end = i;
            if (i < src.len && src[i] == ';') {
                i += 1;
            }

            if (colon_pos >= decl_end) {
                if (opts.error_recovery) {
                    block.recovered_errors += 1;
                    continue;
                }
                return CssDeclBlockError.MissingColon;
            }

            const prop_name = trimAscii(src[decl_start..colon_pos]);
            if (prop_name.len == 0) {
                if (opts.error_recovery) {
                    block.recovered_errors += 1;
                    continue;
                }
                return CssDeclBlockError.EmptyPropertyName;
            }

            const stripped = stripTrailingImportant(src[(colon_pos + 1)..decl_end]);
            if (stripped.value.len == 0) {
                if (opts.error_recovery) {
                    block.recovered_errors += 1;
                    continue;
                }
                return CssDeclBlockError.EmptyPropertyValue;
            }

            * = block.push(prop_name, stripped.value, stripped.is_important)?;
        }

        return block;
    }

    /// 按 CSS 级联优先级查询最终生效的属性值（`!important` 优先于普通声明，同桶后声明覆盖前声明）
    fn getEffective(prop_name: []const u8): CssDeclItem? {
        let ii: usize = _.important_count;
        while (ii > 0) {
            ii -= 1;
            if (_.important_declarations[ii].eqlName(prop_name)) {
                return _.important_declarations[ii];
            }
        }
        let ni: usize = _.normal_count;
        while (ni > 0) {
            ni -= 1;
            if (_.declarations[ni].eqlName(prop_name)) {
                return _.declarations[ni];
            }
        }
        return null;
    }

    /// 级联安全去重压缩：
    /// 1) 同一优先桶内同名属性仅保留最后一次声明（Last-Wins）
    /// 2) 若 `!important` 桶已存在同名属性，则剔除普通桶中的同名冗余声明
    /// 返回被消除的冗余声明总数
    fn minifyDeduplicate(*_): usize {
        let removed: usize = 0;

        // 1. 去重 important_declarations（保留同属性名最后一次出现）
        let kept_imp: usize = 0;
        let i: usize = 0;
        while (i < _.important_count) {
            let shadowed = false;
            let j: usize = i + 1;
            while (j < _.important_count) {
                if (_.important_declarations[i].eqlName(_.important_declarations[j].name)) {
                    shadowed = true;
                    break;
                }
                j += 1;
            }
            if (shadowed) {
                removed += 1;
            } else {
                _.important_declarations[kept_imp] = _.important_declarations[i];
                kept_imp += 1;
            }
            i += 1;
        }
        _.important_count = kept_imp;

        // 2. 去重 declarations（同时剔除被 important 桶或后续普通声明覆盖的条目）
        let kept_norm: usize = 0;
        let k: usize = 0;
        while (k < _.normal_count) {
            let shadowed = false;
            // 是否被 !important 桶覆盖
            let im: usize = 0;
            while (im < _.important_count) {
                if (_.declarations[k].eqlName(_.important_declarations[im].name)) {
                    shadowed = true;
                    break;
                }
                im += 1;
            }
            // 是否被后续普通声明覆盖
            if (!shadowed) {
                let m: usize = k + 1;
                while (m < _.normal_count) {
                    if (_.declarations[k].eqlName(_.declarations[m].name)) {
                        shadowed = true;
                        break;
                    }
                    m += 1;
                }
            }
            if (shadowed) {
                removed += 1;
            } else {
                _.declarations[kept_norm] = _.declarations[k];
                kept_norm += 1;
            }
            k += 1;
        }
        _.normal_count = kept_norm;
        return removed;
    }

    /// 计算属性名序列的 64 位 FNV-1a 签名（对齐 Bun `DeclarationBlock.hashPropertyIds`）
    fn propertyIdHash(): u64 {
        let h: u64 = 14695981039346656037;
        const prime: u64 = 1099511628211;

        let i: usize = 0;
        while (i < _.normal_count) {
            const item = _.declarations[i];
            let ci: usize = 0;
            while (ci < item.name.len) {
                const b = if (item.is_custom) item.name[ci] else asciiLower(item.name[ci]);
                h = (h ^ @as(u64, b)) *% prime;
                ci += 1;
            }
            h = (h ^ 0xFF) *% prime;
            i += 1;
        }
        h = (h ^ 0xFE) *% prime;
        let j: usize = 0;
        while (j < _.important_count) {
            const item = _.important_declarations[j];
            let cj: usize = 0;
            while (cj < item.name.len) {
                const b = if (item.is_custom) item.name[cj] else asciiLower(item.name[cj]);
                h = (h ^ @as(u64, b)) *% prime;
                cj += 1;
            }
            h = (h ^ 0xFF) *% prime;
            j += 1;
        }
        return h;
    }

    /// 序列化为内联声明列表（先普通声明后 `!important` 声明，对齐 Bun `DeclarationBlock.toCss`）
    fn toCss(buf: []u8, minify: bool): CssDeclBlockError!usize {
        let pos: usize = 0;
        const total = _.len();
        let emitted: usize = 0;

        let i: usize = 0;
        while (i < _.normal_count) {
            const d = _.declarations[i];
            * = appendSlice(buf, &pos, d.name)?;
            * = appendSlice(buf, &pos, if (minify) ":" else ": ")?;
            * = appendSlice(buf, &pos, d.value)?;
            if (emitted + 1 < total) {
                * = appendSlice(buf, &pos, if (minify) ";" else "; ")?;
            }
            emitted += 1;
            i += 1;
        }

        let j: usize = 0;
        while (j < _.important_count) {
            const d = _.important_declarations[j];
            * = appendSlice(buf, &pos, d.name)?;
            * = appendSlice(buf, &pos, if (minify) ":" else ": ")?;
            * = appendSlice(buf, &pos, d.value)?;
            * = appendSlice(buf, &pos, if (minify) "!important" else " !important")?;
            if (emitted + 1 < total) {
                * = appendSlice(buf, &pos, if (minify) ";" else "; ")?;
            }
            emitted += 1;
            j += 1;
        }

        return pos;
    }

    /// 序列化为带 `{ ... }` 的 CSS 声明块（对齐 Bun `DeclarationBlock.toCssBlock`）
    fn toCssBlock(buf: []u8, minify: bool): CssDeclBlockError!usize {
        let pos: usize = 0;
        if (minify) {
            * = appendByte(buf, &pos, '{')?;
            if (_.len() > 0) {
                const n = _.toCss(buf[pos..buf.len], true)?;
                pos += n;
            }
            * = appendByte(buf, &pos, '}')?;
            return pos;
        }

        * = appendSlice(buf, &pos, "{\n")?;
        let i: usize = 0;
        while (i < _.normal_count) {
            const d = _.declarations[i];
            * = appendSlice(buf, &pos, "  ")?;
            * = appendSlice(buf, &pos, d.name)?;
            * = appendSlice(buf, &pos, ": ")?;
            * = appendSlice(buf, &pos, d.value)?;
            * = appendSlice(buf, &pos, ";\n")?;
            i += 1;
        }
        let j: usize = 0;
        while (j < _.important_count) {
            const d = _.important_declarations[j];
            * = appendSlice(buf, &pos, "  ")?;
            * = appendSlice(buf, &pos, d.name)?;
            * = appendSlice(buf, &pos, ": ")?;
            * = appendSlice(buf, &pos, d.value)?;
            * = appendSlice(buf, &pos, " !important;\n")?;
            j += 1;
        }
        * = appendByte(buf, &pos, '}')?;
        return pos;
    }
}
