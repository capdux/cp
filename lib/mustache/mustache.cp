// Origin: Pure-CP Stdlib (Zero-Alloc Mustache Template Engine) | Created: 2026-11-22
// Pure-CP zero-allocation Mustache template tokenizer, validator, and HTML-escaping renderer.

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isWs(s[start])) {
        start += 1;
    }
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn findDoubleClose(s: []const u8, start_pos: usize): usize? {
    let i = start_pos;
    while (i + 1 < s.len) {
        if (s[i] == '}' && s[i + 1] == '}') {
            return i;
        }
        i += 1;
    }
    return null;
}

fn findTripleClose(s: []const u8, start_pos: usize): usize? {
    let i = start_pos;
    while (i + 2 < s.len) {
        if (s[i] == '}' && s[i + 1] == '}' && s[i + 2] == '}') {
            return i;
        }
        i += 1;
    }
    return null;
}

fn containsDoubleOpen(s: []const u8): bool {
    let i: usize = 0;
    while (i + 1 < s.len) {
        if (s[i] == '{' && s[i + 1] == '{') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn appendBytes(out: []u8, written: *usize, data: []const u8): bool {
    if (written.* + data.len > out.len) return false;
    let i: usize = 0;
    while (i < data.len) {
        out[written.* + i] = data[i];
        i += 1;
    }
    written.* += data.len;
    return true;
}

fn appendHtmlEscaped(out: []u8, written: *usize, data: []const u8): bool {
    let i: usize = 0;
    while (i < data.len) {
        const c = data[i];
        if (c == '&') {
            if (!appendBytes(out, written, "&amp;")) return false;
        } else if (c == '<') {
            if (!appendBytes(out, written, "&lt;")) return false;
        } else if (c == '>') {
            if (!appendBytes(out, written, "&gt;")) return false;
        } else if (c == '"') {
            if (!appendBytes(out, written, "&quot;")) return false;
        } else if (c == '\'') {
            if (!appendBytes(out, written, "&#39;")) return false;
        } else {
            if (written.* >= out.len) return false;
            out[written.*] = c;
            written.* += 1;
        }
        i += 1;
    }
    return true;
}

fn lookupVar(vars: []const MustacheVar, key: []const u8): []const u8 {
    let i: usize = 0;
    while (i < vars.len) {
        if (strEq(vars[i].key, key)) {
            return vars[i].value;
        }
        i += 1;
    }
    return "";
}

/// Token kind produced by `MustacheIter`.
pub enum MustacheTokenKind {
    text,
    escaped_var,
    raw_var,
    comment,
}

/// A single zero-copy token slice from a Mustache template.
pub struct MustacheToken {
    kind: MustacheTokenKind,
    content: []const u8,
}

/// A key-value variable binding for `renderMustache`.
pub struct MustacheVar {
    key: []const u8,
    value: []const u8,
}

/// Zero-allocation Mustache template tokenizer.
/// Distinguishes `{{{raw}}}`, `{{& raw}}`, `{{! comment}}`, `{{ var }}`, and plain text spans.
pub struct MustacheIter {
    src: []const u8,
    pos: usize,
    has_error: bool,

    fn init(template: []const u8): MustacheIter {
        return MustacheIter {
            src: template,
            pos: 0,
            has_error: false,
        };
    }

    fn next(*_): MustacheToken? {
        if (_.has_error || _.pos >= _.src.len) return null;

        // Check if current position begins a Mustache tag `{{`
        if (_.pos + 1 < _.src.len && _.src[_.pos] == '{' && _.src[_.pos + 1] == '{') {
            // 1. Triple-stash unescaped variable: `{{{ raw }}}`
            if (_.pos + 2 < _.src.len && _.src[_.pos + 2] == '{') {
                if (close_idx := findTripleClose(_.src, _.pos + 3)) {
                    const raw_inner = _.src[_.pos + 3..close_idx];
                    const key = trimWs(raw_inner);
                    if (key.len == 0 || containsDoubleOpen(key)) {
                        _.has_error = true;
                        return null;
                    }
                    _.pos = close_idx + 3;
                    return MustacheToken {
                        kind: MustacheTokenKind.raw_var,
                        content: key,
                    };
                }
                _.has_error = true;
                return null;
            }

            // 2. Comment tag: `{{! comment }}`
            if (_.pos + 2 < _.src.len && _.src[_.pos + 2] == '!') {
                if (close_idx := findDoubleClose(_.src, _.pos + 3)) {
                    const inner = trimWs(_.src[_.pos + 3..close_idx]);
                    _.pos = close_idx + 2;
                    return MustacheToken {
                        kind: MustacheTokenKind.comment,
                        content: inner,
                    };
                }
                _.has_error = true;
                return null;
            }

            // 3. Ampersand unescaped variable: `{{& raw }}`
            if (_.pos + 2 < _.src.len && _.src[_.pos + 2] == '&') {
                if (close_idx := findDoubleClose(_.src, _.pos + 3)) {
                    const raw_inner = _.src[_.pos + 3..close_idx];
                    const key = trimWs(raw_inner);
                    if (key.len == 0 || containsDoubleOpen(key)) {
                        _.has_error = true;
                        return null;
                    }
                    _.pos = close_idx + 2;
                    return MustacheToken {
                        kind: MustacheTokenKind.raw_var,
                        content: key,
                    };
                }
                _.has_error = true;
                return null;
            }

            // 4. Standard HTML-escaped variable: `{{ var }}`
            if (close_idx := findDoubleClose(_.src, _.pos + 2)) {
                const raw_inner = _.src[_.pos + 2..close_idx];
                const key = trimWs(raw_inner);
                if (key.len == 0 || containsDoubleOpen(key)) {
                    _.has_error = true;
                    return null;
                }
                _.pos = close_idx + 2;
                return MustacheToken {
                    kind: MustacheTokenKind.escaped_var,
                    content: key,
                };
            }
            _.has_error = true;
            return null;
        }

        // Plain text span until next `{{` or end of template
        const start = _.pos;
        while (_.pos < _.src.len) {
            if (_.pos + 1 < _.src.len && _.src[_.pos] == '{' && _.src[_.pos + 1] == '{') {
                break;
            }
            _.pos += 1;
        }
        return MustacheToken {
            kind: MustacheTokenKind.text,
            content: _.src[start.._.pos],
        };
    }
}

/// Validates that all `{{...}}` and `{{{...}}}` tags are properly closed and variable names are non-empty.
pub fn validateMustache(template: []const u8): bool {
    let it = MustacheIter.init(template);
    while (tok := it.next()) {
        * = tok;
    }
    return !it.has_error;
}

/// Renders a Mustache template into caller-provided `out` buffer with zero heap allocation.
/// - `{{var}}`: HTML-escapes `&`, `<`, `>`, `"`, `'`
/// - `{{{raw}}}` and `{{& raw}}`: outputs raw value unescaped
/// - `{{!comment}}`: omitted
/// - Missing variables render as empty string
/// - Returns `null` on unclosed/malformed tags or insufficient `out` buffer capacity
pub fn renderMustache(template: []const u8, vars: []const MustacheVar, out: []u8): ([]const u8)? {
    let it = MustacheIter.init(template);
    let written: usize = 0;

    while (tok := it.next()) {
        if (tok.kind == MustacheTokenKind.text) {
            if (!appendBytes(out, &written, tok.content)) return null;
        } else if (tok.kind == MustacheTokenKind.escaped_var) {
            const val = lookupVar(vars, tok.content);
            if (!appendHtmlEscaped(out, &written, val)) return null;
        } else if (tok.kind == MustacheTokenKind.raw_var) {
            const val = lookupVar(vars, tok.content);
            if (!appendBytes(out, &written, val)) return null;
        }
        // MustacheTokenKind.comment produces no output
    }

    if (it.has_error) return null;
    return out[0..written];
}
