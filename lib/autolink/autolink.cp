// Source: Absorbed from /data/dev/bun/src/md/autolinks.cp
//! GFM (GitHub Flavored Markdown) permissive autolink scanner for URLs (`http://`, `https://`,
//! `ftp://`), `www.` links, and email addresses, with trailing unbalanced parenthesis and
//! HTML entity reference trimming.

export enum AutolinkKind: u8 {
    url = 0,
    www = 1,
    email = 2,
}

export struct AutolinkSpan {
    beg: usize,
    end: usize,
    kind: AutolinkKind = .url,

    pub fn slice(content: []const u8): []const u8 {
        return content[_.beg.._.end];
    }

    pub fn len(): usize {
        return _.end - _.beg;
    }
}

export struct ScanUrlResult {
    end: usize,
    ok: bool,
}

struct Scheme {
    name: []const u8,
    suffix: []const u8,
}

export fn isListBullet(c: u8): bool {
    return c == '-' || c == '+' || c == '*';
}

export fn isListItemMark(c: u8): bool {
    return c == '-' || c == '+' || c == '*' || c == '.' || c == ')';
}

fn isAlphaNum(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn asciiCaseEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

export fn isInSet(c: u8, set: []const u8): bool {
    let i: usize = 0;
    while (i < set.len) : (i += 1) {
        if (c == set[i]) return true;
    }
    return false;
}

/// Check left boundary for permissive autolinks.
/// When `allow_emph` is true, emphasis delimiters (`*_~`) are also valid boundaries.
export fn checkLeftBoundary(content: []const u8, pos: usize, allow_emph: bool): bool {
    if (pos == 0) return true;
    return switch (content[pos - 1]) {
        ' ', '\t', '\n', '\r', 0x0B, 0x0C => true,
        '(', '{', '[' => true,
        '*', '_', '~' => allow_emph,
        else => false,
    };
}

/// Check right boundary for permissive autolinks.
/// When `allow_emph` is true, emphasis delimiters (`*_~`) are also valid boundaries.
export fn checkRightBoundary(content: []const u8, pos: usize, allow_emph: bool): bool {
    if (pos >= content.len) return true;
    return switch (content[pos]) {
        ' ', '\t', '\n', '\r', 0x0B, 0x0C => true,
        ')', '}', ']', '<' => true,
        '.', '!', '?', ',', ';', '&' => true,
        '*', '_', '~' => allow_emph,
        else => false,
    };
}

/// Scan a URL component (host, path, query, or fragment) following md4c's URL_MAP.
export fn scanUrlComponent(
    content: []const u8,
    start: usize,
    start_char: u8,
    delim_char: u8,
    allowed_nonalnum: []const u8,
    min_components: u32,
    optional_end_char: u8,
): ScanUrlResult {
    let pos = start;
    let n_components: u32 = 0;
    if (start_char != 0) {
        if (pos >= content.len || content[pos] != start_char) {
            return ScanUrlResult{ end: pos, ok: min_components == 0 };
        }
        if (min_components > 0 && (pos + 1 >= content.len || !isAlphaNum(content[pos + 1]))) {
            return ScanUrlResult{ end: pos, ok: min_components == 0 };
        }
        pos += 1;
    }

    while (pos < content.len) {
        if (isAlphaNum(content[pos])) {
            if (n_components == 0) n_components = 1;
            pos += 1;
        } else if (isInSet(content[pos], allowed_nonalnum) &&
            ((pos > 0 && (isAlphaNum(content[pos - 1]) || content[pos - 1] == ')' || isInSet(content[pos - 1], allowed_nonalnum))) || content[pos] == '(') &&
            ((pos + 1 < content.len && (isAlphaNum(content[pos + 1]) || content[pos + 1] == '(' || isInSet(content[pos + 1], allowed_nonalnum))) || content[pos] == ')'))
        {
            if (content[pos] == delim_char) n_components += 1;
            pos += 1;
        } else {
            break;
        }
    }

    if (pos < content.len && optional_end_char != 0 && content[pos] == optional_end_char) {
        pos += 1;
    }

    if (n_components < min_components) {
        return ScanUrlResult{ end: pos, ok: false };
    }
    return ScanUrlResult{ end: pos, ok: true };
}

/// GFM post-processing: trim trailing entity-like `&name;` suffixes and unbalanced `)`.
export fn postProcessAutolinkEnd(content: []const u8, beg: usize, end_in: usize): usize {
    let end = end_in;

    // Case 1: URL itself ends with `;` (e.g., `&hl;` was fully scanned)
    if (end > beg && content[end - 1] == ';') {
        let j = end - 2;
        while (j > beg && isAlphaNum(content[j])) {
            j -= 1;
        }
        if (j >= beg && content[j] == '&') {
            end = j;
        }
    }
    // Case 2: `;` is the next char after URL end (e.g., `commonmark&hl` followed by `;`)
    if (end < content.len && content[end] == ';' && end > beg) {
        let j = end - 1;
        while (j > beg && isAlphaNum(content[j])) {
            j -= 1;
        }
        if (j >= beg && content[j] == '&') {
            end = j;
        }
    }

    // Trim trailing unbalanced `)`
    while (end > beg && content[end - 1] == ')') {
        let open: i32 = 0;
        let close: i32 = 0;
        let k = beg;
        while (k < end) : (k += 1) {
            if (content[k] == '(') open += 1;
            if (content[k] == ')') close += 1;
        }
        if (close > open) {
            end -= 1;
        } else {
            break;
        }
    }

    return end;
}

/// Detect a permissive autolink anchored at trigger character `pos` (`:`, `@`, or `.`).
export fn findPermissiveAutolink(content: []const u8, pos: usize, allow_emph: bool): AutolinkSpan? {
    if (pos >= content.len) return null;
    const c = content[pos];

    if (c == ':') {
        const schemes: [3]Scheme = [
            Scheme{ name: "http", suffix: "//" },
            Scheme{ name: "https", suffix: "//" },
            Scheme{ name: "ftp", suffix: "//" },
        ];
        let s_idx: usize = 0;
        while (s_idx < schemes.len) : (s_idx += 1) {
            const scheme = schemes[s_idx];
            const slen = scheme.name.len;
            const suflen = scheme.suffix.len;
            if (pos >= slen && pos + 1 + suflen < content.len) {
                if (asciiCaseEql(content[pos - slen .. pos], scheme.name) &&
                    eqlBytes(content[pos + 1 .. pos + 1 + suflen], scheme.suffix))
                {
                    const beg = pos - slen;
                    if (!checkLeftBoundary(content, beg, allow_emph)) continue;

                    let end = pos + 1 + suflen;
                    const host = scanUrlComponent(content, end, 0, '.', ".-_", 2, 0);
                    if (!host.ok) continue;
                    end = host.end;

                    const path = scanUrlComponent(content, end, '/', '/', "/.-_~*+%", 0, '/');
                    end = path.end;

                    const query = scanUrlComponent(content, end, '?', '&', "&.-+_=()~*%", 1, 0);
                    end = query.end;

                    const frag = scanUrlComponent(content, end, '#', 0, ".-+_~*%", 1, 0);
                    end = frag.end;

                    end = postProcessAutolinkEnd(content, beg, end);
                    if (!checkRightBoundary(content, end, allow_emph)) continue;

                    return AutolinkSpan{ beg: beg, end: end, kind: .url };
                }
            }
        }
    } else if (c == '@') {
        if (pos == 0 || pos + 3 >= content.len) return null;
        if (!isAlphaNum(content[pos - 1]) || !isAlphaNum(content[pos + 1])) return null;

        let beg = pos;
        while (beg > 0) {
            if (isAlphaNum(content[beg - 1])) {
                beg -= 1;
            } else if (beg >= 2 && isAlphaNum(content[beg - 2]) &&
                isInSet(content[beg - 1], ".-_+") && isAlphaNum(content[beg]))
            {
                beg -= 1;
            } else {
                break;
            }
        }
        if (beg == pos) return null;
        if (!checkLeftBoundary(content, beg, allow_emph)) return null;

        const host = scanUrlComponent(content, pos + 1, 0, '.', ".-_", 2, 0);
        if (!host.ok) return null;
        const end = host.end;
        if (!checkRightBoundary(content, end, allow_emph)) return null;

        return AutolinkSpan{ beg: beg, end: end, kind: .email };
    } else if (c == '.') {
        if (pos < 3) return null;
        if (!asciiCaseEql(content[pos - 3 .. pos], "www")) return null;

        const beg = pos - 3;
        if (!checkLeftBoundary(content, beg, allow_emph)) return null;

        let end = pos + 1;
        const host = scanUrlComponent(content, end, 0, '.', ".-_", 1, 0);
        if (!host.ok) return null;
        end = host.end;

        const path = scanUrlComponent(content, end, '/', '/', "/.-_~*+%", 0, '/');
        end = path.end;

        const query = scanUrlComponent(content, end, '?', '&', "&.-+_=()~*%", 1, 0);
        end = query.end;

        const frag = scanUrlComponent(content, end, '#', 0, ".-+_~*%", 1, 0);
        end = frag.end;

        end = postProcessAutolinkEnd(content, beg, end);
        if (!checkRightBoundary(content, end, allow_emph)) return null;

        return AutolinkSpan{ beg: beg, end: end, kind: .www };
    }

    return null;
}

/// Scan forward from `start` to find the next permissive autolink in `content`.
export fn findNextAutolink(content: []const u8, start: usize, allow_emph: bool): AutolinkSpan? {
    let i = start;
    while (i < content.len) : (i += 1) {
        const ch = content[i];
        if (ch == ':' || ch == '@' || ch == '.') {
            if (span := findPermissiveAutolink(content, i, allow_emph)) {
                if (span.beg >= start) {
                    return span;
                }
            }
        }
    }
    return null;
}
