// @cap/ejs-tpl — Zero-heap Embedded Template Engine modeled on npm `ejs` v3.
// Supports `<%= expr %>` (HTML-escaped via `@cap/escape-html`), `<%- expr %>` (raw),
// `<%# comment %>`, `<% if key %>...<% else %>...<% end %>`, `<% repeat key %>...<% end %>`,
// `<% unless key %>...<% end %>`, `<%%` literal escape, and whitespace slurping `<%_ ... _%>` / `-%>`.

import { strEq, startsWith, trim } from "cap:ascii";
import { escapeHtml } from "../../escape-html/src/index.cp";

pub const EJS_MAX_ENTRIES: usize = 32;

pub struct EjsEntry {
    pub key: string,
    pub str_val: string,
    pub bool_val: bool,
    pub count_val: usize,
    pub is_count: bool,
}

pub struct EjsContext {
    pub entries: [32]EjsEntry,
    pub count: usize,

    pub static fn new(): EjsContext {
        let empty = EjsEntry { key: "", str_val: "", bool_val: false, count_val: 0, is_count: false };
        let arr: [32]EjsEntry = undefined;
        let i: usize = 0;
        while (i < 32) {
            arr[i] = empty;
            i = i + 1;
        }
        return EjsContext {
            entries: arr,
            count: 0,
        };
    }

    pub fn setStr(*_, key: string, val: string): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].key, key)) {
                _.entries[i].str_val = val;
                _.entries[i].bool_val = val.len > 0;
                _.entries[i].is_count = false;
                return true;
            }
            i = i + 1;
        }
        if (_.count >= EJS_MAX_ENTRIES) { return false; }
        _.entries[_.count] = EjsEntry {
            key: key,
            str_val: val,
            bool_val: val.len > 0,
            count_val: 0,
            is_count: false,
        };
        _.count = _.count + 1;
        return true;
    }

    pub fn setBool(*_, key: string, val: bool): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].key, key)) {
                _.entries[i].bool_val = val;
                _.entries[i].str_val = if (val) { "true" } else { "false" };
                _.entries[i].is_count = false;
                return true;
            }
            i = i + 1;
        }
        if (_.count >= EJS_MAX_ENTRIES) { return false; }
        _.entries[_.count] = EjsEntry {
            key: key,
            str_val: if (val) { "true" } else { "false" },
            bool_val: val,
            count_val: 0,
            is_count: false,
        };
        _.count = _.count + 1;
        return true;
    }

    pub fn setRepeatCount(*_, key: string, n: usize): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].key, key)) {
                _.entries[i].count_val = n;
                _.entries[i].bool_val = n > 0;
                _.entries[i].is_count = true;
                return true;
            }
            i = i + 1;
        }
        if (_.count >= EJS_MAX_ENTRIES) { return false; }
        _.entries[_.count] = EjsEntry {
            key: key,
            str_val: "",
            bool_val: n > 0,
            count_val: n,
            is_count: true,
        };
        _.count = _.count + 1;
        return true;
    }

    pub fn size(): usize {
        return _.count;
    }
}

fn ejsWriteStr(s: string, out: []u8, pos: usize): usize {
    if (pos >= out.len || s.len == 0) { return pos; }
    const avail = out.len - pos;
    const n = if (s.len < avail) s.len else avail;
    @memcpy(out[pos .. pos + n], s[0..n]);
    return pos + n;
}

fn ejsLookupStr(ctx: EjsContext, key: string): string {
    let i: usize = 0;
    while (i < ctx.count) {
        if (strEq(ctx.entries[i].key, key)) {
            return ctx.entries[i].str_val;
        }
        i = i + 1;
    }
    return "";
}

fn ejsLookupBool(ctx: EjsContext, key: string): bool {
    let i: usize = 0;
    while (i < ctx.count) {
        if (strEq(ctx.entries[i].key, key)) {
            return ctx.entries[i].bool_val;
        }
        i = i + 1;
    }
    return false;
}

fn ejsLookupCount(ctx: EjsContext, key: string): usize {
    let i: usize = 0;
    while (i < ctx.count) {
        if (strEq(ctx.entries[i].key, key)) {
            if (ctx.entries[i].is_count) { return ctx.entries[i].count_val; }
            return if (ctx.entries[i].bool_val) { 1 } else { 0 };
        }
        i = i + 1;
    }
    return 0;
}

fn ejsFindTagOpen(tpl: string, start_pos: usize): usize {
    let i = start_pos;
    while (i + 1 < tpl.len) {
        if (tpl[i] == 60 && tpl[i + 1] == 37) { // "<%"
            return i;
        }
        i = i + 1;
    }
    return tpl.len;
}

fn ejsFindTagClose(tpl: string, start_pos: usize): usize {
    let i = start_pos;
    while (i + 1 < tpl.len) {
        if (tpl[i] == 37 && tpl[i + 1] == 62) { // "%>"
            return i;
        }
        i = i + 1;
    }
    return tpl.len;
}

// Find matching `<% end %>` (and optional top-level `<% else %>`) starting at `body_start`.
// Writes `else_pos` (or `tpl.len` if none) into `out_pos[0]` and `end_tag_start` into `out_pos[1]`,
// and `after_end` into `out_pos[2]`.
fn ejsFindBlockBounds(tpl: string, body_start: usize, out_pos: []usize): bool {
    out_pos[0] = tpl.len;
    out_pos[1] = tpl.len;
    out_pos[2] = tpl.len;
    let depth: usize = 1;
    let pos = body_start;
    while (pos < tpl.len) {
        let open = ejsFindTagOpen(tpl, pos);
        if (open >= tpl.len) { return false; }
        if (open + 2 < tpl.len && tpl[open + 2] == 37) { // "<%%" literal
            pos = open + 3;
            continue;
        }
        let close = ejsFindTagClose(tpl, open + 2);
        if (close >= tpl.len) { return false; }
        let inner_start = open + 2;
        let inner_end = close;
        if (inner_start < inner_end && (tpl[inner_start] == 95)) { inner_start = inner_start + 1; }
        if (inner_end > inner_start && (tpl[inner_end - 1] == 45 || tpl[inner_end - 1] == 95)) {
            inner_end = inner_end - 1;
        }
        let inner = trim(tpl[inner_start..inner_end]);
        if (startsWith(inner, "if ") || startsWith(inner, "unless ") || startsWith(inner, "repeat ")) {
            depth = depth + 1;
        } else if (strEq(inner, "else")) {
            if (depth == 1 && out_pos[0] == tpl.len) {
                out_pos[0] = open;
            }
        } else if (strEq(inner, "end")) {
            depth = depth - 1;
            if (depth == 0) {
                out_pos[1] = open;
                out_pos[2] = close + 2;
                return true;
            }
        }
        pos = close + 2;
    }
    return false;
}

fn ejsRenderSlice(tpl: string, ctx: EjsContext, out: []u8, start_out: usize, depth: usize): usize {
    if (depth > 16) { return start_out; }
    let p = start_out;
    let pos: usize = 0;
    while (pos < tpl.len) {
        let open = ejsFindTagOpen(tpl, pos);
        if (open >= tpl.len) {
            p = ejsWriteStr(tpl[pos..tpl.len], out, p);
            break;
        }
        // Check `<%_`: slurp trailing horizontal whitespace before tag
        let text_end = open;
        let slurp_before = (open + 2 < tpl.len && tpl[open + 2] == 95);
        if (slurp_before) {
            while (text_end > pos && (tpl[text_end - 1] == 32 || tpl[text_end - 1] == 9)) {
                text_end = text_end - 1;
            }
        }
        if (text_end > pos) {
            p = ejsWriteStr(tpl[pos..text_end], out, p);
        }
        // Literal `<%%` -> `<%`
        if (open + 2 < tpl.len && tpl[open + 2] == 37) {
            p = ejsWriteStr("<%", out, p);
            pos = open + 3;
            continue;
        }
        let close = ejsFindTagClose(tpl, open + 2);
        if (close >= tpl.len) {
            // Unclosed tag: emit literally
            p = ejsWriteStr(tpl[open..tpl.len], out, p);
            break;
        }
        let tag_start = open + 2;
        let tag_end = close;
        let slurp_after_nl = false;
        let slurp_after_ws = false;
        if (tag_end > tag_start && tpl[tag_end - 1] == 45) { // "-%>"
            slurp_after_nl = true;
            tag_end = tag_end - 1;
        } else if (tag_end > tag_start && tpl[tag_end - 1] == 95) { // "_%>"
            slurp_after_ws = true;
            tag_end = tag_end - 1;
        }

        let next_pos = close + 2;
        if (slurp_after_nl) {
            if (next_pos < tpl.len && tpl[next_pos] == 13) { next_pos = next_pos + 1; }
            if (next_pos < tpl.len && tpl[next_pos] == 10) { next_pos = next_pos + 1; }
        } else if (slurp_after_ws) {
            while (next_pos < tpl.len && (tpl[next_pos] == 32 || tpl[next_pos] == 9)) {
                next_pos = next_pos + 1;
            }
        }

        if (tag_start < tag_end) {
            let lead = tpl[tag_start];
            if (lead == 61) { // "<%=" HTML-escaped output
                let key = trim(tpl[tag_start + 1..tag_end]);
                let val = ejsLookupStr(ctx, key);
                if (p < out.len) {
                    let written = escapeHtml(val, out[p..out.len]);
                    p = p + written;
                }
                pos = next_pos;
                continue;
            }
            if (lead == 45) { // "<%-" raw unescaped output
                let key = trim(tpl[tag_start + 1..tag_end]);
                let val = ejsLookupStr(ctx, key);
                p = ejsWriteStr(val, out, p);
                pos = next_pos;
                continue;
            }
            if (lead == 35) { // "<%#" comment
                pos = next_pos;
                continue;
            }
            if (lead == 95) { // "<%_" control tag with slurp
                tag_start = tag_start + 1;
            }
        }

        let cmd = trim(tpl[tag_start..tag_end]);
        if (startsWith(cmd, "if ") || startsWith(cmd, "unless ") || startsWith(cmd, "repeat ")) {
            let bounds: [3]usize = undefined;
            if (ejsFindBlockBounds(tpl, next_pos, bounds[0..3])) {
                let else_open = bounds[0];
                let end_open = bounds[1];
                let after_end = bounds[2];
                let true_end = if (else_open < end_open) { else_open } else { end_open };
                let true_body = tpl[next_pos..true_end];
                let false_body: string = "";
                if (else_open < end_open) {
                    let else_close = ejsFindTagClose(tpl, else_open + 2);
                    if (else_close + 2 <= end_open) {
                        false_body = tpl[else_close + 2..end_open];
                    }
                }
                if (startsWith(cmd, "if ")) {
                    let key = trim(cmd[3..cmd.len]);
                    if (ejsLookupBool(ctx, key)) {
                        p = ejsRenderSlice(true_body, ctx, out, p, depth + 1);
                    } else if (false_body.len > 0) {
                        p = ejsRenderSlice(false_body, ctx, out, p, depth + 1);
                    }
                } else if (startsWith(cmd, "unless ")) {
                    let key = trim(cmd[7..cmd.len]);
                    if (!ejsLookupBool(ctx, key)) {
                        p = ejsRenderSlice(true_body, ctx, out, p, depth + 1);
                    } else if (false_body.len > 0) {
                        p = ejsRenderSlice(false_body, ctx, out, p, depth + 1);
                    }
                } else {
                    let key = trim(cmd[7..cmd.len]);
                    let times = ejsLookupCount(ctx, key);
                    let r: usize = 0;
                    while (r < times) {
                        p = ejsRenderSlice(true_body, ctx, out, p, depth + 1);
                        r = r + 1;
                    }
                }
                pos = after_end;
                continue;
            }
        }
        pos = next_pos;
    }
    return p;
}

/// Renders an EJS template with `ctx` into `out`, returning total bytes written.
pub fn renderEjs(tpl: string, ctx: EjsContext, out: []u8): usize {
    return ejsRenderSlice(tpl, ctx, out, 0, 0);
}
