// Source: Absorbed from /data/dev/bun/src/ini.zig (EditorConfig Parser & File Property Resolver)
// Pure-CP zero-allocation .editorconfig parser, section glob matcher, and property resolver.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn parseU32Strict(s: []const u8): u32? {
    if (s.len == 0) return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        if (val > 429496729) return null;
        val = val * 10 + @as(u32, c - '0');
        i += 1;
    }
    return val;
}

fn containsSlash(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '/') return true;
        i += 1;
    }
    return false;
}

fn basenameOf(path: []const u8): []const u8 {
    let last_slash: usize? = null;
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == '/') last_slash = i;
        i += 1;
    }
    if (ls := last_slash) return path[ls + 1..];
    return path;
}

/// Match a simple glob segment (supports `*`, `**`, `?`, `{a,b,c}` brace alternation).
fn matchSimpleGlob(pat: []const u8, text: []const u8): bool {
    // Expand first top-level `{a,b,c}` alternation if present
    let open_brace: usize? = null;
    let close_brace: usize? = null;
    let bi: usize = 0;
    while (bi < pat.len) {
        if (pat[bi] == '{' && open_brace == null) {
            open_brace = bi;
        } else if (pat[bi] == '}' && open_brace != null) {
            close_brace = bi;
            break;
        }
        bi += 1;
    }
    if (open_brace != null && close_brace != null) {
        const ob = open_brace.?;
        const cb = close_brace.?;
        const prefix = pat[0..ob];
        const suffix = pat[cb + 1..];
        const inner = pat[ob + 1..cb];
        let alt_start: usize = 0;
        let k: usize = 0;
        while (k <= inner.len) {
            if (k == inner.len || inner[k] == ',') {
                const alt = inner[alt_start..k];
                let combined: [256]u8 = [0; 256];
                if (prefix.len + alt.len + suffix.len <= combined.len) {
                    let w: usize = 0;
                    let a: usize = 0;
                    while (a < prefix.len) {
                        combined[w] = prefix[a];
                        w += 1;
                        a += 1;
                    }
                    let b: usize = 0;
                    while (b < alt.len) {
                        combined[w] = alt[b];
                        w += 1;
                        b += 1;
                    }
                    let c: usize = 0;
                    while (c < suffix.len) {
                        combined[w] = suffix[c];
                        w += 1;
                        c += 1;
                    }
                    if (matchSimpleGlob(combined[0..w], text)) return true;
                }
                alt_start = k + 1;
            }
            k += 1;
        }
        return false;
    }

    // Standard wildcard matching (`**` crosses `/`, `*` does not cross `/`, `?` matches any non-`/`)
    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize? = null;
    let star_ti: usize = 0;
    let star_cross_slash = false;

    while (ti < text.len) {
        if (pi + 1 < pat.len && pat[pi] == '*' && pat[pi + 1] == '*') {
            star_pi = pi + 2;
            star_ti = ti;
            star_cross_slash = true;
            pi += 2;
            continue;
        }
        if (pi < pat.len && pat[pi] == '*') {
            star_pi = pi + 1;
            star_ti = ti;
            star_cross_slash = false;
            pi += 1;
            continue;
        }
        if (pi < pat.len && pat[pi] == '?' && text[ti] != '/') {
            pi += 1;
            ti += 1;
            continue;
        }
        if (pi < pat.len && pat[pi] == text[ti]) {
            pi += 1;
            ti += 1;
            continue;
        }
        if (sp := star_pi) {
            if (!star_cross_slash && text[star_ti] == '/') {
                return false;
            }
            star_ti += 1;
            ti = star_ti;
            pi = sp;
            continue;
        }
        return false;
    }

    while (pi < pat.len && pat[pi] == '*') {
        pi += 1;
    }
    return pi == pat.len;
}

/// Match an EditorConfig section pattern against a relative POSIX file path.
export fn matchSectionPattern(pattern: []const u8, rel_path: []const u8): bool {
    let p = pattern;
    if (p.len > 0 && p[0] == '/') p = p[1..];
    let target = rel_path;
    if (target.len > 0 && target[0] == '/') target = target[1..];

    if (!containsSlash(pattern)) {
        return matchSimpleGlob(p, basenameOf(target));
    }
    return matchSimpleGlob(p, target);
}

export enum IndentStyle {
    unset,
    space,
    tab,
}

export enum EndOfLine {
    unset,
    lf,
    crlf,
    cr,
}

export enum Charset {
    unset,
    latin1,
    utf8,
    utf8_bom,
    utf16be,
    utf16le,
}

export struct EditorConfigProps {
    is_root: bool,
    indent_style: IndentStyle,
    indent_size: u32?,
    indent_size_tab: bool,
    tab_width: u32?,
    end_of_line: EndOfLine,
    charset: Charset,
    trim_trailing_whitespace: bool?,
    insert_final_newline: bool?,
    max_line_length: u32?,

    pub fn empty(): EditorConfigProps {
        return EditorConfigProps {
            is_root: false,
            indent_style: IndentStyle.unset,
            indent_size: null,
            indent_size_tab: false,
            tab_width: null,
            end_of_line: EndOfLine.unset,
            charset: Charset.unset,
            trim_trailing_whitespace: null,
            insert_final_newline: null,
            max_line_length: null,
        };
    }

    /// Resolve effective tab width (falls back to `indent_size` if `tab_width` is unset).
    pub fn effectiveTabWidth(): u32? {
        if (tw := _.tab_width) return tw;
        return _.indent_size;
    }
}

/// Resolve `.editorconfig` properties for `rel_path` (POSIX `/` separators).
/// Later matching sections override earlier ones; `unset` clears a property.
export fn resolveEditorConfig(content: []const u8, rel_path: []const u8): EditorConfigProps {
    let props = EditorConfigProps.empty();
    let in_preamble = true;
    let section_active = false;

    let pos: usize = 0;
    if (content.len >= 3 && content[0] == 0xEF && content[1] == 0xBB && content[2] == 0xBF) {
        pos = 3;
    }

    while (pos < content.len) {
        const line_start = pos;
        while (pos < content.len && content[pos] != '\n') pos += 1;
        let line_end = pos;
        if (pos < content.len && content[pos] == '\n') pos += 1;
        if (line_end > line_start && content[line_end - 1] == '\r') line_end -= 1;

        const line = trimWs(content[line_start..line_end]);
        if (line.len == 0 || line[0] == '#' || line[0] == ';') continue;

        if (line[0] == '[' && line[line.len - 1] == ']') {
            in_preamble = false;
            const sec_pat = trimWs(line[1..line.len - 1]);
            section_active = matchSectionPattern(sec_pat, rel_path);
            continue;
        }

        let eq_pos: usize? = null;
        let k: usize = 0;
        while (k < line.len) {
            if (line[k] == '=') {
                eq_pos = k;
                break;
            }
            k += 1;
        }
        if (eq_pos == null) continue;
        const ep = eq_pos.?;
        const key = trimWs(line[0..ep]);
        const val = trimWs(line[ep + 1..]);

        if (in_preamble) {
            if (eqIgnoreCase(key, "root") && eqIgnoreCase(val, "true")) {
                props.is_root = true;
            }
            continue;
        }
        if (!section_active) continue;

        if (eqIgnoreCase(key, "indent_style")) {
            if (eqIgnoreCase(val, "space")) {
                props.indent_style = IndentStyle.space;
            } else if (eqIgnoreCase(val, "tab")) {
                props.indent_style = IndentStyle.tab;
            } else if (eqIgnoreCase(val, "unset")) {
                props.indent_style = IndentStyle.unset;
            }
        } else if (eqIgnoreCase(key, "indent_size")) {
            if (eqIgnoreCase(val, "tab")) {
                props.indent_size = null;
                props.indent_size_tab = true;
            } else if (eqIgnoreCase(val, "unset")) {
                props.indent_size = null;
                props.indent_size_tab = false;
            } else if (n := parseU32Strict(val)) {
                props.indent_size = n;
                props.indent_size_tab = false;
            }
        } else if (eqIgnoreCase(key, "tab_width")) {
            if (eqIgnoreCase(val, "unset")) {
                props.tab_width = null;
            } else if (n := parseU32Strict(val)) {
                props.tab_width = n;
            }
        } else if (eqIgnoreCase(key, "end_of_line")) {
            if (eqIgnoreCase(val, "lf")) {
                props.end_of_line = EndOfLine.lf;
            } else if (eqIgnoreCase(val, "crlf")) {
                props.end_of_line = EndOfLine.crlf;
            } else if (eqIgnoreCase(val, "cr")) {
                props.end_of_line = EndOfLine.cr;
            } else if (eqIgnoreCase(val, "unset")) {
                props.end_of_line = EndOfLine.unset;
            }
        } else if (eqIgnoreCase(key, "charset")) {
            if (eqIgnoreCase(val, "utf-8")) {
                props.charset = Charset.utf8;
            } else if (eqIgnoreCase(val, "utf-8-bom")) {
                props.charset = Charset.utf8_bom;
            } else if (eqIgnoreCase(val, "latin1")) {
                props.charset = Charset.latin1;
            } else if (eqIgnoreCase(val, "utf-16be")) {
                props.charset = Charset.utf16be;
            } else if (eqIgnoreCase(val, "utf-16le")) {
                props.charset = Charset.utf16le;
            } else if (eqIgnoreCase(val, "unset")) {
                props.charset = Charset.unset;
            }
        } else if (eqIgnoreCase(key, "trim_trailing_whitespace")) {
            if (eqIgnoreCase(val, "true")) {
                props.trim_trailing_whitespace = true;
            } else if (eqIgnoreCase(val, "false")) {
                props.trim_trailing_whitespace = false;
            } else if (eqIgnoreCase(val, "unset")) {
                props.trim_trailing_whitespace = null;
            }
        } else if (eqIgnoreCase(key, "insert_final_newline")) {
            if (eqIgnoreCase(val, "true")) {
                props.insert_final_newline = true;
            } else if (eqIgnoreCase(val, "false")) {
                props.insert_final_newline = false;
            } else if (eqIgnoreCase(val, "unset")) {
                props.insert_final_newline = null;
            }
        } else if (eqIgnoreCase(key, "max_line_length")) {
            if (eqIgnoreCase(val, "off") || eqIgnoreCase(val, "unset")) {
                props.max_line_length = null;
            } else if (n := parseU32Strict(val)) {
                props.max_line_length = n;
            }
        }
    }

    return props;
}
