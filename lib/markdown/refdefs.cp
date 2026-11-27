// Source: Absorbed from /data/dev/bun/src/md/ref_defs.cp | Synced: 2026-11-23T22:00:12Z
// Pure-CP CommonMark §4.7 Link Reference Definition parser, label normalizer,
// destination/title parsers, and first-definition-wins lookup table.

pub struct RefDef {
    /// Raw or normalized label slice.
    label: []const u8,
    /// Raw destination slice from source.
    dest: []const u8,
    /// Raw title slice from source (empty if omitted).
    title: []const u8,
}

pub struct ParseRefDefResult {
    end_pos: usize,
    label: []const u8,
    dest: []const u8,
    title: []const u8,
}

pub struct ParseDestResult {
    dest: []const u8,
    end_pos: usize,
}

pub struct ParseTitleResult {
    title: []const u8,
    end_pos: usize,
}

type RefDefVec16 = @Vector(16, u8);

fn makeRefDefWsLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' '] = 1;
    lut['\t'] = 1;
    lut['\n'] = 1;
    lut['\r'] = 1;
    return lut;
}

fn makeRefDefLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

const REFDEF_WS_LUT: [u8; 256] = makeRefDefWsLut();
const REFDEF_LOWER_LUT: [u8; 256] = makeRefDefLowerLut();

/// Normalize a CommonMark link label into caller-provided `out` buffer:
/// strip leading/trailing ASCII whitespace, collapse internal whitespace runs
/// (including newlines) to a single space, and fold ASCII uppercase to lowercase.
/// Returns the normalized slice, or null if `out` is too small.
pub fn normalizeLabel(raw: []const u8, out: []u8): ([]const u8)? {
    let out_len: usize = 0;
    let in_ws = true;
    let i: usize = 0;
    const vsp: RefDefVec16 = @splat(' ' as u8);
    const vtb: RefDefVec16 = @splat('\t' as u8);
    const vnl: RefDefVec16 = @splat('\n' as u8);
    const vcr: RefDefVec16 = @splat('\r' as u8);
    const vA: RefDefVec16 = @splat('A' as u8);
    const vZ: RefDefVec16 = @splat('Z' as u8);
    const v20: RefDefVec16 = @splat(0x20 as u8);
    while (i < raw.len) {
        if (i + 16 <= raw.len && out_len + 16 <= out.len) {
            const v: RefDefVec16 = raw[i .. i + 16][0..16].*;
            const has_ws = (v == vsp) | (v == vtb) | (v == vnl) | (v == vcr);
            if (!@reduce(.Or, has_ws)) {
                const is_up = (v >= vA) & (v <= vZ);
                const lower = @select(u8, is_up, v | v20, v);
                const arr: [16]u8 = lower;
                @memcpy(out[out_len .. out_len + 16], arr[0..16]);
                out_len += 16;
                i += 16;
                in_ws = false;
                continue;
            }
        }
        const c = raw[i];
        if (REFDEF_WS_LUT[usize(c)] != 0) {
            if (!in_ws && out_len > 0) {
                if (out_len >= out.len) return null;
                out[out_len] = ' ';
                out_len += 1;
                in_ws = true;
            }
        } else {
            if (out_len >= out.len) return null;
            out[out_len] = REFDEF_LOWER_LUT[usize(c)];
            out_len += 1;
            in_ws = false;
        }
        i += 1;
    }
    if (out_len > 0 && out[out_len - 1] == ' ') {
        out_len -= 1;
    }
    return out[0..out_len];
}

/// Compare two link labels for CommonMark normalized equality without allocating.
pub fn labelsEqual(a: []const u8, b: []const u8): bool {
    let ia: usize = 0;
    let ib: usize = 0;
    while (ia < a.len && isAsciiWs(a[ia])) : (ia += 1) {}
    while (ib < b.len && isAsciiWs(b[ib])) : (ib += 1) {}

    let a_end = a.len;
    let b_end = b.len;
    while (a_end > ia && isAsciiWs(a[a_end - 1])) : (a_end -= 1) {}
    while (b_end > ib && isAsciiWs(b[b_end - 1])) : (b_end -= 1) {}

    if (ia == a_end && ib == b_end) return true;
    if (ia == a_end || ib == b_end) return false;

    const vsp: RefDefVec16 = @splat(' ' as u8);
    const vtb: RefDefVec16 = @splat('\t' as u8);
    const vnl: RefDefVec16 = @splat('\n' as u8);
    const vcr: RefDefVec16 = @splat('\r' as u8);
    const vA: RefDefVec16 = @splat('A' as u8);
    const vZ: RefDefVec16 = @splat('Z' as u8);
    const v20: RefDefVec16 = @splat(0x20 as u8);

    while (ia < a_end && ib < b_end) {
        const ws_a = isAsciiWs(a[ia]);
        const ws_b = isAsciiWs(b[ib]);
        if (ws_a != ws_b) return false;
        if (ws_a) {
            while (ia < a_end && isAsciiWs(a[ia])) : (ia += 1) {}
            while (ib < b_end && isAsciiWs(b[ib])) : (ib += 1) {}
        } else {
            if (ia + 16 <= a_end && ib + 16 <= b_end) {
                const va: RefDefVec16 = a[ia .. ia + 16][0..16].*;
                const vb: RefDefVec16 = b[ib .. ib + 16][0..16].*;
                const has_ws = (va == vsp) | (va == vtb) | (va == vnl) | (va == vcr) |
                    (vb == vsp) | (vb == vtb) | (vb == vnl) | (vb == vcr);
                if (!@reduce(.Or, has_ws)) {
                    const la = @select(u8, (va >= vA) & (va <= vZ), va | v20, va);
                    const lb = @select(u8, (vb >= vA) & (vb <= vZ), vb | v20, vb);
                    if (@reduce(.Or, la ^ lb) != 0) return false;
                    ia += 16;
                    ib += 16;
                    continue;
                }
            }
            if (toLowerAscii(a[ia]) != toLowerAscii(b[ib])) return false;
            ia += 1;
            ib += 1;
        }
    }
    return ia == a_end && ib == b_end;
}

/// Skip optional spaces/tabs plus at most one newline followed by optional spaces/tabs.
pub fn skipRefDefWhitespace(text: []const u8, start: usize): usize {
    let p = start;
    while (p < text.len && (text[p] == ' ' || text[p] == '\t')) : (p += 1) {}
    if (p < text.len && text[p] == '\n') {
        p += 1;
        while (p < text.len && (text[p] == ' ' || text[p] == '\t')) : (p += 1) {}
    }
    return p;
}

/// Parse a CommonMark link destination (`<...>` or bare with balanced parentheses).
pub fn parseRefDefDest(text: []const u8, start: usize): ParseDestResult? {
    let p = start;
    if (p >= text.len) return null;

    if (text[p] == '<') {
        p += 1;
        const dest_start = p;
        const vgt: RefDefVec16 = @splat('>' as u8);
        const vlt: RefDefVec16 = @splat('<' as u8);
        const vnl: RefDefVec16 = @splat('\n' as u8);
        const vbs: RefDefVec16 = @splat('\\' as u8);
        while (p < text.len && text[p] != '>' && text[p] != '\n') {
            if (p + 16 <= text.len) {
                const v: RefDefVec16 = text[p .. p + 16][0..16].*;
                if (!@reduce(.Or, (v == vgt) | (v == vlt) | (v == vnl) | (v == vbs))) {
                    p += 16;
                    continue;
                }
            }
            if (text[p] == '<') return null; // unescaped '<' forbidden inside angle destination
            if (text[p] == '\\' && p + 1 < text.len) {
                p += 2;
            } else {
                p += 1;
            }
        }
        if (p >= text.len || text[p] != '>') return null;
        const dest = text[dest_start..p];
        p += 1;
        return ParseDestResult{ dest: dest, end_pos: p };
    } else {
        const dest_start = p;
        let paren_depth: u32 = 0;
        const vsp: RefDefVec16 = @splat(0x20 as u8);
        const vlp: RefDefVec16 = @splat('(' as u8);
        const vrp: RefDefVec16 = @splat(')' as u8);
        const vbs: RefDefVec16 = @splat('\\' as u8);
        while (p < text.len && !isAsciiWs(text[p]) && text[p] > 0x1F) {
            if (p + 16 <= text.len) {
                const v: RefDefVec16 = text[p .. p + 16][0..16].*;
                if (!@reduce(.Or, (v <= vsp) | (v == vlp) | (v == vrp) | (v == vbs))) {
                    p += 16;
                    continue;
                }
            }
            if (text[p] == '(') {
                paren_depth += 1;
            } else if (text[p] == ')') {
                if (paren_depth == 0) break;
                paren_depth -= 1;
            }
            if (text[p] == '\\' && p + 1 < text.len) {
                p += 2;
            } else {
                p += 1;
            }
        }
        if (p == dest_start || paren_depth != 0) return null;
        return ParseDestResult{ dest: text[dest_start..p], end_pos: p };
    }
}

/// Parse a CommonMark link title delimited by `"..."`, `'...'`, or `(...)`.
/// Rejects blank lines inside the title and unescaped nested `(` in `(...)` titles.
pub fn parseRefDefTitle(text: []const u8, start: usize): ParseTitleResult? {
    let p = start;
    if (p >= text.len) return null;

    const open_char = text[p];
    let close_char: u8 = 0;
    if (open_char == '"' || open_char == '\'') {
        close_char = open_char;
    } else if (open_char == '(') {
        close_char = ')';
    } else {
        return null;
    }

    p += 1;
    const title_start = p;
    let prev_was_nl = false;

    while (p < text.len && text[p] != close_char) {
        if (text[p] == '\\' && p + 1 < text.len) {
            prev_was_nl = false;
            p += 2;
        } else {
            if (open_char == '(' && text[p] == '(') return null;
            if (text[p] == '\n') {
                if (prev_was_nl) return null; // blank line inside title forbidden
                prev_was_nl = true;
            } else if (text[p] != ' ' && text[p] != '\t') {
                prev_was_nl = false;
            }
            p += 1;
        }
    }
    if (p >= text.len || text[p] != close_char) return null;
    const title = text[title_start..p];
    p += 1;
    return ParseTitleResult{ title: title, end_pos: p };
}

/// Parse a single CommonMark §4.7 link reference definition starting at `pos`.
/// Syntax: `[label]: <dest> "optional title"` followed by end-of-line or EOF.
pub fn parseRefDef(text: []const u8, pos: usize): ParseRefDefResult? {
    let p = pos;
    // Allow up to 3 leading spaces per CommonMark
    let spaces: usize = 0;
    while (p < text.len && text[p] == ' ' && spaces < 3) : (p += 1) {
        spaces += 1;
    }
    if (p >= text.len || text[p] != '[') return null;
    p += 1;

    // Label up to 999 chars, no unescaped '[' or ']', must contain at least one non-whitespace char
    const label_start = p;
    let label_len: usize = 0;
    let has_non_ws = false;
    while (p < text.len && text[p] != ']') {
        if (text[p] == '[') return null;
        if (text[p] == '\\' && p + 1 < text.len) {
            has_non_ws = true;
            p += 2;
            label_len += 2;
        } else {
            if (!isAsciiWs(text[p])) has_non_ws = true;
            p += 1;
            label_len += 1;
        }
        if (label_len > 999) return null;
    }
    if (p >= text.len || !has_non_ws) return null;
    const label = text[label_start..p];
    p += 1; // skip ']'

    if (p >= text.len || text[p] != ':') return null;
    p += 1;

    p = skipRefDefWhitespace(text, p);
    const dest_res = parseRefDefDest(text, p) ?? return null;
    p = dest_res.end_pos;
    const dest = dest_res.dest;
    const pos_after_dest = p;

    const p_before_title_ws = p;
    p = skipRefDefWhitespace(text, p);
    let had_newline_before_title = false;
    let scan = p_before_title_ws;
    while (scan < p) : (scan += 1) {
        if (text[scan] == '\n') {
            had_newline_before_title = true;
            break;
        }
    }

    if (p < text.len && (text[p] == '"' || text[p] == '\'' || text[p] == '(')) {
        if (p > pos_after_dest) {
            if (title_res := parseRefDefTitle(text, p)) {
                let after_title = title_res.end_pos;
                while (after_title < text.len && (text[after_title] == ' ' || text[after_title] == '\t')) : (after_title += 1) {}
                if (after_title >= text.len || text[after_title] == '\n') {
                    let end_p = after_title;
                    if (end_p < text.len && text[end_p] == '\n') end_p += 1;
                    return ParseRefDefResult{
                        end_pos: end_p,
                        label: label,
                        dest: dest,
                        title: title_res.title,
                    };
                }
                if (!had_newline_before_title) return null;
            } else {
                if (!had_newline_before_title) return null;
            }
        }
    }

    // Backtrack to right after dest and verify end-of-line
    p = pos_after_dest;
    while (p < text.len && (text[p] == ' ' || text[p] == '\t')) : (p += 1) {}
    if (p < text.len && text[p] != '\n') return null;
    if (p < text.len && text[p] == '\n') p += 1;

    return ParseRefDefResult{
        end_pos: p,
        label: label,
        dest: dest,
        title: "",
    };
}

/// Fixed-capacity zero-allocation CommonMark link reference definition table
/// implementing first-definition-wins semantics.
pub struct RefDefTable {
    entries: [64]RefDef,
    len: usize,

    pub static fn init(): RefDefTable {
        return RefDefTable{
            entries: [RefDef{ label: "", dest: "", title: "" }; 64],
            len: 0,
        };
    }

    /// Insert a definition; ignores duplicates (CommonMark first-definition-wins).
    /// Returns true if inserted, false if duplicate or full.
    pub fn insert(*_, def: RefDef): bool {
        if (_.lookup(def.label) != null) return false;
        if (_.len >= 64) return false;
        _.entries[_.len] = def;
        _.len += 1;
        return true;
    }

    /// Look up a definition by normalized label equality.
    pub fn lookup(raw_label: []const u8): RefDef? {
        if (raw_label.len == 0) return null;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (labelsEqual(_.entries[i].label, raw_label)) {
                return _.entries[i];
            }
        }
        return null;
    }

    /// Parse as many consecutive link reference definitions as possible from `text`,
    /// populating the table and returning the byte offset where remaining content begins.
    pub fn parseAll(*_, text: []const u8): usize {
        let pos: usize = 0;
        while (pos < text.len) {
            const res = parseRefDef(text, pos) ?? break;
            if (_.insert(RefDef{ label: res.label, dest: res.dest, title: res.title })) {
                pos = res.end_pos;
            } else {
                pos = res.end_pos;
            }
        }
        return pos;
    }
}

fn isAsciiWs(c: u8): bool {
    return REFDEF_WS_LUT[usize(c)] != 0;
}

fn toLowerAscii(c: u8): u8 {
    return REFDEF_LOWER_LUT[usize(c)];
}
