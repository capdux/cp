// Source: cp/lib/globset/glob.cp (Single-Pattern Glob Compiler & NFA Matcher)
// Pure CP glob compiler and zero-allocation NFA matcher supporting *, **, ?,
// character classes ([abc], [a-z], [!0-9], [^a-f]), escapes, and options.

const std = @import("std");

/// Error set returned by glob compilation and GlobSet/IgnoreMatcher operations.
pub err GlobError {
    InvalidPattern,
    UnclosedBracket,
    UnclosedBrace,
    EmptyPattern,
    OutOfMemory,
}

/// Configuration options for `GlobMatcher` and `GlobSetBuilder`.
pub struct GlobOptions {
    case_insensitive: bool = false,
    literal_separator: bool = true,
}

const TokenKind = enum {
    Literal,
    AnyChar,
    CharClass,
    Star,
    DoubleStar,
    LeadingDoubleStarSlash,
};

const Token = struct {
    kind: TokenKind,
    byte: u8,
    bitmap: [32]u8,
};

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn toUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

fn foldByte(c: u8, case_insensitive: bool): u8 {
    if (case_insensitive) {
        return toLowerAscii(c);
    }
    return c;
}

fn zeroBitmap(): [32]u8 {
    let b: [32]u8 = undefined;
    @memset(&b, 0);
    return b;
}

fn setBit(bitmap: *[32]u8, c: u8) {
    const idx = (c >> 3) as usize;
    const mask = (1 as u8) << ((c & 7) as u3);
    bitmap[idx] |= mask;
}

fn clearBit(bitmap: *[32]u8, c: u8) {
    const idx = (c >> 3) as usize;
    const mask = (1 as u8) << ((c & 7) as u3);
    bitmap[idx] &= ~mask;
}

fn hasBit(bitmap: *const [32]u8, c: u8): bool {
    const idx = (c >> 3) as usize;
    const mask = (1 as u8) << ((c & 7) as u3);
    return (bitmap[idx] & mask) != 0;
}

fn setClassByte(bitmap: *[32]u8, c: u8, case_insensitive: bool) {
    setBit(bitmap, c);
    if (case_insensitive) {
        setBit(bitmap, toLowerAscii(c));
        setBit(bitmap, toUpperAscii(c));
    }
}

/// Compiled single-pattern glob matcher using a linear-time NFA simulation.
pub struct GlobMatcher {
    alloc: std.mem.Allocator,
    tokens: []Token,
    opts: GlobOptions,

    /// Compiles `pattern` into a reusable `GlobMatcher`.
    pub fn compile(alloc: std.mem.Allocator, pattern: string, opts: GlobOptions): GlobMatcher ! GlobError {
        if (pattern.len == 0) {
            return GlobError.EmptyPattern;
        }

        let tokens: std.ArrayList(Token) = .empty;
        errdefer tokens.deinit(alloc);

        let i: usize = 0;
        while (i < pattern.len) {
            const c = pattern[i];
            if (c == '\\') {
                if (i + 1 >= pattern.len) {
                    return GlobError.InvalidPattern;
                }
                i += 1;
                tokens.append(alloc, Token{
                    kind: .Literal,
                    byte: foldByte(pattern[i], opts.case_insensitive),
                    bitmap: zeroBitmap(),
                }) catch return GlobError.OutOfMemory;
                i += 1;
            } else if (c == '?') {
                tokens.append(alloc, Token{
                    kind: .AnyChar,
                    byte: 0,
                    bitmap: zeroBitmap(),
                }) catch return GlobError.OutOfMemory;
                i += 1;
            } else if (c == '[') {
                let j = i + 1;
                let negated = false;
                if (j < pattern.len && (pattern[j] == '!' || pattern[j] == '^')) {
                    negated = true;
                    j += 1;
                }
                if (j >= pattern.len) {
                    return GlobError.UnclosedBracket;
                }
                if (pattern[j] == ']') {
                    return GlobError.InvalidPattern;
                }

                let bitmap = zeroBitmap();
                while (j < pattern.len && pattern[j] != ']') {
                    let c1: u8 = 0;
                    if (pattern[j] == '\\') {
                        if (j + 1 >= pattern.len) {
                            return GlobError.UnclosedBracket;
                        }
                        j += 1;
                        c1 = pattern[j];
                        j += 1;
                    } else {
                        c1 = pattern[j];
                        j += 1;
                    }

                    if (j + 1 < pattern.len && pattern[j] == '-' && pattern[j + 1] != ']') {
                        j += 1;
                        let c2: u8 = 0;
                        if (pattern[j] == '\\') {
                            if (j + 1 >= pattern.len) {
                                return GlobError.UnclosedBracket;
                            }
                            j += 1;
                            c2 = pattern[j];
                            j += 1;
                        } else {
                            c2 = pattern[j];
                            j += 1;
                        }
                        if (c1 > c2) {
                            return GlobError.InvalidPattern;
                        }
                        let ch: u16 = c1 as u16;
                        while (ch <= (c2 as u16)) : (ch += 1) {
                            setClassByte(&bitmap, ch as u8, opts.case_insensitive);
                        }
                    } else {
                        setClassByte(&bitmap, c1, opts.case_insensitive);
                    }
                }

                if (j >= pattern.len || pattern[j] != ']') {
                    return GlobError.UnclosedBracket;
                }

                if (negated) {
                    let k: usize = 0;
                    while (k < 32) : (k += 1) {
                        bitmap[k] = ~bitmap[k];
                    }
                    if (opts.literal_separator) {
                        clearBit(&bitmap, '/');
                    }
                }

                tokens.append(alloc, Token{
                    kind: .CharClass,
                    byte: 0,
                    bitmap: bitmap,
                }) catch return GlobError.OutOfMemory;
                i = j + 1;
            } else if (c == '*') {
                if (i + 1 < pattern.len && pattern[i + 1] == '*') {
                    i += 2;
                    while (i < pattern.len && pattern[i] == '*') {
                        i += 1;
                    }
                    const at_segment_start = tokens.items.len == 0 ||
                        (tokens.items[tokens.items.len - 1].kind == .Literal &&
                        tokens.items[tokens.items.len - 1].byte == '/');
                    if (at_segment_start && i < pattern.len && pattern[i] == '/') {
                        i += 1;
                        tokens.append(alloc, Token{
                            kind: .LeadingDoubleStarSlash,
                            byte: 0,
                            bitmap: zeroBitmap(),
                        }) catch return GlobError.OutOfMemory;
                    } else {
                        tokens.append(alloc, Token{
                            kind: .DoubleStar,
                            byte: 0,
                            bitmap: zeroBitmap(),
                        }) catch return GlobError.OutOfMemory;
                    }
                } else {
                    tokens.append(alloc, Token{
                        kind: .Star,
                        byte: 0,
                        bitmap: zeroBitmap(),
                    }) catch return GlobError.OutOfMemory;
                    i += 1;
                }
            } else {
                tokens.append(alloc, Token{
                    kind: .Literal,
                    byte: foldByte(c, opts.case_insensitive),
                    bitmap: zeroBitmap(),
                }) catch return GlobError.OutOfMemory;
                i += 1;
            }
        }

        if (tokens.items.len >= 1024) {
            return GlobError.InvalidPattern;
        }

        const owned = tokens.toOwnedSlice(alloc) catch return GlobError.OutOfMemory;
        return GlobMatcher{
            alloc: alloc,
            tokens: owned,
            opts: opts,
        };
    }

    /// Releases the compiled token slice.
    pub fn deinit(*_) {
        if (_.tokens.len > 0) {
            _.alloc.free(_.tokens);
            _.tokens = [];
        }
    }

    /// Returns `true` if `path` matches the compiled glob pattern.
    pub fn matches(path: string): bool {
        const m = _.tokens.len;
        let curr: [1024]bool = undefined;
        let next: [1024]bool = undefined;
        @memset(curr[0..(m + 1)], false);
        curr[0] = true;

        // Epsilon closure
        let s: usize = 0;
        while (s < m) : (s += 1) {
            if (curr[s]) {
                const k = _.tokens[s].kind;
                if (k == .Star || k == .DoubleStar || k == .LeadingDoubleStarSlash) {
                    curr[s + 1] = true;
                }
            }
        }

        let p_idx: usize = 0;
        while (p_idx < path.len) : (p_idx += 1) {
            const ch = path[p_idx];
            const folded = foldByte(ch, _.opts.case_insensitive);
            @memset(next[0..(m + 1)], false);

            s = 0;
            while (s < m) : (s += 1) {
                if (!curr[s]) {
                    continue;
                }
                const tok = _.tokens[s];
                if (tok.kind == .Literal) {
                    if (folded == tok.byte) {
                        next[s + 1] = true;
                    }
                } else if (tok.kind == .AnyChar) {
                    if (!_.opts.literal_separator || ch != '/') {
                        next[s + 1] = true;
                    }
                } else if (tok.kind == .CharClass) {
                    if (hasBit(&tok.bitmap, ch)) {
                        next[s + 1] = true;
                    }
                } else if (tok.kind == .Star) {
                    if (!_.opts.literal_separator || ch != '/') {
                        next[s] = true;
                    }
                } else if (tok.kind == .DoubleStar) {
                    next[s] = true;
                } else if (tok.kind == .LeadingDoubleStarSlash) {
                    next[s] = true;
                    if (ch == '/') {
                        next[s + 1] = true;
                    }
                }
            }

            // Epsilon closure on `next`
            s = 0;
            while (s < m) : (s += 1) {
                if (next[s]) {
                    const k = _.tokens[s].kind;
                    if (k == .Star || k == .DoubleStar || k == .LeadingDoubleStarSlash) {
                        next[s + 1] = true;
                    }
                }
            }

            @memcpy(curr[0..(m + 1)], next[0..(m + 1)]);
        }

        return curr[m];
    }
}
