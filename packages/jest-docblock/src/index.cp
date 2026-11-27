// @cap/jest-docblock — Zero-heap docblock extractor, stripper, parser, and printer in pure CP.
// Clean-room MIT port of npm `jest-docblock` (`extract`, `strip`, `parse`, `parseWithComments`, `print`).
// Accelerated with 16-lane @Vector(16, u8) SIMD newline/comment/string scanning, comptime 256-entry whitespace LUT, and bulk @memcpy.

type JdblVec16 = @Vector(16, u8);

fn jdblBuildWsLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    // bit 0 = any ws (' ', '\t', '\r', '\n'); bit 1 = line ws (' ', '\t', '\r')
    lut[32] = 3; // ' '
    lut[9] = 3;  // '\t'
    lut[13] = 3; // '\r'
    lut[10] = 1; // '\n'
    return lut;
}

const JDBL_WS_LUT: [256]u8 = jdblBuildWsLut();

pub struct DocPragma {
    key: string,
    value: string,
}

pub struct ParsedDocblock {
    pub comments_len: usize,
    pub pragma_count: usize,
}

fn jdblIsWs(c: u8): bool {
    return (JDBL_WS_LUT[c as usize] & 1) != 0;
}

fn jdblIsLineWs(c: u8): bool {
    return (JDBL_WS_LUT[c as usize] & 2) != 0;
}

fn jdblStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: JdblVec16 = a[i .. i + 16][0..16].*;
        const vb: JdblVec16 = b[i .. i + 16][0..16].*;
        if (!@reduce(.And, va == vb)) return false;
        i += 16;
    }
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn jdblFindNewline(s: string, start: usize): usize {
    let i = start;
    const len = s.len;
    const v_nl: JdblVec16 = @splat(10 as u8);
    while (i + 16 <= len) {
        const v: JdblVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_nl)) {
            while (i < len && s[i] != '\n') : (i += 1) {}
            return i;
        }
        i += 16;
    }
    while (i < len && s[i] != '\n') : (i += 1) {}
    return i;
}

// Finds index `j` where `s[j] == '*'` and `s[j + 1] == '/'`, or `s.len` if not found.
fn jdblFindCloseComment(s: string, start: usize): usize {
    let j = start;
    const len = s.len;
    const v_star: JdblVec16 = @splat(42 as u8);
    while (j + 16 < len) {
        const v: JdblVec16 = s[j .. j + 16][0..16].*;
        if (@reduce(.Or, v == v_star)) {
            const limit = j + 16;
            while (j < limit) : (j += 1) {
                if (s[j] == '*' && s[j + 1] == '/') {
                    return j;
                }
            }
        } else {
            j += 16;
        }
    }
    while (j + 1 < len) : (j += 1) {
        if (s[j] == '*' && s[j + 1] == '/') {
            return j;
        }
    }
    return len;
}

fn jdblTrimLine(s: string): string {
    let start: usize = 0;
    while (start < s.len && jdblIsLineWs(s[start])) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && jdblIsLineWs(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn jdblWriteStr(src: string, out: []u8, pos: usize): usize {
    if (pos >= out.len || src.len == 0) {
        return pos;
    }
    let take = src.len;
    const avail = out.len - pos;
    if (take > avail) {
        take = avail;
    }
    @memcpy(out[pos .. pos + take], src[0..take]);
    return pos + take;
}

fn jdblStripDelimiters(docblock: string): string {
    let body = docblock;
    if (body.len >= 5 && body[0] == '/' && body[1] == '*' && body[2] == '*' && body[body.len - 2] == '*' && body[body.len - 1] == '/') {
        body = body[3..body.len - 2];
    } else if (body.len >= 4 && body[0] == '/' && body[1] == '*' && body[body.len - 2] == '*' && body[body.len - 1] == '/') {
        body = body[2..body.len - 2];
    }
    return body;
}

pub fn extractDocblock(code: string): string {
    let i: usize = 0;
    while (i < code.len && jdblIsWs(code[i])) : (i += 1) {}

    // Skip optional shebang line
    if (i + 1 < code.len && code[i] == '#' && code[i + 1] == '!') {
        i = jdblFindNewline(code, i);
        while (i < code.len && jdblIsWs(code[i])) : (i += 1) {}
    }

    if (i + 2 >= code.len || code[i] != '/' || code[i + 1] != '*' || code[i + 2] != '*') {
        return "";
    }

    const j = jdblFindCloseComment(code, i + 3);
    if (j < code.len) {
        return code[i..j + 2];
    }
    return "";
}

/// npm `jest-docblock` `extract`: extracts the top-of-file docblock string (or `""`).
pub fn extract(code: string): string {
    return extractDocblock(code);
}

/// npm `jest-docblock` `strip`: writes `code` with its leading docblock removed into `out`
/// (preserving any leading shebang line). Returns the written byte length.
pub fn strip(code: string, out: []u8): usize {
    let i: usize = 0;
    while (i < code.len && jdblIsWs(code[i])) : (i += 1) {}

    let shebang_end: usize = 0;
    if (i + 1 < code.len && code[i] == '#' && code[i + 1] == '!') {
        i = jdblFindNewline(code, i);
        if (i < code.len && code[i] == '\n') { i += 1; }
        shebang_end = i;
        while (i < code.len && jdblIsWs(code[i])) : (i += 1) {}
    }

    if (i + 2 >= code.len || code[i] != '/' || code[i + 1] != '*' || code[i + 2] != '*') {
        return jdblWriteStr(code, out, 0);
    }

    const close_idx = jdblFindCloseComment(code, i + 3);
    if (close_idx >= code.len) {
        return jdblWriteStr(code, out, 0);
    }
    let found_end = close_idx + 2;

    // Consume trailing newline immediately after closing */
    if (found_end < code.len && code[found_end] == '\r') { found_end += 1; }
    if (found_end < code.len && code[found_end] == '\n') { found_end += 1; }

    let pos: usize = 0;
    if (shebang_end > 0) {
        pos = jdblWriteStr(code[0..shebang_end], out, pos);
    }
    pos = jdblWriteStr(code[found_end..code.len], out, pos);
    return pos;
}

/// Zero-copy `strip` variant for inputs without a leading shebang.
pub fn stripSlice(code: string): string {
    let i: usize = 0;
    while (i < code.len && jdblIsWs(code[i])) : (i += 1) {}
    if (i + 2 >= code.len || code[i] != '/' || code[i + 1] != '*' || code[i + 2] != '*') {
        return code;
    }
    const j = jdblFindCloseComment(code, i + 3);
    if (j < code.len) {
        let end = j + 2;
        if (end < code.len && code[end] == '\r') { end += 1; }
        if (end < code.len && code[end] == '\n') { end += 1; }
        return code[end..code.len];
    }
    return code;
}

pub fn parseDocblock(docblock: string, out: []DocPragma): usize {
    const body = jdblStripDelimiters(docblock);

    let written: usize = 0;
    let pos: usize = 0;
    while (pos <= body.len) {
        const lineEnd = jdblFindNewline(body, pos);

        let line = jdblTrimLine(body[pos..lineEnd]);
        if (line.len > 0 && line[0] == '*') {
            line = jdblTrimLine(line[1..line.len]);
        }

        if (line.len > 1 && line[0] == '@') {
            let kEnd: usize = 1;
            while (kEnd < line.len && !jdblIsLineWs(line[kEnd])) : (kEnd += 1) {}
            const key = line[1..kEnd];
            let vStart = kEnd;
            while (vStart < line.len && jdblIsLineWs(line[vStart])) : (vStart += 1) {}
            const val = jdblTrimLine(line[vStart..line.len]);
            if (key.len > 0 && written < out.len) {
                out[written] = DocPragma{ key: key, value: val };
                written += 1;
            }
        }

        if (lineEnd >= body.len) {
            break;
        }
        pos = lineEnd + 1;
    }
    return written;
}

/// npm `jest-docblock` `parse`: alias for `parseDocblock`.
pub fn parse(docblock: string, out: []DocPragma): usize {
    return parseDocblock(docblock, out);
}

/// npm `jest-docblock` `parseWithComments`: parses both free-form comments and `@pragma` entries.
pub fn parseWithComments(
    docblock: string,
    comments_out: []u8,
    pragmas_out: []DocPragma,
): ParsedDocblock {
    const body = jdblStripDelimiters(docblock);

    let pragma_count: usize = 0;
    let cpos: usize = 0;
    let has_comment_line = false;
    let pending_blank_lines: usize = 0;
    let pos: usize = 0;

    while (pos <= body.len) {
        const lineEnd = jdblFindNewline(body, pos);

        let line = jdblTrimLine(body[pos..lineEnd]);
        if (line.len > 0 && line[0] == '*') {
            line = jdblTrimLine(line[1..line.len]);
        }

        if (line.len > 1 && line[0] == '@') {
            let kEnd: usize = 1;
            while (kEnd < line.len && !jdblIsLineWs(line[kEnd])) : (kEnd += 1) {}
            const key = line[1..kEnd];
            let vStart = kEnd;
            while (vStart < line.len && jdblIsLineWs(line[vStart])) : (vStart += 1) {}
            const val = jdblTrimLine(line[vStart..line.len]);
            if (key.len > 0 && pragma_count < pragmas_out.len) {
                pragmas_out[pragma_count] = DocPragma{ key: key, value: val };
                pragma_count += 1;
            }
        } else if (line.len == 0) {
            if (has_comment_line) {
                pending_blank_lines += 1;
            }
        } else {
            if (has_comment_line) {
                let b: usize = 0;
                while (b <= pending_blank_lines) : (b += 1) {
                    cpos = jdblWriteStr("\n", comments_out, cpos);
                }
            }
            pending_blank_lines = 0;
            has_comment_line = true;
            cpos = jdblWriteStr(line, comments_out, cpos);
        }

        if (lineEnd >= body.len) {
            break;
        }
        pos = lineEnd + 1;
    }

    return ParsedDocblock { comments_len: cpos, pragma_count: pragma_count };
}

pub fn printDocblock(pragmas: []const DocPragma, out: []u8): usize {
    if (pragmas.len == 0) {
        return 0;
    }
    let pos = jdblWriteStr("/**\n", out, 0);
    let i: usize = 0;
    while (i < pragmas.len) : (i += 1) {
        const p = pragmas[i];
        pos = jdblWriteStr(" * @", out, pos);
        pos = jdblWriteStr(p.key, out, pos);
        if (p.value.len > 0) {
            pos = jdblWriteStr(" ", out, pos);
            pos = jdblWriteStr(p.value, out, pos);
        }
        pos = jdblWriteStr("\n", out, pos);
    }
    pos = jdblWriteStr(" */", out, pos);
    return pos;
}

/// npm `jest-docblock` `print`: alias for `printDocblock`.
pub fn print(pragmas: []const DocPragma, out: []u8): usize {
    return printDocblock(pragmas, out);
}

/// Full `jest-docblock` `print({ comments, pragmas })` supporting comments + pragmas roundtrip.
pub fn printWithComments(comments: string, pragmas: []const DocPragma, out: []u8): usize {
    if (comments.len == 0 && pragmas.len == 0) {
        return 0;
    }
    if (comments.len == 0) {
        return printDocblock(pragmas, out);
    }

    let pos = jdblWriteStr("/**\n", out, 0);
    let cidx: usize = 0;
    while (cidx <= comments.len) {
        const lineEnd = jdblFindNewline(comments, cidx);
        const seg = comments[cidx..lineEnd];
        if (seg.len > 0) {
            pos = jdblWriteStr(" * ", out, pos);
            pos = jdblWriteStr(seg, out, pos);
            pos = jdblWriteStr("\n", out, pos);
        } else {
            pos = jdblWriteStr(" *\n", out, pos);
        }
        if (lineEnd >= comments.len) break;
        cidx = lineEnd + 1;
    }

    if (pragmas.len > 0) {
        pos = jdblWriteStr(" *\n", out, pos);
        let i: usize = 0;
        while (i < pragmas.len) : (i += 1) {
            const p = pragmas[i];
            pos = jdblWriteStr(" * @", out, pos);
            pos = jdblWriteStr(p.key, out, pos);
            if (p.value.len > 0) {
                pos = jdblWriteStr(" ", out, pos);
                pos = jdblWriteStr(p.value, out, pos);
            }
            pos = jdblWriteStr("\n", out, pos);
        }
    }

    pos = jdblWriteStr(" */", out, pos);
    return pos;
}

/// Looks up the first pragma value matching `key`.
pub fn getPragma(pragmas: []const DocPragma, key: string): string? {
    let i: usize = 0;
    while (i < pragmas.len) : (i += 1) {
        if (jdblStrEq(pragmas[i].key, key)) {
            return pragmas[i].value;
        }
    }
    return null;
}

/// Returns `true` if `pragmas` contains an entry for `key`.
pub fn hasPragma(pragmas: []const DocPragma, key: string): bool {
    return getPragma(pragmas, key) != null;
}
