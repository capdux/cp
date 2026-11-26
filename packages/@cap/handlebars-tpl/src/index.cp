// @cap/handlebars-tpl — Zero-allocation Handlebars template engine for CP
// Clean-room MIT implementation with parity to npm `handlebars` v4.
// Composes `@cap/escape-html` for XSS-safe `{{var}}` HTML escaping with zero duplication.

import { escapeHtml } from "../../escape-html/src/index.cp";

const HBS_KIND_STR: u8 = 0;
const HBS_KIND_BOOL: u8 = 1;
const HBS_KIND_COUNT: u8 = 2;
const HBS_MAX_ENTRIES: usize = 32;
const HBS_MAX_DEPTH: usize = 16;

struct HbsEntry {
    key: string,
    kind: u8,
    str_val: string,
    bool_val: bool,
    count_val: usize,
}

/// Fixed-capacity (32 entries) zero-allocation template context.
pub struct HbsContext {
    pub entries: [32]HbsEntry,
    pub len: usize,

    /// Create an empty `HbsContext`.
    pub static fn new(): HbsContext {
        const empty_entry = HbsEntry {
            key: "",
            kind: HBS_KIND_STR,
            str_val: "",
            bool_val: false,
            count_val: 0,
        };
        let arr: [32]HbsEntry = [
            empty_entry, empty_entry, empty_entry, empty_entry,
            empty_entry, empty_entry, empty_entry, empty_entry,
            empty_entry, empty_entry, empty_entry, empty_entry,
            empty_entry, empty_entry, empty_entry, empty_entry,
            empty_entry, empty_entry, empty_entry, empty_entry,
            empty_entry, empty_entry, empty_entry, empty_entry,
            empty_entry, empty_entry, empty_entry, empty_entry,
            empty_entry, empty_entry, empty_entry, empty_entry,
        ];
        return HbsContext {
            entries: arr,
            len: 0,
        };
    }

    /// Bind or update a string variable `k` -> `v`.
    pub fn setStr(*_, k: string, v: string) {
        let i: usize = 0;
        while (i < _.len) {
            if (hbsStrEq(_.entries[i].key, k)) {
                _.entries[i].kind = HBS_KIND_STR;
                _.entries[i].str_val = v;
                return;
            }
            i = i + 1;
        }
        if (_.len < HBS_MAX_ENTRIES) {
            _.entries[_.len] = HbsEntry {
                key: k,
                kind: HBS_KIND_STR,
                str_val: v,
                bool_val: false,
                count_val: 0,
            };
            _.len = _.len + 1;
        }
    }

    /// Bind or update a boolean variable `k` -> `v`.
    pub fn setBool(*_, k: string, v: bool) {
        let i: usize = 0;
        while (i < _.len) {
            if (hbsStrEq(_.entries[i].key, k)) {
                _.entries[i].kind = HBS_KIND_BOOL;
                _.entries[i].bool_val = v;
                return;
            }
            i = i + 1;
        }
        if (_.len < HBS_MAX_ENTRIES) {
            _.entries[_.len] = HbsEntry {
                key: k,
                kind: HBS_KIND_BOOL,
                str_val: "",
                bool_val: v,
                count_val: 0,
            };
            _.len = _.len + 1;
        }
    }

    /// Bind or update an unsigned count variable `k` -> `n` (used by `{{#repeat key}}` and interpolation).
    pub fn setCount(*_, k: string, n: usize) {
        let i: usize = 0;
        while (i < _.len) {
            if (hbsStrEq(_.entries[i].key, k)) {
                _.entries[i].kind = HBS_KIND_COUNT;
                _.entries[i].count_val = n;
                return;
            }
            i = i + 1;
        }
        if (_.len < HBS_MAX_ENTRIES) {
            _.entries[_.len] = HbsEntry {
                key: k,
                kind: HBS_KIND_COUNT,
                str_val: "",
                bool_val: false,
                count_val: n,
            };
            _.len = _.len + 1;
        }
    }

    /// Return the number of bound entries in this context.
    pub fn size(): usize {
        return _.len;
    }
}

struct HbsBlockSpan {
    found: bool,
    has_else: bool,
    else_start: usize,
    else_end: usize,
    close_start: usize,
    close_end: usize,
}

fn hbsStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn hbsIsSpace(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 10 || ch == 13;
}

fn hbsTrim(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && hbsIsSpace(s[start])) {
        start = start + 1;
    }
    while (end > start && hbsIsSpace(s[end - 1])) {
        end = end - 1;
    }
    return s[start..end];
}

fn hbsMatchAt(s: string, pos: usize, end: usize, pat: string): bool {
    if (pos + pat.len > end) {
        return false;
    }
    let i: usize = 0;
    while (i < pat.len) {
        if (s[pos + i] != pat[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn hbsFindSubstr(s: string, start: usize, end: usize, pat: string): usize {
    if (pat.len == 0 || start + pat.len > end) {
        return end;
    }
    let i = start;
    const limit = end - pat.len;
    while (i <= limit) {
        if (hbsMatchAt(s, i, end, pat)) {
            return i;
        }
        i = i + 1;
    }
    return end;
}

fn hbsWriteRaw(out: []u8, pos: usize, s: string): usize {
    let w = pos;
    let i: usize = 0;
    while (i < s.len && w < out.len) {
        out[w] = s[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn hbsWriteText(out: []u8, pos: usize, s: string, escape: bool): usize {
    if (s.len == 0 || pos >= out.len) {
        return pos;
    }
    if (escape) {
        return pos + escapeHtml(s, out[pos..]);
    }
    return hbsWriteRaw(out, pos, s);
}

fn hbsWriteUsize(out: []u8, pos: usize, val: usize): usize {
    let digits: [24]u8 = undefined;
    let len: usize = 0;
    let n = val;
    if (n == 0) {
        if (pos < out.len) {
            out[pos] = 48 as u8;
            return pos + 1;
        }
        return pos;
    }
    while (n > 0 && len < 24) {
        digits[len] = (48 + (n % 10)) as u8;
        n = n / 10;
        len = len + 1;
    }
    let w = pos;
    let i = len;
    while (i > 0 && w < out.len) {
        i = i - 1;
        out[w] = digits[i];
        w = w + 1;
    }
    return w;
}

fn hbsWriteTransformed(out: []u8, pos: usize, s: string, to_upper: bool, escape: bool): usize {
    if (s.len == 0 || pos >= out.len) {
        return pos;
    }
    let chunk: [256]u8 = undefined;
    let w = pos;
    let offset: usize = 0;
    while (offset < s.len && w < out.len) {
        let n: usize = 0;
        while (n < 256 && offset < s.len) {
            let ch = s[offset];
            if (to_upper && ch >= 97 && ch <= 122) {
                ch = ch - 32;
            } else if (!to_upper && ch >= 65 && ch <= 90) {
                ch = ch + 32;
            }
            chunk[n] = ch;
            n = n + 1;
            offset = offset + 1;
        }
        w = hbsWriteText(out, w, chunk[0..n], escape);
    }
    return w;
}

fn hbsFindEntry(ctx: HbsContext, key: string): usize {
    let i: usize = 0;
    while (i < ctx.len) {
        if (hbsStrEq(ctx.entries[i].key, key)) {
            return i;
        }
        i = i + 1;
    }
    return HBS_MAX_ENTRIES;
}

fn hbsIsTruthy(ctx: HbsContext, key: string): bool {
    const idx = hbsFindEntry(ctx, key);
    if (idx >= HBS_MAX_ENTRIES) {
        return false;
    }
    const e = ctx.entries[idx];
    if (e.kind == HBS_KIND_STR) {
        return e.str_val.len > 0;
    }
    if (e.kind == HBS_KIND_BOOL) {
        return e.bool_val;
    }
    return e.count_val > 0;
}

fn hbsGetCount(ctx: HbsContext, key: string): usize {
    const idx = hbsFindEntry(ctx, key);
    if (idx >= HBS_MAX_ENTRIES) {
        return 0;
    }
    const e = ctx.entries[idx];
    if (e.kind == HBS_KIND_COUNT) {
        return e.count_val;
    }
    if (e.kind == HBS_KIND_BOOL) {
        return if (e.bool_val) 1 else 0;
    }
    return 0;
}

fn hbsWriteEntry(ctx: HbsContext, idx: usize, escape: bool, out: []u8, pos: usize): usize {
    if (idx >= HBS_MAX_ENTRIES) {
        return pos;
    }
    const e = ctx.entries[idx];
    if (e.kind == HBS_KIND_STR) {
        return hbsWriteText(out, pos, e.str_val, escape);
    }
    if (e.kind == HBS_KIND_BOOL) {
        const lit = if (e.bool_val) "true" else "false";
        return hbsWriteRaw(out, pos, lit);
    }
    return hbsWriteUsize(out, pos, e.count_val);
}

fn hbsStripQuotes(s: string): string {
    if (s.len >= 2) {
        const first = s[0];
        const last = s[s.len - 1];
        if ((first == 34 && last == 34) || (first == 39 && last == 39)) {
            return s[1..(s.len - 1)];
        }
    }
    return s;
}

fn hbsFindSpace(s: string, start: usize): usize {
    let i = start;
    while (i < s.len) {
        if (hbsIsSpace(s[i])) {
            return i;
        }
        i = i + 1;
    }
    return s.len;
}

fn hbsEvalInline(expr_raw: string, ctx: HbsContext, escape: bool, out: []u8, pos: usize): usize {
    const expr = hbsTrim(expr_raw);
    if (expr.len == 0) {
        return pos;
    }

    const sp1 = hbsFindSpace(expr, 0);
    if (sp1 < expr.len) {
        const helper = expr[0..sp1];
        const rest = hbsTrim(expr[sp1..expr.len]);

        if (hbsStrEq(helper, "upper")) {
            const idx = hbsFindEntry(ctx, rest);
            if (idx < HBS_MAX_ENTRIES && ctx.entries[idx].kind == HBS_KIND_STR) {
                return hbsWriteTransformed(out, pos, ctx.entries[idx].str_val, true, escape);
            }
            return hbsWriteEntry(ctx, idx, escape, out, pos);
        }

        if (hbsStrEq(helper, "lower")) {
            const idx = hbsFindEntry(ctx, rest);
            if (idx < HBS_MAX_ENTRIES && ctx.entries[idx].kind == HBS_KIND_STR) {
                return hbsWriteTransformed(out, pos, ctx.entries[idx].str_val, false, escape);
            }
            return hbsWriteEntry(ctx, idx, escape, out, pos);
        }

        if (hbsStrEq(helper, "default")) {
            const sp2 = hbsFindSpace(rest, 0);
            const key = rest[0..sp2];
            const fallback = if (sp2 < rest.len) hbsStripQuotes(hbsTrim(rest[sp2..rest.len])) else "";
            const idx = hbsFindEntry(ctx, key);
            if (idx < HBS_MAX_ENTRIES) {
                const e = ctx.entries[idx];
                if (e.kind == HBS_KIND_STR) {
                    if (e.str_val.len > 0) {
                        return hbsWriteText(out, pos, e.str_val, escape);
                    }
                    return hbsWriteText(out, pos, fallback, escape);
                }
                return hbsWriteEntry(ctx, idx, escape, out, pos);
            }
            return hbsWriteText(out, pos, fallback, escape);
        }
    }

    const idx = hbsFindEntry(ctx, expr);
    return hbsWriteEntry(ctx, idx, escape, out, pos);
}

fn hbsFindBlockBounds(tpl: string, body_start: usize, end: usize, block_tag: string): HbsBlockSpan {
    let nest: usize = 0;
    let has_else = false;
    let else_start: usize = 0;
    let else_end: usize = 0;

    let i = body_start;
    while (i < end) {
        const open = hbsFindSubstr(tpl, i, end, "{{");
        if (open >= end) {
            break;
        }

        // Skip block comment {{!-- ... --}}
        if (hbsMatchAt(tpl, open, end, "{{!--")) {
            const c_end = hbsFindSubstr(tpl, open + 5, end, "--}}");
            i = if (c_end < end) c_end + 4 else end;
            continue;
        }

        // Skip triple-stash {{{ ... }}}
        if (hbsMatchAt(tpl, open, end, "{{{")) {
            const t_end = hbsFindSubstr(tpl, open + 3, end, "}}}");
            i = if (t_end < end) t_end + 3 else end;
            continue;
        }

        const close = hbsFindSubstr(tpl, open + 2, end, "}}");
        if (close >= end) {
            break;
        }

        const inner = hbsTrim(tpl[(open + 2)..close]);
        if (inner.len > 0 && inner[0] == (0x21 as u8)) { // '!' comment
            i = close + 2;
            continue;
        }

        if (inner.len > 0 && inner[0] == (0x23 as u8)) { // '#' nested open block
            nest = nest + 1;
            i = close + 2;
            continue;
        }

        if (inner.len > 0 && inner[0] == (0x2F as u8)) { // '/' close block
            const close_tag = hbsTrim(inner[1..inner.len]);
            if (nest == 0) {
                if (hbsStrEq(close_tag, block_tag)) {
                    return HbsBlockSpan {
                        found: true,
                        has_else: has_else,
                        else_start: else_start,
                        else_end: else_end,
                        close_start: open,
                        close_end: close + 2,
                    };
                }
                i = close + 2;
                continue;
            }
            nest = nest - 1;
            i = close + 2;
            continue;
        }

        if (nest == 0 && !has_else && hbsStrEq(inner, "else")) {
            has_else = true;
            else_start = open;
            else_end = close + 2;
        }

        i = close + 2;
    }

    return HbsBlockSpan {
        found: false,
        has_else: false,
        else_start: 0,
        else_end: 0,
        close_start: end,
        close_end: end,
    };
}

fn hbsRenderRange(
    tpl: string,
    start: usize,
    end: usize,
    ctx: HbsContext,
    out: []u8,
    pos: usize,
    depth: usize
): usize {
    if (depth >= HBS_MAX_DEPTH || start >= end || pos >= out.len) {
        return pos;
    }

    let w = pos;
    let i = start;
    while (i < end && w < out.len) {
        const open = hbsFindSubstr(tpl, i, end, "{{");
        if (open >= end) {
            w = hbsWriteRaw(out, w, tpl[i..end]);
            break;
        }

        if (open > i) {
            w = hbsWriteRaw(out, w, tpl[i..open]);
        }

        // 1. Block comment {{!-- ... --}}
        if (hbsMatchAt(tpl, open, end, "{{!--")) {
            const c_end = hbsFindSubstr(tpl, open + 5, end, "--}}");
            i = if (c_end < end) c_end + 4 else end;
            continue;
        }

        // 2. Unescaped triple-stash {{{ ... }}}
        if (hbsMatchAt(tpl, open, end, "{{{")) {
            const t_end = hbsFindSubstr(tpl, open + 3, end, "}}}");
            if (t_end >= end) {
                w = hbsWriteRaw(out, w, tpl[open..end]);
                break;
            }
            w = hbsEvalInline(tpl[(open + 3)..t_end], ctx, false, out, w);
            i = t_end + 3;
            continue;
        }

        const close = hbsFindSubstr(tpl, open + 2, end, "}}");
        if (close >= end) {
            w = hbsWriteRaw(out, w, tpl[open..end]);
            break;
        }

        const inner = hbsTrim(tpl[(open + 2)..close]);
        if (inner.len == 0) {
            i = close + 2;
            continue;
        }

        // 3. Inline comment {{! ... }}
        if (inner[0] == (0x21 as u8)) { // '!'
            i = close + 2;
            continue;
        }

        // 4. Block helpers: {{#if key}}, {{#unless key}}, {{#repeat key}}
        if (inner[0] == (0x23 as u8)) { // '#'
            const header = hbsTrim(inner[1..inner.len]);
            const sp = hbsFindSpace(header, 0);
            const tag = header[0..sp];
            const arg = if (sp < header.len) hbsTrim(header[sp..header.len]) else "";

            const body_start = close + 2;
            const span = hbsFindBlockBounds(tpl, body_start, end, tag);
            if (!span.found) {
                i = body_start;
                continue;
            }

            if (hbsStrEq(tag, "if")) {
                if (hbsIsTruthy(ctx, arg)) {
                    const branch_end = if (span.has_else) span.else_start else span.close_start;
                    w = hbsRenderRange(tpl, body_start, branch_end, ctx, out, w, depth + 1);
                } else if (span.has_else) {
                    w = hbsRenderRange(tpl, span.else_end, span.close_start, ctx, out, w, depth + 1);
                }
            } else if (hbsStrEq(tag, "unless")) {
                if (!hbsIsTruthy(ctx, arg)) {
                    const branch_end = if (span.has_else) span.else_start else span.close_start;
                    w = hbsRenderRange(tpl, body_start, branch_end, ctx, out, w, depth + 1);
                } else if (span.has_else) {
                    w = hbsRenderRange(tpl, span.else_end, span.close_start, ctx, out, w, depth + 1);
                }
            } else if (hbsStrEq(tag, "repeat")) {
                const count = hbsGetCount(ctx, arg);
                if (count > 0) {
                    const branch_end = if (span.has_else) span.else_start else span.close_start;
                    let r: usize = 0;
                    while (r < count && w < out.len) {
                        w = hbsRenderRange(tpl, body_start, branch_end, ctx, out, w, depth + 1);
                        r = r + 1;
                    }
                } else if (span.has_else) {
                    w = hbsRenderRange(tpl, span.else_end, span.close_start, ctx, out, w, depth + 1);
                }
            }

            i = span.close_end;
            continue;
        }

        // 5. Standard escaped interpolation or built-in inline helper
        w = hbsEvalInline(inner, ctx, true, out, w);
        i = close + 2;
    }

    return w;
}

/// Render Handlebars template `tpl` with `ctx` into caller-provided `out` buffer
/// with zero heap allocations. Returns total bytes written into `out`.
pub fn renderHandlebars(tpl: string, ctx: HbsContext, out: []u8): usize {
    if (tpl.len == 0 || out.len == 0) {
        return 0;
    }
    return hbsRenderRange(tpl, 0, tpl.len, ctx, out, 0, 0);
}

export {
    HbsContext,
    renderHandlebars,
};
