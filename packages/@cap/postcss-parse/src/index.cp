// @cap/postcss-parse — Zero-heap CSS AST parser, query/mutation engine & serializer (postcss v8 parity)
// Clean-room MIT CP implementation of npm `postcss` (150M+/wk).

// Node kinds
pub const PC_RULE: u8 = 1;
pub const PC_DECL: u8 = 2;
pub const PC_AT_RULE: u8 = 3;
pub const PC_COMMENT: u8 = 4;

// Fixed-pool capacities (zero heap allocation)
pub const PC_MAX_NODES: usize = 32;
pub const PC_MAX_DECLS: usize = 64;
pub const PC_MAX_COMMENTS: usize = 16;

fn pcIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13 || c == 12;
}

fn pcToLower(c: u8): u8 {
    if (c >= 65 && c <= 90) {
        return c + 32;
    }
    return c;
}

fn pcStrEq(a: []const u8, b: string): bool {
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

fn pcStrEqIgnoreCase(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (pcToLower(a[i]) != pcToLower(b[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn pcCopyTrimmed(src: []const u8, dst: []u8): usize {
    let a: usize = 0;
    let b: usize = src.len;
    while (a < b && pcIsSpace(src[a])) {
        a = a + 1;
    }
    while (b > a && pcIsSpace(src[b - 1])) {
        b = b - 1;
    }
    const n = if (b - a < dst.len) { b - a } else { dst.len };
    let i: usize = 0;
    while (i < n) {
        dst[i] = src[a + i];
        i = i + 1;
    }
    return n;
}

fn pcAppendSlice(out: []u8, pos: *usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len && pos.* < out.len) {
        out[pos.*] = src[i];
        pos.* = pos.* + 1;
        i = i + 1;
    }
}

fn pcAppendByte(out: []u8, pos: *usize, b: u8) {
    if (pos.* < out.len) {
        out[pos.*] = b;
        pos.* = pos.* + 1;
    }
}

fn pcIsDeclAtRule(name: []const u8): bool {
    return pcStrEqIgnoreCase(name, "font-face") || pcStrEqIgnoreCase(name, "page");
}

pub struct PostCssDecl {
    pub kind: u8,
    pub parent_node: i32,
    pub prop: [48]u8,
    pub prop_len: usize,
    pub value: [128]u8,
    pub value_len: usize,
    pub important: bool,

    pub static fn empty(): PostCssDecl {
        return PostCssDecl {
            kind: PC_DECL,
            parent_node: -1,
            prop: [0; 48],
            prop_len: 0,
            value: [0; 128],
            value_len: 0,
            important: false,
        };
    }

    pub fn propStr(): []const u8 {
        return _.prop[0.._.prop_len];
    }

    pub fn valueStr(): []const u8 {
        return _.value[0.._.value_len];
    }
}

pub struct PostCssNode {
    pub kind: u8,
    pub parent_node: i32,
    pub name: [64]u8,
    pub name_len: usize,
    pub params: [128]u8,
    pub params_len: usize,
    pub has_block: bool,

    pub static fn empty(): PostCssNode {
        return PostCssNode {
            kind: PC_RULE,
            parent_node: -1,
            name: [0; 64],
            name_len: 0,
            params: [0; 128],
            params_len: 0,
            has_block: false,
        };
    }

    pub fn nameStr(): []const u8 {
        return _.name[0.._.name_len];
    }

    pub fn paramsStr(): []const u8 {
        return _.params[0.._.params_len];
    }
}

pub struct PostCssComment {
    pub kind: u8,
    pub text: [128]u8,
    pub text_len: usize,

    pub static fn empty(): PostCssComment {
        return PostCssComment {
            kind: PC_COMMENT,
            text: [0; 128],
            text_len: 0,
        };
    }

    pub fn textStr(): []const u8 {
        return _.text[0.._.text_len];
    }
}

fn pcAddComment(sheet: *PostCssSheet, raw_text: []const u8) {
    if (sheet.comment_len >= PC_MAX_COMMENTS) {
        return;
    }
    let c = PostCssComment.empty();
    c.text_len = pcCopyTrimmed(raw_text, c.text[0..128]);
    sheet.comments[sheet.comment_len] = c;
    sheet.comment_len = sheet.comment_len + 1;
}

/// Skips whitespace and `/* ... */` comments (recording each comment into `sheet`).
/// Returns `false` if an unterminated comment is encountered.
fn pcSkipWsAndComments(css: string, pos: *usize, end: usize, sheet: *PostCssSheet): bool {
    while (pos.* < end) {
        if (pcIsSpace(css[pos.*])) {
            pos.* = pos.* + 1;
            continue;
        }
        if (pos.* + 1 < end && css[pos.*] == 47 && css[pos.* + 1] == 42) {
            const cstart = pos.* + 2;
            let i = cstart;
            let closed = false;
            while (i + 1 < end) {
                if (css[i] == 42 && css[i + 1] == 47) {
                    pcAddComment(sheet, css[cstart..i]);
                    pos.* = i + 2;
                    closed = true;
                    break;
                }
                i = i + 1;
            }
            if (!closed) {
                sheet.ok = false;
                pos.* = end;
                return false;
            }
            continue;
        }
        break;
    }
    return true;
}

/// Scans a quoted string (`'...'` or `"..."`), respecting backslash escapes.
fn pcScanString(css: string, pos: *usize, end: usize, sheet: *PostCssSheet): bool {
    const quote = css[pos.*];
    let i = pos.* + 1;
    while (i < end) {
        if (css[i] == 92) {
            i = i + 2;
            continue;
        }
        if (css[i] == quote) {
            pos.* = i + 1;
            return true;
        }
        i = i + 1;
    }
    sheet.ok = false;
    pos.* = end;
    return false;
}

/// Strips inline comments from `css[start..end]` into `dst`, preserving quoted strings.
fn pcExtractCleanText(css: string, start: usize, end: usize, dst: []u8, sheet: *PostCssSheet): usize {
    let tmp: [256]u8 = [0; 256];
    let tlen: usize = 0;
    let i = start;
    while (i < end) {
        const c = css[i];
        if (c == 34 || c == 39) {
            const qstart = i;
            let qpos = i;
            pcScanString(css, &qpos, end, sheet);
            let k = qstart;
            while (k < qpos && tlen < 256) {
                tmp[tlen] = css[k];
                tlen = tlen + 1;
                k = k + 1;
            }
            i = qpos;
            continue;
        }
        if (i + 1 < end && c == 47 && css[i + 1] == 42) {
            const cstart = i + 2;
            let j = cstart;
            let closed = false;
            while (j + 1 < end) {
                if (css[j] == 42 && css[j + 1] == 47) {
                    pcAddComment(sheet, css[cstart..j]);
                    i = j + 2;
                    closed = true;
                    break;
                }
                j = j + 1;
            }
            if (!closed) {
                sheet.ok = false;
                break;
            }
            continue;
        }
        if (tlen < 256) {
            tmp[tlen] = c;
            tlen = tlen + 1;
        }
        i = i + 1;
    }
    return pcCopyTrimmed(tmp[0..tlen], dst);
}

/// Checks and strips trailing `!important` (case-insensitive) from `buf[0..len]`.
fn pcStripImportant(buf: []u8, len: *usize): bool {
    let n = len.*;
    while (n > 0 && pcIsSpace(buf[n - 1])) {
        n = n - 1;
    }
    if (n < 10) {
        len.* = n;
        return false;
    }
    // Check trailing "important" (9 chars)
    if (!pcStrEqIgnoreCase(buf[n - 9..n], "important")) {
        len.* = n;
        return false;
    }
    let k = n - 9;
    while (k > 0 && pcIsSpace(buf[k - 1])) {
        k = k - 1;
    }
    if (k == 0 || buf[k - 1] != 33) { // '!'
        len.* = n;
        return false;
    }
    k = k - 1;
    while (k > 0 && pcIsSpace(buf[k - 1])) {
        k = k - 1;
    }
    len.* = k;
    return true;
}

/// Parses declarations inside `css[start..end]` for `parent_node`.
fn pcParseDecls(css: string, start: usize, end: usize, parent_node: i32, sheet: *PostCssSheet) {
    let pos = start;
    while (pos < end) {
        if (!pcSkipWsAndComments(css, &pos, end, sheet)) {
            return;
        }
        if (pos >= end) {
            break;
        }
        if (css[pos] == 59) { // empty ';'
            pos = pos + 1;
            continue;
        }

        // Scan property name up to ':' or ';'
        const prop_start = pos;
        let colon_pos: usize = end;
        while (pos < end) {
            const c = css[pos];
            if (c == 34 || c == 39) {
                pcScanString(css, &pos, end, sheet);
                continue;
            }
            if (pos + 1 < end && c == 47 && css[pos + 1] == 42) {
                if (!pcSkipWsAndComments(css, &pos, end, sheet)) {
                    return;
                }
                continue;
            }
            if (c == 58) { // ':'
                colon_pos = pos;
                pos = pos + 1;
                break;
            }
            if (c == 59) { // ';' without ':' -> skip malformed decl
                pos = pos + 1;
                break;
            }
            pos = pos + 1;
        }
        if (colon_pos == end) {
            continue;
        }

        // Scan value up to ';' (protecting strings and parentheses)
        const val_start = pos;
        let paren_depth: i32 = 0;
        while (pos < end) {
            const c = css[pos];
            if (c == 34 || c == 39) {
                pcScanString(css, &pos, end, sheet);
                continue;
            }
            if (pos + 1 < end && c == 47 && css[pos + 1] == 42) {
                // Skip over comment; pcExtractCleanText will record it
                let j = pos + 2;
                let closed = false;
                while (j + 1 < end) {
                    if (css[j] == 42 && css[j + 1] == 47) {
                        pos = j + 2;
                        closed = true;
                        break;
                    }
                    j = j + 1;
                }
                if (!closed) {
                    sheet.ok = false;
                    pos = end;
                    break;
                }
                continue;
            }
            if (c == 40) { paren_depth = paren_depth + 1; }
            else if (c == 41 && paren_depth > 0) { paren_depth = paren_depth - 1; }
            else if (c == 59 && paren_depth == 0) {
                break;
            }
            pos = pos + 1;
        }
        const val_end = pos;
        if (pos < end && css[pos] == 59) {
            pos = pos + 1;
        }

        if (sheet.decl_len < PC_MAX_DECLS) {
            let d = PostCssDecl.empty();
            d.parent_node = parent_node;
            d.prop_len = pcExtractCleanText(css, prop_start, colon_pos, d.prop[0..48], sheet);
            d.value_len = pcExtractCleanText(css, val_start, val_end, d.value[0..128], sheet);
            let vlen = d.value_len;
            d.important = pcStripImportant(d.value[0..128], &vlen);
            d.value_len = vlen;
            if (d.prop_len > 0) {
                sheet.decls[sheet.decl_len] = d;
                sheet.decl_len = sheet.decl_len + 1;
            }
        }
    }
}

/// Finds matching `}` for an opening `{` at `open_pos`, protecting strings & comments.
fn pcFindMatchingBrace(css: string, open_pos: usize, end: usize, out_close: *usize, sheet: *PostCssSheet): bool {
    let i = open_pos + 1;
    let depth: i32 = 1;
    while (i < end) {
        const c = css[i];
        if (c == 34 || c == 39) {
            if (!pcScanString(css, &i, end, sheet)) {
                return false;
            }
            continue;
        }
        if (i + 1 < end && c == 47 && css[i + 1] == 42) {
            let j = i + 2;
            let closed = false;
            while (j + 1 < end) {
                if (css[j] == 42 && css[j + 1] == 47) {
                    i = j + 2;
                    closed = true;
                    break;
                }
                j = j + 1;
            }
            if (!closed) {
                sheet.ok = false;
                return false;
            }
            continue;
        }
        if (c == 123) {
            depth = depth + 1;
        } else if (c == 125) {
            depth = depth - 1;
            if (depth == 0) {
                out_close.* = i;
                return true;
            }
        }
        i = i + 1;
    }
    sheet.ok = false;
    return false;
}

/// Parses rules and at-rules inside `css[start..end]` with parent `parent_node`.
fn pcParseBlockList(css: string, start: usize, end: usize, parent_node: i32, depth: usize, sheet: *PostCssSheet) {
    if (depth > 4) {
        return;
    }
    let pos = start;
    while (pos < end) {
        if (!pcSkipWsAndComments(css, &pos, end, sheet)) {
            return;
        }
        if (pos >= end) {
            break;
        }
        if (css[pos] == 125) { // stray '}'
            sheet.ok = false;
            pos = pos + 1;
            continue;
        }

        if (css[pos] == 64) { // '@' At-Rule
            pos = pos + 1;
            const name_start = pos;
            while (pos < end && !pcIsSpace(css[pos]) && css[pos] != 59 && css[pos] != 123 && css[pos] != 40) {
                pos = pos + 1;
            }
            const name_end = pos;

            // Scan params up to ';' or '{'
            const params_start = pos;
            let term: u8 = 0;
            while (pos < end) {
                const c = css[pos];
                if (c == 34 || c == 39) {
                    if (!pcScanString(css, &pos, end, sheet)) {
                        return;
                    }
                    continue;
                }
                if (pos + 1 < end && c == 47 && css[pos + 1] == 42) {
                    let j = pos + 2;
                    let closed = false;
                    while (j + 1 < end) {
                        if (css[j] == 42 && css[j + 1] == 47) {
                            pos = j + 2;
                            closed = true;
                            break;
                        }
                        j = j + 1;
                    }
                    if (!closed) {
                        sheet.ok = false;
                        return;
                    }
                    continue;
                }
                if (c == 59 || c == 123) {
                    term = c;
                    break;
                }
                pos = pos + 1;
            }
            const params_end = pos;

            let node_idx: i32 = -1;
            if (sheet.node_len < PC_MAX_NODES) {
                let node = PostCssNode.empty();
                node.kind = PC_AT_RULE;
                node.parent_node = parent_node;
                node.name_len = pcCopyTrimmed(css[name_start..name_end], node.name[0..64]);
                node.params_len = pcExtractCleanText(css, params_start, params_end, node.params[0..128], sheet);
                node.has_block = term == 123;
                node_idx = sheet.node_len as i32;
                sheet.nodes[sheet.node_len] = node;
                sheet.node_len = sheet.node_len + 1;
            }

            if (term == 59) {
                pos = pos + 1;
            } else if (term == 123) {
                let close_pos: usize = 0;
                if (!pcFindMatchingBrace(css, pos, end, &close_pos, sheet)) {
                    return;
                }
                if (node_idx >= 0) {
                    const ni = node_idx as usize;
                    if (pcIsDeclAtRule(sheet.nodes[ni].name[0..sheet.nodes[ni].name_len])) {
                        pcParseDecls(css, pos + 1, close_pos, node_idx, sheet);
                    } else {
                        pcParseBlockList(css, pos + 1, close_pos, node_idx, depth + 1, sheet);
                    }
                }
                pos = close_pos + 1;
            } else {
                // EOF without ';' or '{'
                sheet.ok = false;
                return;
            }
            continue;
        }

        // Standard Rule: selector { decls }
        const sel_start = pos;
        let found_brace = false;
        while (pos < end) {
            const c = css[pos];
            if (c == 34 || c == 39) {
                if (!pcScanString(css, &pos, end, sheet)) {
                    return;
                }
                continue;
            }
            if (pos + 1 < end && c == 47 && css[pos + 1] == 42) {
                let j = pos + 2;
                let closed = false;
                while (j + 1 < end) {
                    if (css[j] == 42 && css[j + 1] == 47) {
                        pos = j + 2;
                        closed = true;
                        break;
                    }
                    j = j + 1;
                }
                if (!closed) {
                    sheet.ok = false;
                    return;
                }
                continue;
            }
            if (c == 123) {
                found_brace = true;
                break;
            }
            pos = pos + 1;
        }
        if (!found_brace) {
            sheet.ok = false;
            return;
        }
        const sel_end = pos;

        let close_pos: usize = 0;
        if (!pcFindMatchingBrace(css, pos, end, &close_pos, sheet)) {
            return;
        }

        let node_idx: i32 = -1;
        if (sheet.node_len < PC_MAX_NODES) {
            let node = PostCssNode.empty();
            node.kind = PC_RULE;
            node.parent_node = parent_node;
            node.name_len = pcExtractCleanText(css, sel_start, sel_end, node.name[0..64], sheet);
            node.has_block = true;
            node_idx = sheet.node_len as i32;
            sheet.nodes[sheet.node_len] = node;
            sheet.node_len = sheet.node_len + 1;
        }

        if (node_idx >= 0) {
            pcParseDecls(css, pos + 1, close_pos, node_idx, sheet);
        }
        pos = close_pos + 1;
    }
}

fn pcSerializeNode(sheet: PostCssSheet, node_idx: usize, indent: usize, out: []u8, pos: *usize) {
    const node = sheet.nodes[node_idx];
    let sp: usize = 0;
    while (sp < indent) {
        pcAppendByte(out, pos, 32);
        sp = sp + 1;
    }

    if (node.kind == PC_AT_RULE) {
        pcAppendByte(out, pos, 64); // '@'
        pcAppendSlice(out, pos, node.name[0..node.name_len]);
        if (node.params_len > 0) {
            pcAppendByte(out, pos, 32);
            pcAppendSlice(out, pos, node.params[0..node.params_len]);
        }
        if (!node.has_block) {
            pcAppendSlice(out, pos, ";\n");
            return;
        }
        pcAppendSlice(out, pos, " {\n");

        // Emit direct declarations of this at-rule (e.g. @font-face)
        let d: usize = 0;
        while (d < sheet.decl_len) {
            if (sheet.decls[d].parent_node == (node_idx as i32)) {
                let k: usize = 0;
                while (k < indent + 2) {
                    pcAppendByte(out, pos, 32);
                    k = k + 1;
                }
                pcAppendSlice(out, pos, sheet.decls[d].prop[0..sheet.decls[d].prop_len]);
                pcAppendSlice(out, pos, ": ");
                pcAppendSlice(out, pos, sheet.decls[d].value[0..sheet.decls[d].value_len]);
                if (sheet.decls[d].important) {
                    pcAppendSlice(out, pos, " !important");
                }
                pcAppendSlice(out, pos, ";\n");
            }
            d = d + 1;
        }

        // Emit nested child nodes (e.g. rules inside @media)
        let child: usize = 0;
        while (child < sheet.node_len) {
            if (sheet.nodes[child].parent_node == (node_idx as i32)) {
                pcSerializeNode(sheet, child, indent + 2, out, pos);
            }
            child = child + 1;
        }

        sp = 0;
        while (sp < indent) {
            pcAppendByte(out, pos, 32);
            sp = sp + 1;
        }
        pcAppendSlice(out, pos, "}\n");
        return;
    }

    // Standard PC_RULE
    pcAppendSlice(out, pos, node.name[0..node.name_len]);
    pcAppendSlice(out, pos, " {\n");
    let d: usize = 0;
    while (d < sheet.decl_len) {
        if (sheet.decls[d].parent_node == (node_idx as i32)) {
            let k: usize = 0;
            while (k < indent + 2) {
                pcAppendByte(out, pos, 32);
                k = k + 1;
            }
            pcAppendSlice(out, pos, sheet.decls[d].prop[0..sheet.decls[d].prop_len]);
            pcAppendSlice(out, pos, ": ");
            pcAppendSlice(out, pos, sheet.decls[d].value[0..sheet.decls[d].value_len]);
            if (sheet.decls[d].important) {
                pcAppendSlice(out, pos, " !important");
            }
            pcAppendSlice(out, pos, ";\n");
        }
        d = d + 1;
    }
    sp = 0;
    while (sp < indent) {
        pcAppendByte(out, pos, 32);
        sp = sp + 1;
    }
    pcAppendSlice(out, pos, "}\n");
}

pub struct PostCssSheet {
    pub nodes: [32]PostCssNode,
    pub node_len: usize,
    pub decls: [64]PostCssDecl,
    pub decl_len: usize,
    pub comments: [16]PostCssComment,
    pub comment_len: usize,
    pub ok: bool,

    pub static fn empty(): PostCssSheet {
        let nodes: [32]PostCssNode = undefined;
        let i: usize = 0;
        while (i < PC_MAX_NODES) {
            nodes[i] = PostCssNode.empty();
            i = i + 1;
        }
        let decls: [64]PostCssDecl = undefined;
        i = 0;
        while (i < PC_MAX_DECLS) {
            decls[i] = PostCssDecl.empty();
            i = i + 1;
        }
        let comments: [16]PostCssComment = undefined;
        i = 0;
        while (i < PC_MAX_COMMENTS) {
            comments[i] = PostCssComment.empty();
            i = i + 1;
        }
        return PostCssSheet {
            nodes: nodes,
            node_len: 0,
            decls: decls,
            decl_len: 0,
            comments: comments,
            comment_len: 0,
            ok: true,
        };
    }

    pub static fn parse(css: string): PostCssSheet {
        let sheet = PostCssSheet.empty();
        pcParseBlockList(css, 0, css.len, -1, 0, &sheet);
        return sheet;
    }

    pub fn ruleCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.node_len) {
            if (_.nodes[i].kind == PC_RULE) {
                count = count + 1;
            }
            i = i + 1;
        }
        return count;
    }

    pub fn declCount(): usize {
        return _.decl_len;
    }

    pub fn atRuleCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.node_len) {
            if (_.nodes[i].kind == PC_AT_RULE) {
                count = count + 1;
            }
            i = i + 1;
        }
        return count;
    }

    pub fn commentCount(): usize {
        return _.comment_len;
    }

    /// Queries the value of `prop` under `selector`, writing it into `out`.
    /// Returns written byte length, or -1 if not found.
    pub fn findDecl(selector: string, prop: string, out: []u8): i64 {
        let found_idx: i64 = -1;
        let d: usize = 0;
        while (d < _.decl_len) {
            const p = _.decls[d].parent_node;
            if (p >= 0) {
                const ni = p as usize;
                if (_.nodes[ni].kind == PC_RULE
                    && pcStrEq(_.nodes[ni].name[0.._.nodes[ni].name_len], selector)
                    && pcStrEqIgnoreCase(_.decls[d].prop[0.._.decls[d].prop_len], prop)) {
                    found_idx = d as i64;
                }
            }
            d = d + 1;
        }
        if (found_idx < 0) {
            return -1;
        }
        const di = found_idx as usize;
        const n = if (_.decls[di].value_len < out.len) { _.decls[di].value_len } else { out.len };
        let i: usize = 0;
        while (i < n) {
            out[i] = _.decls[di].value[i];
            i = i + 1;
        }
        return n as i64;
    }

    /// Returns whether the matching declaration `(selector, prop)` has `!important`.
    pub fn isDeclImportant(selector: string, prop: string): bool {
        let imp = false;
        let d: usize = 0;
        while (d < _.decl_len) {
            const p = _.decls[d].parent_node;
            if (p >= 0) {
                const ni = p as usize;
                if (_.nodes[ni].kind == PC_RULE
                    && pcStrEq(_.nodes[ni].name[0.._.nodes[ni].name_len], selector)
                    && pcStrEqIgnoreCase(_.decls[d].prop[0.._.decls[d].prop_len], prop)) {
                    imp = _.decls[d].important;
                }
            }
            d = d + 1;
        }
        return imp;
    }

    /// Updates an existing declaration `(selector, prop)` in place, or appends a new one
    /// (creating the rule if `selector` does not yet exist).
    pub fn setDecl(*_, selector: string, prop: string, val: string, important: bool): bool {
        let target_rule: i32 = -1;
        let n: usize = 0;
        while (n < _.node_len) {
            if (_.nodes[n].kind == PC_RULE && pcStrEq(_.nodes[n].name[0.._.nodes[n].name_len], selector)) {
                target_rule = n as i32;
            }
            n = n + 1;
        }

        if (target_rule >= 0) {
            let d: usize = 0;
            while (d < _.decl_len) {
                if (_.decls[d].parent_node == target_rule
                    && pcStrEqIgnoreCase(_.decls[d].prop[0.._.decls[d].prop_len], prop)) {
                    _.decls[d].value_len = pcCopyTrimmed(val, _.decls[d].value[0..128]);
                    _.decls[d].important = important;
                    return true;
                }
                d = d + 1;
            }
        } else {
            if (_.node_len >= PC_MAX_NODES || _.decl_len >= PC_MAX_DECLS) {
                return false;
            }
            let rule = PostCssNode.empty();
            rule.kind = PC_RULE;
            rule.parent_node = -1;
            rule.name_len = pcCopyTrimmed(selector, rule.name[0..64]);
            rule.has_block = true;
            target_rule = _.node_len as i32;
            _.nodes[_.node_len] = rule;
            _.node_len = _.node_len + 1;
        }

        if (_.decl_len >= PC_MAX_DECLS) {
            return false;
        }
        let decl = PostCssDecl.empty();
        decl.parent_node = target_rule;
        decl.prop_len = pcCopyTrimmed(prop, decl.prop[0..48]);
        decl.value_len = pcCopyTrimmed(val, decl.value[0..128]);
        decl.important = important;
        _.decls[_.decl_len] = decl;
        _.decl_len = _.decl_len + 1;
        return true;
    }

    /// Serializes the AST back into normalized CSS text (including `!important` and nested at-rules).
    pub fn toCss(out: []u8): usize {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.node_len) {
            if (_.nodes[i].parent_node == -1) {
                pcSerializeNode(_, i, 0, out, &pos);
            }
            i = i + 1;
        }
        return pos;
    }
}

export {
    PC_RULE,
    PC_DECL,
    PC_AT_RULE,
    PC_COMMENT,
    PC_MAX_NODES,
    PC_MAX_DECLS,
    PC_MAX_COMMENTS,
    PostCssDecl,
    PostCssNode,
    PostCssComment,
    PostCssSheet,
};
