// Source: Absorbed from /data/dev/bun/src/js_parser.zig | Synced: 2026-11-21T16:33:53Z (JSONC Comment & Trailing-Comma Stripper)
// Pure-CP zero-allocation JSON with Comments (//, /* */) and Trailing Commas stripper & scanner.

/// Result of peeking past whitespace and comments after a comma.
struct PeekResult {
    valid: bool;
    is_closing: bool;
}

fn isJsonSpace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

/// Skip a `/* ... */` block comment starting at `pos` (`input[pos..pos+2] == "/*"`).
/// Returns the index immediately after `*/`, or `null` if unterminated.
fn skipBlockComment(input: []const u8, pos: usize): usize? {
    let i = pos + 2;
    while (i + 1 < input.len) {
        if (input[i] == '*' && input[i + 1] == '/') {
            return i + 2;
        }
        i += 1;
    }
    return null;
}

/// Skip a `// ...` line comment starting at `pos`, stopping before `\r` or `\n` (or at EOF).
fn skipLineComment(input: []const u8, pos: usize): usize {
    let i = pos + 2;
    while (i < input.len && input[i] != '\n' && input[i] != '\r') {
        i += 1;
    }
    return i;
}

/// Skip a double-quoted JSON string starting at `pos` (`input[pos] == '"'`).
/// Returns the index immediately after the closing `"`, or `null` if unterminated.
fn skipString(input: []const u8, pos: usize): usize? {
    let i = pos + 1;
    while (i < input.len) {
        const b = input[i];
        if (b == '\\') {
            if (i + 1 >= input.len) return null;
            i += 2;
            continue;
        }
        if (b == '"') {
            return i + 1;
        }
        i += 1;
    }
    return null;
}

/// Look ahead from `pos` (right after a `,`), skipping whitespace and comments,
/// to check whether the next significant token is `}` or `]`.
fn peekTrailingComma(input: []const u8, pos: usize): PeekResult {
    let i = pos;
    while (i < input.len) {
        const b = input[i];
        if (isJsonSpace(b)) {
            i += 1;
            continue;
        }
        if (b == '/' && i + 1 < input.len) {
            if (input[i + 1] == '/') {
                i = skipLineComment(input, i);
                continue;
            }
            if (input[i + 1] == '*') {
                if (end_pos := skipBlockComment(input, i)) {
                    i = end_pos;
                    continue;
                }
                return PeekResult { valid: false, is_closing: false };
            }
        }
        const closing = (b == '}' || b == ']');
        return PeekResult { valid: true, is_closing: closing };
    }
    return PeekResult { valid: true, is_closing: false };
}

/// Core JSONC stripper shared by `stripJsonc` and `stripJsoncPreserveLines`.
fn stripJsoncImpl(input: []const u8, out: []u8, preserve_lines: bool): ([]const u8)? {
    let i: usize = 0;
    let w: usize = 0;

    while (i < input.len) {
        const b = input[i];

        // 1. Double-quoted string literal: copy verbatim including escapes
        if (b == '"') {
            const end_pos = skipString(input, i) orelse return null;
            const len = end_pos - i;
            if (w + len > out.len) return null;
            let k: usize = 0;
            while (k < len) {
                out[w + k] = input[i + k];
                k += 1;
            }
            w += len;
            i = end_pos;
            continue;
        }

        // 2. Single-line `//` or multi-line `/* ... */` comment
        if (b == '/' && i + 1 < input.len) {
            if (input[i + 1] == '/') {
                i = skipLineComment(input, i);
                continue;
            }
            if (input[i + 1] == '*') {
                const end_pos = skipBlockComment(input, i) orelse return null;
                if (preserve_lines) {
                    let k = i + 2;
                    while (k + 2 <= end_pos) {
                        if (input[k] == '\n') {
                            if (w >= out.len) return null;
                            out[w] = '\n';
                            w += 1;
                        }
                        k += 1;
                    }
                }
                i = end_pos;
                continue;
            }
        }

        // 3. Trailing comma before `}` or `]`
        if (b == ',') {
            const peek = peekTrailingComma(input, i + 1);
            if (!peek.valid) return null;
            if (peek.is_closing) {
                i += 1;
                continue;
            }
        }

        // 4. Regular JSON byte
        if (w >= out.len) return null;
        out[w] = b;
        w += 1;
        i += 1;
    }

    return out[0..w];
}

/// Strip `//` line comments, `/* ... */` block comments, and trailing commas before `}` / `]`
/// from `input`, writing standard JSON into `out`.
/// Protects contents of double-quoted strings (including escapes).
/// Returns `null` on unterminated block comment, unterminated string, or insufficient `out` buffer.
pub fn stripJsonc(input: []const u8, out: []u8): ([]const u8)? {
    return stripJsoncImpl(input, out, false);
}

/// Same as `stripJsonc`, but preserves `\n` newlines inside multi-line `/* ... */` block comments
/// so that line numbers remain aligned for downstream configuration error diagnostics.
pub fn stripJsoncPreserveLines(input: []const u8, out: []u8): ([]const u8)? {
    return stripJsoncImpl(input, out, true);
}

/// String-aware fast scan checking whether `input` contains any `//` or `/*` JSONC comments.
pub fn hasJsoncComments(input: []const u8): bool {
    let i: usize = 0;
    while (i < input.len) {
        const b = input[i];
        if (b == '"') {
            if (end_pos := skipString(input, i)) {
                i = end_pos;
                continue;
            }
            return false;
        }
        if (b == '/' && i + 1 < input.len) {
            if (input[i + 1] == '/' || input[i + 1] == '*') {
                return true;
            }
        }
        i += 1;
    }
    return false;
}

/// Count the number of trailing commas (commas followed only by whitespace/comments before `}` or `]`)
/// in `input`, ignoring commas inside string literals and comments.
pub fn countJsoncTrailingCommas(input: []const u8): usize {
    let i: usize = 0;
    let count: usize = 0;

    while (i < input.len) {
        const b = input[i];
        if (b == '"') {
            if (end_pos := skipString(input, i)) {
                i = end_pos;
                continue;
            }
            break;
        }
        if (b == '/' && i + 1 < input.len) {
            if (input[i + 1] == '/') {
                i = skipLineComment(input, i);
                continue;
            }
            if (input[i + 1] == '*') {
                if (end_pos := skipBlockComment(input, i)) {
                    i = end_pos;
                    continue;
                }
                break;
            }
        }
        if (b == ',') {
            const peek = peekTrailingComma(input, i + 1);
            if (!peek.valid) break;
            if (peek.is_closing) {
                count += 1;
            }
        }
        i += 1;
    }

    return count;
}
