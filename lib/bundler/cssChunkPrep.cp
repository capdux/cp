// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/prepareCssAstsForChunk.cp | Synced: 2026-11-22T13:32:00Z
// Pure-CP zero-allocation CSS Chunk AST preparation engine.
// Faithfully implements Bun's `prepareCssAstsForChunk` invariants:
// - Prefix `@import` / `ignored` stripping while preserving interleaved `@layer` statements (Regression #28914)
// - Inside-out condition wrapping (`@layer` -> `@supports` -> `@media`) with empty-body side-effect semantics:
//   * empty anonymous `@layer {}` is omitted
//   * empty named `@layer foo {}` lowers to `@layer foo;` (preserves layer cascade order)
//   * empty `@supports` / `@media` wrappers are omitted
// - Nested conditional external `@import` serialization (single-layer direct vs multi-layer `data:text/css,` chain)
// - Reverse-order top-level rule deduplication (last declaration wins to preserve CSS cascade)

/// 单次预处理支持的最大 CSS 规则条数
pub const CSS_PREP_MAX_RULES: usize = 64;
/// 单条导入支持的最大嵌套条件层数
pub const CSS_PREP_MAX_CONDITIONS: usize = 8;
/// 单次条件包装内部临时缓冲容量（字节）
pub const CSS_PREP_SCRATCH_CAP: usize = 1024;

/// CSS Chunk 预处理错误集
pub err CssChunkPrepError {
    CapacityExceeded,
    BufferTooSmall,
    EmptyExternalUrl,
}

/// 顶层 CSS 规则种类
pub enum CssPrepRuleKind: u8 {
    Import = 0,
    Ignored = 1,
    LayerStatement = 2,
    StyleRule = 3,
    AtRule = 4,
}

/// 轻量零拷贝 CSS 规则描述
pub struct CssPrepRule {
    kind: CssPrepRuleKind = .StyleRule,
    text: []const u8 = "",
    /// 用于跨文件逆序去重的规范键（空串表示不参与去重）
    dedup_key: []const u8 = "",
}

/// 零堆分配 CSS 规则输出列表
pub struct CssPrepRuleList {
    items: [CSS_PREP_MAX_RULES]CssPrepRule = [CssPrepRule{}; CSS_PREP_MAX_RULES],
    count: usize = 0,

    static fn init(): CssPrepRuleList {
        return CssPrepRuleList{};
    }

    fn push(*_, rule: CssPrepRule): usize! {
        if (_.count >= CSS_PREP_MAX_RULES) {
            return err.CapacityExceeded;
        }
        _.items[_.count] = rule;
        _.count += 1;
        return _.count;
    }

    fn slice(): []const CssPrepRule {
        return _.items[0.._.count];
    }
}

/// 单层 `@import` 携带的级联条件（`layer` / `supports` / `media`）
pub struct CssPrepCondition {
    /// 是否声明了 `layer` 或 `layer(<name>)`
    has_layer: bool = false,
    /// `null` 表示匿名 `layer`，非空表示具名 `layer(<name>)`
    layer_name: ([]const u8)? = null,
    /// 可选 `supports(...)` 条件正文（不含外层 `supports(...)`）
    supports: ([]const u8)? = null,
    /// 可选 `<media-query-list>` 正文
    media: ([]const u8)? = null,
}

/// 前缀 `@import` / `ignored` 剥离统计（对齐 Bun Regression #28914）
pub struct CssPrepPrefixStats {
    original_count: usize = 0,
    stripped_count: usize = 0,
    preserved_prefix_layers: usize = 0,
    kept_count: usize = 0,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn copyInto(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) {
        return err.BufferTooSmall;
    }
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

fn copyByte(dst: []u8, offset: usize, b: u8): usize! {
    if (offset + 1 > dst.len) {
        return err.BufferTooSmall;
    }
    dst[offset] = b;
    return offset + 1;
}

/// 剥离 CSS 文件头部的 `@import` 与 `ignored` 规则，同时无损保留交错出现的 `@layer` 顺序声明语句
/// （对齐 Bun `prepareCssAstsForChunk.cp` Regression #28914，绝不原地修改输入切片 `src`）
pub fn stripCssPrefixImportsPreservingLayers(
    src: []const CssPrepRule,
    out: *CssPrepRuleList,
): CssPrepPrefixStats! {
    out.count = 0;

    let layer_count: usize = 0;
    let prefix_end: usize = src.len;
    for (i of 0..src.len) {
        const kind = src[i].kind;
        if (kind == .Import || kind == .Ignored) {
            continue;
        }
        if (kind == .LayerStatement) {
            layer_count += 1;
            continue;
        }
        prefix_end = i;
        break;
    }

    const stripped = prefix_end - layer_count;

    // 保留前缀中的 @layer 语句
    for (i of 0..prefix_end) {
        if (src[i].kind == .LayerStatement) {
            * = out.push(src[i])?;
        }
    }
    // 追加前缀之后的所有主体规则
    for (i of prefix_end..src.len) {
        * = out.push(src[i])?;
    }

    return CssPrepPrefixStats{
        original_count: src.len,
        stripped_count: stripped,
        preserved_prefix_layers: layer_count,
        kept_count: out.count,
    };
}

/// 逆序应用 `@import` 条件链，将 CSS 主体按 `layer` -> `supports` -> `media` 从内到外包装。
/// 严格遵循 Bun `wrapRulesWithConditions` 的空体副作用规则：
/// - 空匿名 `@layer {}` 省略；
/// - 空具名 `@layer foo {}` 降级为 `@layer foo;`；
/// - 空 `@supports` / `@media` 省略。
pub fn wrapCssBodyWithConditions(
    body: []const u8,
    conditions: []const CssPrepCondition,
    out: []u8,
): usize! {
    if (body.len > CSS_PREP_SCRATCH_CAP) {
        return err.BufferTooSmall;
    }

    let cur_buf: [CSS_PREP_SCRATCH_CAP]u8 = [0; CSS_PREP_SCRATCH_CAP];
    let nxt_buf: [CSS_PREP_SCRATCH_CAP]u8 = [0; CSS_PREP_SCRATCH_CAP];
    let cur_len = copyInto(cur_buf[0..], 0, body)?;

    let ci = conditions.len;
    while (ci > 0) {
        ci -= 1;
        const cond = conditions[ci];

        // 1. @layer 包装（空具名层降级为语句以保留层序副作用）
        if (cond.has_layer) {
            if (cur_len == 0) {
                if (lname := cond.layer_name) {
                    if (lname.len > 0) {
                        let n: usize = 0;
                        n = copyInto(nxt_buf[0..], n, "@layer ")?;
                        n = copyInto(nxt_buf[0..], n, lname)?;
                        n = copyInto(nxt_buf[0..], n, ";")?;
                        cur_len = copyInto(cur_buf[0..], 0, nxt_buf[0..n])?;
                    }
                }
            } else {
                let n: usize = 0;
                n = copyInto(nxt_buf[0..], n, "@layer")?;
                if (lname := cond.layer_name) {
                    if (lname.len > 0) {
                        n = copyInto(nxt_buf[0..], n, " ")?;
                        n = copyInto(nxt_buf[0..], n, lname)?;
                    }
                }
                n = copyInto(nxt_buf[0..], n, "{")?;
                n = copyInto(nxt_buf[0..], n, cur_buf[0..cur_len])?;
                n = copyInto(nxt_buf[0..], n, "}")?;
                cur_len = copyInto(cur_buf[0..], 0, nxt_buf[0..n])?;
            }
        }

        // 2. @supports 包装（仅在主体非空时生成）
        if (cur_len > 0) {
            if (sup := cond.supports) {
                if (sup.len > 0) {
                    let n: usize = 0;
                    n = copyInto(nxt_buf[0..], n, "@supports (")?;
                    n = copyInto(nxt_buf[0..], n, sup)?;
                    n = copyInto(nxt_buf[0..], n, "){")?;
                    n = copyInto(nxt_buf[0..], n, cur_buf[0..cur_len])?;
                    n = copyInto(nxt_buf[0..], n, "}")?;
                    cur_len = copyInto(cur_buf[0..], 0, nxt_buf[0..n])?;
                }
            }
        }

        // 3. @media 包装（仅在主体非空时生成）
        if (cur_len > 0) {
            if (med := cond.media) {
                if (med.len > 0) {
                    let n: usize = 0;
                    n = copyInto(nxt_buf[0..], n, "@media ")?;
                    n = copyInto(nxt_buf[0..], n, med)?;
                    n = copyInto(nxt_buf[0..], n, "{")?;
                    n = copyInto(nxt_buf[0..], n, cur_buf[0..cur_len])?;
                    n = copyInto(nxt_buf[0..], n, "}")?;
                    cur_len = copyInto(cur_buf[0..], 0, nxt_buf[0..n])?;
                }
            }
        }
    }

    return copyInto(out, 0, cur_buf[0..cur_len]);
}

fn writeSingleImport(
    url: []const u8,
    cond: CssPrepCondition?,
    out: []u8,
    offset: usize,
): usize! {
    let pos = copyInto(out, offset, "@import \"")?;
    pos = copyInto(out, pos, url)?;
    pos = copyInto(out, pos, "\"")?;

    if (c := cond) {
        if (c.has_layer) {
            if (lname := c.layer_name) {
                if (lname.len > 0) {
                    pos = copyInto(out, pos, " layer(")?;
                    pos = copyInto(out, pos, lname)?;
                    pos = copyInto(out, pos, ")")?;
                } else {
                    pos = copyInto(out, pos, " layer")?;
                }
            } else {
                pos = copyInto(out, pos, " layer")?;
            }
        }
        if (sup := c.supports) {
            if (sup.len > 0) {
                pos = copyInto(out, pos, " supports(")?;
                pos = copyInto(out, pos, sup)?;
                pos = copyInto(out, pos, ")")?;
            }
        }
        if (med := c.media) {
            if (med.len > 0) {
                pos = copyInto(out, pos, " ")?;
                pos = copyInto(out, pos, med)?;
            }
        }
    }

    return copyInto(out, pos, ";");
}

fn percentEncodeDataCss(src: []const u8, out: []u8, offset: usize): usize! {
    const hex = "0123456789ABCDEF";
    let pos = offset;
    for (i of 0..src.len) {
        const b = src[i];
        if (b == '"' || b == '#' || b == '%' || b == '\n' || b == '\r') {
            pos = copyByte(out, pos, '%')?;
            pos = copyByte(out, pos, hex[(b >> 4) as usize])?;
            pos = copyByte(out, pos, hex[(b & 0x0f) as usize])?;
        } else {
            pos = copyByte(out, pos, b)?;
        }
    }
    return pos;
}

/// 格式化带条件链的外部 CSS `@import`（对齐 Bun `external_path` 处理逻辑）：
/// - 0 或 1 层条件直接输出单条 `@import "<url>" ...;`
/// - 多层嵌套条件通过 `data:text/css,` 链保留嵌套 `@layer` / `supports` / `media` 语义
pub fn formatNestedExternalImport(
    url: []const u8,
    conditions: []const CssPrepCondition,
    out: []u8,
): usize! {
    if (url.len == 0) {
        return err.EmptyExternalUrl;
    }
    if (conditions.len == 0) {
        return writeSingleImport(url, null, out, 0);
    }
    if (conditions.len == 1) {
        return writeSingleImport(url, conditions[0], out, 0);
    }

    let cur_url_buf: [CSS_PREP_SCRATCH_CAP]u8 = [0; CSS_PREP_SCRATCH_CAP];
    let inner_stmt_buf: [CSS_PREP_SCRATCH_CAP]u8 = [0; CSS_PREP_SCRATCH_CAP];
    let next_url_buf: [CSS_PREP_SCRATCH_CAP]u8 = [0; CSS_PREP_SCRATCH_CAP];

    let cur_url_len = copyInto(cur_url_buf[0..], 0, url)?;

    // 从最内层条件向外封装为 data:text/css, URL，直至保留最外层 conditions[0]
    let j = conditions.len;
    while (j > 1) {
        j -= 1;
        const stmt_len = writeSingleImport(cur_url_buf[0..cur_url_len], conditions[j], inner_stmt_buf[0..], 0)?;
        let n = copyInto(next_url_buf[0..], 0, "data:text/css,")?;
        n = percentEncodeDataCss(inner_stmt_buf[0..stmt_len], next_url_buf[0..], n)?;
        cur_url_len = copyInto(cur_url_buf[0..], 0, next_url_buf[0..n])?;
    }

    return writeSingleImport(cur_url_buf[0..cur_url_len], conditions[0], out, 0);
}

/// 逆序跨文件顶层规则去重（后声明胜出，保持 CSS 级联优先级），返回被去重移除的规则数量
pub fn dedupCssChunkRulesReverse(
    src: []const CssPrepRule,
    out: *CssPrepRuleList,
): usize! {
    if (src.len > CSS_PREP_MAX_RULES) {
        return err.CapacityExceeded;
    }
    let keep: [CSS_PREP_MAX_RULES]bool = [true; CSS_PREP_MAX_RULES];
    let removed: usize = 0;

    let i = src.len;
    while (i > 0) {
        i -= 1;
        const key = src[i].dedup_key;
        if (key.len == 0) {
            continue;
        }
        // 检查在更靠后（索引 > i）且保留的规则中是否已出现相同 key
        let dup = false;
        let k = i + 1;
        while (k < src.len) {
            if (keep[k] && src[k].dedup_key.len > 0 && bytesEql(src[k].dedup_key, key)) {
                dup = true;
                break;
            }
            k += 1;
        }
        if (dup) {
            keep[i] = false;
            removed += 1;
        }
    }

    out.count = 0;
    for (idx of 0..src.len) {
        if (keep[idx]) {
            * = out.push(src[idx])?;
        }
    }
    return removed;
}
