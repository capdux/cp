// @cap/mustache-tpl — Zero-heap Mustache v1.4 logic-less template engine (mustache.js v4 parity)
// Clean-room MIT CP implementation of npm `mustache` (60M+/wk).
// Reuses:
//   - `escapeHtml` from `@cap/escape-html` for XSS-safe `{{var}}` interpolation.

import { escapeHtml } from "../../escape-html/src/index.cp";

pub const MST_VAL_STR: u8 = 1;
pub const MST_VAL_BOOL: u8 = 2;
pub const MST_VAL_LIST: u8 = 3;
pub const MST_MAX_ENTRIES: usize = 32;

fn mstIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn mstStrEq(a: []const u8, b: string): bool {
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

fn mstTrimSlice(s: string, out_start: *usize, out_end: *usize) {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && mstIsSpace(s[a])) {
        a = a + 1;
    }
    while (b > a && mstIsSpace(s[b - 1])) {
        b = b - 1;
    }
    out_start.* = a;
    out_end.* = b;
}

fn mstCopyBytes(src: string, dst: []u8): usize {
    const n = if (src.len < dst.len) { src.len } else { dst.len };
    let i: usize = 0;
    while (i < n) {
        dst[i] = src[i];
        i = i + 1;
    }
    return n;
}

fn mstAppendSlice(out: []u8, pos: *usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len && pos.* < out.len) {
        out[pos.*] = src[i];
        pos.* = pos.* + 1;
        i = i + 1;
    }
}

fn mstAppendByte(out: []u8, pos: *usize, b: u8) {
    if (pos.* < out.len) {
        out[pos.*] = b;
        pos.* = pos.* + 1;
    }
}

fn mstFindSubstr2(s: string, start_pos: usize, c0: u8, c1: u8): i64 {
    let i = start_pos;
    while (i + 1 < s.len) {
        if (s[i] == c0 && s[i + 1] == c1) {
            return i as i64;
        }
        i = i + 1;
    }
    return -1;
}

fn mstFindSubstr3(s: string, start_pos: usize, c0: u8, c1: u8, c2: u8): i64 {
    let i = start_pos;
    while (i + 2 < s.len) {
        if (s[i] == c0 && s[i + 1] == c1 && s[i + 2] == c2) {
            return i as i64;
        }
        i = i + 1;
    }
    return -1;
}

pub struct MustacheEntry {
    pub key: [48]u8,
    pub key_len: usize,
    pub val_kind: u8,
    pub str_val: [128]u8,
    pub str_len: usize,
    pub bool_val: bool,
    pub list_count: usize,

    pub static fn empty(): MustacheEntry {
        return MustacheEntry {
            key: [0; 48],
            key_len: 0,
            val_kind: 0,
            str_val: [0; 128],
            str_len: 0,
            bool_val: false,
            list_count: 0,
        };
    }
}

pub struct MustacheContext {
    pub entries: [32]MustacheEntry,
    pub count: usize,

    pub static fn new(): MustacheContext {
        let entries: [32]MustacheEntry = undefined;
        let i: usize = 0;
        while (i < MST_MAX_ENTRIES) {
            entries[i] = MustacheEntry.empty();
            i = i + 1;
        }
        return MustacheContext {
            entries: entries,
            count: 0,
        };
    }

    pub fn entryCount(): usize {
        return _.count;
    }

    pub fn setStr(*_, key: string, val: string): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (mstStrEq(_.entries[i].key[0.._.entries[i].key_len], key)) {
                _.entries[i].val_kind = MST_VAL_STR;
                _.entries[i].str_len = mstCopyBytes(val, _.entries[i].str_val[0..128]);
                _.entries[i].bool_val = false;
                _.entries[i].list_count = 0;
                return true;
            }
            i = i + 1;
        }
        if (_.count >= MST_MAX_ENTRIES) {
            return false;
        }
        let e = MustacheEntry.empty();
        e.key_len = mstCopyBytes(key, e.key[0..48]);
        e.val_kind = MST_VAL_STR;
        e.str_len = mstCopyBytes(val, e.str_val[0..128]);
        _.entries[_.count] = e;
        _.count = _.count + 1;
        return true;
    }

    pub fn setBool(*_, key: string, val: bool): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (mstStrEq(_.entries[i].key[0.._.entries[i].key_len], key)) {
                _.entries[i].val_kind = MST_VAL_BOOL;
                _.entries[i].bool_val = val;
                _.entries[i].str_len = 0;
                _.entries[i].list_count = 0;
                return true;
            }
            i = i + 1;
        }
        if (_.count >= MST_MAX_ENTRIES) {
            return false;
        }
        let e = MustacheEntry.empty();
        e.key_len = mstCopyBytes(key, e.key[0..48]);
        e.val_kind = MST_VAL_BOOL;
        e.bool_val = val;
        _.entries[_.count] = e;
        _.count = _.count + 1;
        return true;
    }

    pub fn setListCount(*_, key: string, list_count: usize): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (mstStrEq(_.entries[i].key[0.._.entries[i].key_len], key)) {
                _.entries[i].val_kind = MST_VAL_LIST;
                _.entries[i].list_count = list_count;
                _.entries[i].str_len = 0;
                _.entries[i].bool_val = false;
                return true;
            }
            i = i + 1;
        }
        if (_.count >= MST_MAX_ENTRIES) {
            return false;
        }
        let e = MustacheEntry.empty();
        e.key_len = mstCopyBytes(key, e.key[0..48]);
        e.val_kind = MST_VAL_LIST;
        e.list_count = list_count;
        _.entries[_.count] = e;
        _.count = _.count + 1;
        return true;
    }
}

fn mstLookup(ctx: MustacheContext, key: string): i32 {
    let i: usize = 0;
    while (i < ctx.count) {
        if (mstStrEq(ctx.entries[i].key[0..ctx.entries[i].key_len], key)) {
            return i as i32;
        }
        i = i + 1;
    }
    return -1;
}

fn mstEmitValue(ctx: MustacheContext, key: string, escaped: bool, out: []u8, pos: *usize) {
    const idx = mstLookup(ctx, key);
    if (idx < 0) {
        return;
    }
    const e = ctx.entries[idx as usize];
    if (e.val_kind == MST_VAL_STR) {
        const s: string = e.str_val[0..e.str_len];
        if (escaped) {
            if (pos.* < out.len) {
                const written = escapeHtml(s, out[pos.*..out.len]);
                pos.* = pos.* + written;
            }
        } else {
            mstAppendSlice(out, pos, s);
        }
    } else if (e.val_kind == MST_VAL_BOOL) {
        if (e.bool_val) {
            mstAppendSlice(out, pos, "true");
        } else {
            mstAppendSlice(out, pos, "false");
        }
    }
}

/// Finds the matching `{{/key}}` closing tag starting at `from`, respecting nested `{{#key}}`/`{{^key}}`.
/// Writes body end offset into `out_body_end` and offset after closing `}}` into `out_after_close`.
fn mstFindSectionClose(
    tpl: string,
    start_pos: usize,
    section_key: string,
    out_body_end: *usize,
    out_after_close: *usize,
): bool {
    let i = start_pos;
    let nesting: usize = 1;
    while (i < tpl.len) {
        const open_pos = mstFindSubstr2(tpl, i, 123, 123);
        if (open_pos < 0) {
            return false;
        }
        const op = open_pos as usize;
        // Skip triple mustache {{{...}}}
        if (op + 2 < tpl.len && tpl[op + 2] == 123) {
            const close3 = mstFindSubstr3(tpl, op + 3, 125, 125, 125);
            if (close3 < 0) {
                return false;
            }
            i = (close3 as usize) + 3;
            continue;
        }
        const close_pos = mstFindSubstr2(tpl, op + 2, 125, 125);
        if (close_pos < 0) {
            return false;
        }
        const cp = close_pos as usize;
        let t0: usize = 0;
        let t1: usize = 0;
        mstTrimSlice(tpl[op + 2..cp], &t0, &t1);
        const inner = tpl[op + 2 + t0..op + 2 + t1];
        if (inner.len > 1) {
            const sigil = inner[0];
            if (sigil == 35 || sigil == 94 || sigil == 47) { // '#' | '^' | '/'
                let k0: usize = 0;
                let k1: usize = 0;
                mstTrimSlice(inner[1..inner.len], &k0, &k1);
                const tag_key = inner[1 + k0..1 + k1];
                if (mstStrEq(tag_key, section_key)) {
                    if (sigil == 35 || sigil == 94) {
                        nesting = nesting + 1;
                    } else {
                        nesting = nesting - 1;
                        if (nesting == 0) {
                            out_body_end.* = op;
                            out_after_close.* = cp + 2;
                            return true;
                        }
                    }
                }
            }
        }
        i = cp + 2;
    }
    return false;
}

fn mstRenderSlice(tpl: string, ctx: MustacheContext, out: []u8, pos: *usize, depth: usize) {
    if (depth > 8 || pos.* >= out.len) {
        return;
    }
    let i: usize = 0;
    while (i < tpl.len && pos.* < out.len) {
        if (i + 1 < tpl.len && tpl[i] == 123 && tpl[i + 1] == 123) {
            // Triple mustache {{{ key }}} (raw unescaped)
            if (i + 2 < tpl.len && tpl[i + 2] == 123) {
                const close3 = mstFindSubstr3(tpl, i + 3, 125, 125, 125);
                if (close3 < 0) {
                    mstAppendSlice(out, pos, tpl[i..tpl.len]);
                    return;
                }
                const c3 = close3 as usize;
                let k0: usize = 0;
                let k1: usize = 0;
                mstTrimSlice(tpl[i + 3..c3], &k0, &k1);
                mstEmitValue(ctx, tpl[i + 3 + k0..i + 3 + k1], false, out, pos);
                i = c3 + 3;
                continue;
            }

            // Double mustache {{ ... }}
            const close2 = mstFindSubstr2(tpl, i + 2, 125, 125);
            if (close2 < 0) {
                // Unclosed tag: emit verbatim safely
                mstAppendSlice(out, pos, tpl[i..tpl.len]);
                return;
            }
            const c2 = close2 as usize;
            let t0: usize = 0;
            let t1: usize = 0;
            mstTrimSlice(tpl[i + 2..c2], &t0, &t1);
            const inner = tpl[i + 2 + t0..i + 2 + t1];
            i = c2 + 2;

            if (inner.len == 0) {
                continue;
            }
            const sigil = inner[0];
            if (sigil == 33) { // '!' comment
                continue;
            }
            if (sigil == 38) { // '&' unescaped variable
                let k0: usize = 0;
                let k1: usize = 0;
                mstTrimSlice(inner[1..inner.len], &k0, &k1);
                mstEmitValue(ctx, inner[1 + k0..1 + k1], false, out, pos);
                continue;
            }
            if (sigil == 47) { // stray closing tag '/'
                continue;
            }
            if (sigil == 35 || sigil == 94) { // '#' truthy section or '^' inverted section
                let k0: usize = 0;
                let k1: usize = 0;
                mstTrimSlice(inner[1..inner.len], &k0, &k1);
                const key = inner[1 + k0..1 + k1];

                let body_end: usize = 0;
                let after_close: usize = 0;
                if (!mstFindSectionClose(tpl, i, key, &body_end, &after_close)) {
                    // Unclosed section: safely skip to end without panic
                    continue;
                }
                const body = tpl[i..body_end];
                i = after_close;

                const idx = mstLookup(ctx, key);
                let repeat_count: usize = 0;
                if (sigil == 35) {
                    if (idx >= 0) {
                        const e = ctx.entries[idx as usize];
                        if (e.val_kind == MST_VAL_STR) {
                            if (e.str_len > 0) { repeat_count = 1; }
                        } else if (e.val_kind == MST_VAL_BOOL) {
                            if (e.bool_val) { repeat_count = 1; }
                        } else if (e.val_kind == MST_VAL_LIST) {
                            repeat_count = e.list_count;
                        }
                    }
                } else {
                    // Inverted section '^'
                    if (idx < 0) {
                        repeat_count = 1;
                    } else {
                        const e = ctx.entries[idx as usize];
                        if (e.val_kind == MST_VAL_STR) {
                            if (e.str_len == 0) { repeat_count = 1; }
                        } else if (e.val_kind == MST_VAL_BOOL) {
                            if (!e.bool_val) { repeat_count = 1; }
                        } else if (e.val_kind == MST_VAL_LIST) {
                            if (e.list_count == 0) { repeat_count = 1; }
                        }
                    }
                }

                let r: usize = 0;
                while (r < repeat_count && pos.* < out.len) {
                    mstRenderSlice(body, ctx, out, pos, depth + 1);
                    r = r + 1;
                }
                continue;
            }

            // Standard HTML-escaped interpolation {{ key }}
            mstEmitValue(ctx, inner, true, out, pos);
            continue;
        }

        mstAppendByte(out, pos, tpl[i]);
        i = i + 1;
    }
}

/// Renders a Mustache v1.4 `template` with `ctx` into `out` (zero-heap, safe truncation).
/// Returns the number of bytes written to `out`.
pub fn renderMustache(template: string, ctx: MustacheContext, out: []u8): usize {
    if (template.len == 0 || out.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    mstRenderSlice(template, ctx, out, &pos, 0);
    return pos;
}

export {
    MST_VAL_STR,
    MST_VAL_BOOL,
    MST_VAL_LIST,
    MST_MAX_ENTRIES,
    MustacheEntry,
    MustacheContext,
    renderMustache,
};
