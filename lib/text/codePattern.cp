// Origin: Pure-CP Stdlib (AI Coding Agent Token-Aware Structural Code Pattern Matcher) | Created: 2026-11-22
// Zero-heap-allocation token-aware structural code pattern matcher with $VAR metavariable
// capture and cross-position consistency constraints (lightweight ast-grep / semgrep engine).

const std = @import("std");

const MAX_LOCAL_CAPTURES: usize = 32;

/// Lightweight lexical token classification for structural code pattern matching.
pub enum CodeTokenKind {
    Ident,
    Number,
    String,
    Punct,
    MetaVar,
}

/// Zero-copy token span with 1-based source line number.
pub struct CodeToken {
    kind: CodeTokenKind = CodeTokenKind.Ident;
    start: usize = 0;
    end: usize = 0;
    line: u32 = 0;

    /// Returns the zero-copy token text slice from `src` (bounds-safe).
    fn slice(src: string): string {
        if (_.start <= _.end && _.end <= src.len) {
            return src[_.start.._.end];
        }
        return "";
    }
}

/// Zero-copy metavariable binding captured during a pattern match.
pub struct MetaCapture {
    var_name: string = "";
    captured_text: string = "";
}

/// Byte/line span and capture slice metadata for a single pattern match hit.
pub struct PatternMatchHit {
    start_byte: usize = 0;
    end_byte: usize = 0;
    start_line: u32 = 0;
    end_line: u32 = 0;
    capture_start: usize = 0;
    capture_count: usize = 0;
}

fn isAlphaOrUnderscore(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isIdentChar(c: u8): bool {
    return isAlphaOrUnderscore(c) || isDigit(c);
}

fn isTwoCharOp(a: u8, b: u8): bool {
    if (a == '=' && b == '=') return true;
    if (a == '!' && b == '=') return true;
    if (a == '<' && b == '=') return true;
    if (a == '>' && b == '=') return true;
    if (a == '-' && b == '>') return true;
    if (a == '=' && b == '>') return true;
    if (a == ':' && b == ':') return true;
    if (a == ':' && b == '=') return true;
    if (a == '&' && b == '&') return true;
    if (a == '|' && b == '|') return true;
    if (a == '+' && b == '+') return true;
    if (a == '-' && b == '-') return true;
    if (a == '+' && b == '=') return true;
    if (a == '-' && b == '=') return true;
    if (a == '*' && b == '=') return true;
    if (a == '/' && b == '=') return true;
    if (a == '?' && b == '?') return true;
    if (a == '.' && b == '.') return true;
    return false;
}

/// Tokenizes `src` into `out_tokens`, skipping whitespace, `//` line comments, and `/* ... */`
/// block comments. Recognizes identifiers, numbers, string literals, `$VAR` metavariables,
/// common multi-character operators (`==`, `!=`, `<=`, `>=`, `->`, `=>`, `::`, etc.), and punctuation.
/// Truncation-safe: writes up to `out_tokens.len` tokens and returns total token count.
pub fn tokenizeCodeLite(src: string, out_tokens: *[]CodeToken): usize {
    let total: usize = 0;
    let pos: usize = 0;
    let line: u32 = 1;

    while (pos < src.len) {
        const c = src[pos];

        // Whitespace
        if (c == ' ' || c == '\t' || c == '\r') {
            pos += 1;
            continue;
        }
        if (c == '\n') {
            pos += 1;
            line += 1;
            continue;
        }

        // Line comment: // ...
        if (c == '/' && pos + 1 < src.len && src[pos + 1] == '/') {
            pos += 2;
            while (pos < src.len && src[pos] != '\n') {
                pos += 1;
            }
            continue;
        }

        // Block comment: /* ... */
        if (c == '/' && pos + 1 < src.len && src[pos + 1] == '*') {
            pos += 2;
            while (pos + 1 < src.len) {
                if (src[pos] == '\n') {
                    line += 1;
                    pos += 1;
                    continue;
                }
                if (src[pos] == '*' && src[pos + 1] == '/') {
                    pos += 2;
                    break;
                }
                pos += 1;
            }
            continue;
        }

        const tok_start = pos;
        const tok_line = line;

        // Metavariable: $NAME
        if (c == '$' && pos + 1 < src.len && isAlphaOrUnderscore(src[pos + 1])) {
            pos += 2;
            while (pos < src.len && isIdentChar(src[pos])) : (pos += 1) {}
            if (total < out_tokens.len) {
                out_tokens[total] = CodeToken{
                    kind: CodeTokenKind.MetaVar,
                    start: tok_start,
                    end: pos,
                    line: tok_line,
                };
            }
            total += 1;
            continue;
        }

        // Identifier
        if (isAlphaOrUnderscore(c)) {
            pos += 1;
            while (pos < src.len && isIdentChar(src[pos])) : (pos += 1) {}
            if (total < out_tokens.len) {
                out_tokens[total] = CodeToken{
                    kind: CodeTokenKind.Ident,
                    start: tok_start,
                    end: pos,
                    line: tok_line,
                };
            }
            total += 1;
            continue;
        }

        // Number literal (decimal, hex/bin suffix, float dot, underscore separators)
        if (isDigit(c)) {
            pos += 1;
            while (pos < src.len && (isIdentChar(src[pos]) || (src[pos] == '.' && pos + 1 < src.len && isDigit(src[pos + 1])))) : (pos += 1) {}
            if (total < out_tokens.len) {
                out_tokens[total] = CodeToken{
                    kind: CodeTokenKind.Number,
                    start: tok_start,
                    end: pos,
                    line: tok_line,
                };
            }
            total += 1;
            continue;
        }

        // String / char / template literal
        if (c == '"' || c == '`' || c == '\'') {
            if (c == '\'') {
                // Only treat single quote as string/char literal if closed on the same line
                let scan: usize = pos + 1;
                let closed: bool = false;
                while (scan < src.len && src[scan] != '\n') {
                    if (src[scan] == '\\' && scan + 1 < src.len) {
                        scan += 2;
                        continue;
                    }
                    if (src[scan] == '\'') {
                        closed = true;
                        scan += 1;
                        break;
                    }
                    scan += 1;
                }
                if (closed) {
                    pos = scan;
                    if (total < out_tokens.len) {
                        out_tokens[total] = CodeToken{
                            kind: CodeTokenKind.String,
                            start: tok_start,
                            end: pos,
                            line: tok_line,
                        };
                    }
                    total += 1;
                    continue;
                }
                // Otherwise emit lone '\'' as Punct (e.g. Rust lifetime)
                pos += 1;
                if (total < out_tokens.len) {
                    out_tokens[total] = CodeToken{
                        kind: CodeTokenKind.Punct,
                        start: tok_start,
                        end: pos,
                        line: tok_line,
                    };
                }
                total += 1;
                continue;
            }

            const quote = c;
            pos += 1;
            while (pos < src.len) {
                const ch = src[pos];
                if (ch == '\\' && pos + 1 < src.len) {
                    if (src[pos + 1] == '\n') {
                        line += 1;
                    }
                    pos += 2;
                    continue;
                }
                if (ch == '\n') {
                    line += 1;
                }
                pos += 1;
                if (ch == quote) break;
            }
            if (total < out_tokens.len) {
                out_tokens[total] = CodeToken{
                    kind: CodeTokenKind.String,
                    start: tok_start,
                    end: pos,
                    line: tok_line,
                };
            }
            total += 1;
            continue;
        }

        // Two-character operator or single-character punctuation
        if (pos + 1 < src.len && isTwoCharOp(c, src[pos + 1])) {
            pos += 2;
        } else {
            pos += 1;
        }
        if (total < out_tokens.len) {
            out_tokens[total] = CodeToken{
                kind: CodeTokenKind.Punct,
                start: tok_start,
                end: pos,
                line: tok_line,
            };
        }
        total += 1;
    }

    return total;
}

/// Finds all non-overlapping token-aware matches of `pattern` in `src`, ignoring whitespace and
/// comments. Supports `$VAR` metavariable captures with cross-position equality constraints
/// (e.g., `$X = $X + 1` requires both occurrences of `$X` to bind to identical token text),
/// plus `$_` wildcard metavariables that match any single token without binding constraints.
///
/// Zero heap allocations. Returns total match count (truncation-safe against `out_hits` / `out_captures`).
/// Returns `0` if `pattern` is empty or exceeds `pat_tok_buf.len`.
pub fn findCodePatternMatches(
    src: string,
    pattern: string,
    src_tok_buf: *[]CodeToken,
    pat_tok_buf: *[]CodeToken,
    out_hits: *[]PatternMatchHit,
    out_captures: *[]MetaCapture,
): usize {
    if (src.len == 0 || pattern.len == 0) return 0;

    const pat_total = tokenizeCodeLite(pattern, pat_tok_buf);
    if (pat_total == 0 || pat_total > pat_tok_buf.len) return 0;

    const src_total = tokenizeCodeLite(src, src_tok_buf);
    const src_len = if (src_total < src_tok_buf.len) src_total else src_tok_buf.len;
    if (src_len < pat_total) return 0;

    let total_hits: usize = 0;
    let cap_written: usize = 0;

    let i: usize = 0;
    while (i + pat_total <= src_len) {
        let local_caps: [MAX_LOCAL_CAPTURES]MetaCapture = [MetaCapture{}; MAX_LOCAL_CAPTURES];
        let local_count: usize = 0;
        let matched: bool = true;

        let j: usize = 0;
        while (j < pat_total) : (j += 1) {
            const pt = pat_tok_buf[j];
            const st = src_tok_buf[i + j];
            const pt_text = pt.slice(pattern);
            const st_text = st.slice(src);

            if (pt.kind == CodeTokenKind.MetaVar) {
                // Wildcard "$_" matches any token without enforcing equality
                if (std.mem.eql(u8, pt_text, "$_")) {
                    continue;
                }
                // Check existing binding for cross-position consistency
                let found_existing: bool = false;
                let k: usize = 0;
                while (k < local_count) : (k += 1) {
                    if (std.mem.eql(u8, local_caps[k].var_name, pt_text)) {
                        found_existing = true;
                        if (!std.mem.eql(u8, local_caps[k].captured_text, st_text)) {
                            matched = false;
                        }
                        break;
                    }
                }
                if (!matched) break;
                if (!found_existing && local_count < MAX_LOCAL_CAPTURES) {
                    local_caps[local_count] = MetaCapture{
                        var_name: pt_text,
                        captured_text: st_text,
                    };
                    local_count += 1;
                }
            } else {
                if (pt.kind != st.kind || !std.mem.eql(u8, pt_text, st_text)) {
                    matched = false;
                    break;
                }
            }
        }

        if (matched) {
            const hit_cap_start = cap_written;
            let hit_cap_written: usize = 0;
            let c_idx: usize = 0;
            while (c_idx < local_count) : (c_idx += 1) {
                if (cap_written < out_captures.len) {
                    out_captures[cap_written] = local_caps[c_idx];
                    cap_written += 1;
                    hit_cap_written += 1;
                }
            }

            if (total_hits < out_hits.len) {
                const first_tok = src_tok_buf[i];
                const last_tok = src_tok_buf[i + pat_total - 1];
                out_hits[total_hits] = PatternMatchHit{
                    start_byte: first_tok.start,
                    end_byte: last_tok.end,
                    start_line: first_tok.line,
                    end_line: last_tok.line,
                    capture_start: hit_cap_start,
                    capture_count: hit_cap_written,
                };
            }
            total_hits += 1;
            i += pat_total;
        } else {
            i += 1;
        }
    }

    return total_hits;
}
