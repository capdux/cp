// Source: Absorbed from /data/dev/bun/src/css/values/rect.cp | Synced: 2026-11-22T04:25:42Z
// Pure-CP zero-allocation CSS 4-side box shorthand (`CssBoxRect`) and 2-component (`CssSize2D`)
// parser (function/calc-aware tokenizer) and minimal-component CSS serializer.

/// CSS 4-side `Rect` 与 2D `Size` 解析与序列化错误集
pub err CssBoxRectError {
    EmptyRect,
    TooManyValues,
    UnclosedDelimiter,
    BufferTooSmall,
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn bytesEql(a: []const u8, b: []const u8): bool {
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

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = ch;
    return pos + 1;
}

/// 将空白分隔的 CSS 值列表拆分为最多 `max_parts` 个顶层组件切片，
/// 正确保护 `calc(...)`、`var(...)`、`rgb(...)` 等圆括号/方括号与引号内部的空白。
fn splitTopLevelValues(raw: []const u8, out_parts: [][]const u8, max_parts: usize): usize! {
    let count: usize = 0;
    let i: usize = 0;

    while (i < raw.len) {
        while (i < raw.len && isWs(raw[i])) {
            i += 1;
        }
        if (i >= raw.len) {
            break;
        }
        const start = i;
        let paren_depth: usize = 0;
        let bracket_depth: usize = 0;
        let in_quote: u8 = 0;

        while (i < raw.len) {
            const c = raw[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < raw.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                i += 1;
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
                i += 1;
                continue;
            }
            if (c == '(') {
                paren_depth += 1;
            } else if (c == ')') {
                if (paren_depth == 0) {
                    return err.UnclosedDelimiter;
                }
                paren_depth -= 1;
            } else if (c == '[') {
                bracket_depth += 1;
            } else if (c == ']') {
                if (bracket_depth == 0) {
                    return err.UnclosedDelimiter;
                }
                bracket_depth -= 1;
            } else if (isWs(c) && paren_depth == 0 && bracket_depth == 0) {
                break;
            }
            i += 1;
        }

        if (paren_depth != 0 || bracket_depth != 0 || in_quote != 0) {
            return err.UnclosedDelimiter;
        }
        if (count >= max_parts) {
            return err.TooManyValues;
        }
        out_parts[count] = raw[start..i];
        count += 1;
    }

    if (count == 0) {
        return err.EmptyRect;
    }
    return count;
}

/// CSS 盒模型四边简写值（`margin` / `padding` / `inset` / `border-width` 等，对齐 Bun `Rect`）
///
/// 解析规则：
/// - 1 值：`a` → `(a, a, a, a)`
/// - 2 值：`a b` → `(a, b, a, b)`
/// - 3 值：`a b c` → `(a, b, c, b)`
/// - 4 值：`a b c d` → `(a, b, c, d)`
///
/// 序列化 `toCss` 自动折叠等价边，输出最少分量（1 / 2 / 3 / 4 值）。
pub struct CssBoxRect {
    top: []const u8 = "",
    right: []const u8 = "",
    bottom: []const u8 = "",
    left: []const u8 = "",

    static fn all(val: []const u8): CssBoxRect {
        return CssBoxRect{
            top: val,
            right: val,
            bottom: val,
            left: val,
        };
    }

    static fn symmetric(vertical: []const u8, horizontal: []const u8): CssBoxRect {
        return CssBoxRect{
            top: vertical,
            right: horizontal,
            bottom: vertical,
            left: horizontal,
        };
    }

    /// 解析 1~4 个顶层空白分隔的 CSS 边值（支持 `calc(...)` / `var(...)` 等含内部空白的函数）
    static fn parse(raw: []const u8): CssBoxRect! {
        let parts: [4][]const u8 = [""; 4];
        const n = splitTopLevelValues(raw, parts[0..], 4)?;
        if (n == 1) {
            return CssBoxRect.all(parts[0]);
        }
        if (n == 2) {
            return CssBoxRect.symmetric(parts[0], parts[1]);
        }
        if (n == 3) {
            return CssBoxRect{
                top: parts[0],
                right: parts[1],
                bottom: parts[2],
                left: parts[1],
            };
        }
        return CssBoxRect{
            top: parts[0],
            right: parts[1],
            bottom: parts[2],
            left: parts[3],
        };
    }

    /// 返回最短等价简写所需的分量数（1 / 2 / 3 / 4）
    fn minimalComponentCount(): usize {
        const same_v = bytesEql(_.top, _.bottom);
        const same_h = bytesEql(_.right, _.left);
        if (same_v && same_h && bytesEql(_.top, _.right)) {
            return 1;
        }
        if (same_v && same_h) {
            return 2;
        }
        if (same_h) {
            return 3;
        }
        return 4;
    }

    /// 四边是否完全一致
    fn isUniform(): bool {
        return _.minimalComponentCount() == 1;
    }

    /// 是否上下对称且左右对称
    fn isSymmetric(): bool {
        return bytesEql(_.top, _.bottom) && bytesEql(_.right, _.left);
    }

    fn eql(other: CssBoxRect): bool {
        return bytesEql(_.top, other.top) &&
            bytesEql(_.right, other.right) &&
            bytesEql(_.bottom, other.bottom) &&
            bytesEql(_.left, other.left);
    }

    /// 最短化 CSS 序列化（对齐 Bun `Rect.toCss`：自动折叠重复边为 1/2/3/4 个分量）
    fn toCss(out: []u8): usize! {
        let pos = appendBytes(out, 0, _.top)?;
        const same_v = bytesEql(_.top, _.bottom);
        const same_h = bytesEql(_.right, _.left);
        if (same_v && same_h && bytesEql(_.top, _.right)) {
            return pos;
        }
        pos = appendChar(out, pos, ' ')?;
        pos = appendBytes(out, pos, _.right)?;
        if (same_v && same_h) {
            return pos;
        }
        pos = appendChar(out, pos, ' ')?;
        pos = appendBytes(out, pos, _.bottom)?;
        if (same_h) {
            return pos;
        }
        pos = appendChar(out, pos, ' ')?;
        pos = appendBytes(out, pos, _.left)?;
        return pos;
    }

    /// 完整展开为 4 边序列 `top right bottom left`
    fn toCssExpanded(out: []u8): usize! {
        let pos = appendBytes(out, 0, _.top)?;
        pos = appendChar(out, pos, ' ')?;
        pos = appendBytes(out, pos, _.right)?;
        pos = appendChar(out, pos, ' ')?;
        pos = appendBytes(out, pos, _.bottom)?;
        pos = appendChar(out, pos, ' ')?;
        pos = appendBytes(out, pos, _.left)?;
        return pos;
    }
}

/// CSS 二维对称简写值（`gap` / `overflow` / `border-spacing` 等 1~2 值属性）
pub struct CssSize2D {
    first: []const u8 = "",
    second: []const u8 = "",

    static fn both(val: []const u8): CssSize2D {
        return CssSize2D{ first: val, second: val };
    }

    static fn parse(raw: []const u8): CssSize2D! {
        let parts: [2][]const u8 = [""; 2];
        const n = splitTopLevelValues(raw, parts[0..], 2)?;
        if (n == 1) {
            return CssSize2D.both(parts[0]);
        }
        return CssSize2D{ first: parts[0], second: parts[1] };
    }

    fn isUniform(): bool {
        return bytesEql(_.first, _.second);
    }

    fn eql(other: CssSize2D): bool {
        return bytesEql(_.first, other.first) && bytesEql(_.second, other.second);
    }

    /// 最短化序列化：若两分量相等仅输出单值
    fn toCss(out: []u8): usize! {
        let pos = appendBytes(out, 0, _.first)?;
        if (!bytesEql(_.first, _.second)) {
            pos = appendChar(out, pos, ' ')?;
            pos = appendBytes(out, pos, _.second)?;
        }
        return pos;
    }
}
