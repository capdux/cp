// Reference-hardened against BurntSushi/ripgrep (MIT/Unlicense)
// Source: cp/lib/globset/glob.cp (Single-Pattern Glob Compiler & Linear-Time NFA Matcher)
// Pure CP glob compiler and linear-time O(NM) NFA matcher supporting *, **, ?,
// character classes ([abc], [a-z], [!0-9], [^a-f]), escapes, options, and
// 16-lane @Vector(16, u8) SIMD hot-path scanners & literal prefix/suffix fast-paths.

const std = @import("std");

type GlobVec16u8 = @Vector(16, u8);

/// Maximum compiled NFA token count per glob pattern (bounds stack state arrays).
pub const GLOB_MAX_TOKENS: usize = 1024;
/// Maximum expanded alternates allowed per `{a,b,...}` brace pattern (prevents combinatorial DoS).
pub const GLOB_MAX_ALTERNATES: usize = 64;
/// Maximum brace nesting depth allowed during `{...}` expansion.
pub const GLOB_MAX_BRACE_DEPTH: usize = 16;

/// Error set returned by glob compilation and GlobSet/IgnoreMatcher operations.
pub err GlobError {
    InvalidPattern,
    UnclosedBracket,
    UnclosedBrace,
    EmptyPattern,
    OutOfMemory,
    TooManyAlternates,
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

fn globLoadVec16(data: []const u8, off: usize): GlobVec16u8 {
    const v: GlobVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD scan returning the first index of `needle` in `haystack`, or `null`.
pub fn simdIndexOfByte(haystack: []const u8, needle: u8): usize? {
    let i: usize = 0;
    const n = haystack.len;
    const v_needle: GlobVec16u8 = @splat(needle);
    while (i + 16 <= n) {
        const chunk = globLoadVec16(haystack, i);
        const mask: u16 = @bitCast(chunk == v_needle);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        if (haystack[i] == needle) return i;
    }
    return null;
}

/// Scalar reference for `simdIndexOfByte` (differential verification).
pub fn simdIndexOfByteScalar(haystack: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < haystack.len) : (i += 1) {
        if (haystack[i] == needle) return i;
    }
    return null;
}

/// 16-lane `@Vector(16, u8)` SIMD scan returning the first index of any of `a`, `b`, `c`, `d` in `haystack`.
pub fn simdIndexOfAny4(haystack: []const u8, a: u8, b: u8, c: u8, d: u8): usize? {
    let i: usize = 0;
    const n = haystack.len;
    const va: GlobVec16u8 = @splat(a);
    const vb: GlobVec16u8 = @splat(b);
    const vc: GlobVec16u8 = @splat(c);
    const vd: GlobVec16u8 = @splat(d);
    while (i + 16 <= n) {
        const chunk = globLoadVec16(haystack, i);
        const ma: u16 = @bitCast(chunk == va);
        const mb: u16 = @bitCast(chunk == vb);
        const mc: u16 = @bitCast(chunk == vc);
        const md: u16 = @bitCast(chunk == vd);
        const mask: u16 = ma | mb | mc | md;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const ch = haystack[i];
        if (ch == a || ch == b || ch == c || ch == d) return i;
    }
    return null;
}

/// Scalar reference for `simdIndexOfAny4` (differential verification).
pub fn simdIndexOfAny4Scalar(haystack: []const u8, a: u8, b: u8, c: u8, d: u8): usize? {
    let i: usize = 0;
    while (i < haystack.len) : (i += 1) {
        const ch = haystack[i];
        if (ch == a || ch == b || ch == c || ch == d) return i;
    }
    return null;
}

/// 16-lane `@Vector(16, u8)` SIMD scan for the first glob metacharacter (`*`, `?`, `[`, `{`, `\`).
pub fn simdIndexOfGlobMagic(pattern: []const u8): usize? {
    let i: usize = 0;
    const n = pattern.len;
    const v_star: GlobVec16u8 = @splat('*');
    const v_q: GlobVec16u8 = @splat('?');
    const v_lb: GlobVec16u8 = @splat('[');
    const v_lc: GlobVec16u8 = @splat('{');
    const v_bs: GlobVec16u8 = @splat('\\');
    while (i + 16 <= n) {
        const chunk = globLoadVec16(pattern, i);
        const m0: u16 = @bitCast(chunk == v_star);
        const m1: u16 = @bitCast(chunk == v_q);
        const m2: u16 = @bitCast(chunk == v_lb);
        const m3: u16 = @bitCast(chunk == v_lc);
        const m4: u16 = @bitCast(chunk == v_bs);
        const mask: u16 = m0 | m1 | m2 | m3 | m4;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const ch = pattern[i];
        if (ch == '*' || ch == '?' || ch == '[' || ch == '{' || ch == '\\') return i;
    }
    return null;
}

/// Scalar reference for `simdIndexOfGlobMagic` (differential verification).
pub fn simdIndexOfGlobMagicScalar(pattern: []const u8): usize? {
    let i: usize = 0;
    while (i < pattern.len) : (i += 1) {
        const ch = pattern[i];
        if (ch == '*' || ch == '?' || ch == '[' || ch == '{' || ch == '\\') return i;
    }
    return null;
}

/// 16-lane `@Vector(16, u8)` SIMD byte-slice equality check for literal prefix/suffix fast-paths.
pub fn simdBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    const n = a.len;
    while (i + 16 <= n) {
        const va = globLoadVec16(a, i);
        const vb = globLoadVec16(b, i);
        if (!@reduce(.And, va == vb)) return false;
        i += 16;
    }
    while (i < n) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Scalar reference for `simdBytesEqual` (differential verification).
pub fn simdBytesEqualScalar(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn globToLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn globToUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

fn globFoldByte(c: u8, case_insensitive: bool): u8 {
    if (case_insensitive) {
        return globToLowerAscii(c);
    }
    return c;
}

fn globZeroBitmap(): [32]u8 {
    let b: [32]u8 = undefined;
    @memset(&b, 0);
    return b;
}

fn globSetBit(bitmap: *[32]u8, c: u8) {
    const idx = (c >> 3) as usize;
    const mask = (1 as u8) << ((c & 7) as u3);
    bitmap[idx] |= mask;
}

fn globClearBit(bitmap: *[32]u8, c: u8) {
    const idx = (c >> 3) as usize;
    const mask = (1 as u8) << ((c & 7) as u3);
    bitmap[idx] &= ~mask;
}

fn globHasBit(bitmap: *const [32]u8, c: u8): bool {
    const idx = (c >> 3) as usize;
    const mask = (1 as u8) << ((c & 7) as u3);
    return (bitmap[idx] & mask) != 0;
}

fn globSetClassByte(bitmap: *[32]u8, c: u8, case_insensitive: bool) {
    globSetBit(bitmap, c);
    if (case_insensitive) {
        globSetBit(bitmap, globToLowerAscii(c));
        globSetBit(bitmap, globToUpperAscii(c));
    }
}

/// Compiled single-pattern glob matcher using a linear-time O(NM) NFA simulation
/// with 16-lane SIMD literal prefix/suffix fast-paths (zero exponential backtracking).
pub struct GlobMatcher {
    alloc: std.mem.Allocator,
    tokens: []Token,
    prefix_bytes: []u8,
    suffix_bytes: []u8,
    is_literal_only: bool,
    opts: GlobOptions,

    /// Compiles `pattern` into a reusable `GlobMatcher`.
    pub static fn compile(alloc: std.mem.Allocator, pattern: string, opts: GlobOptions): GlobMatcher ! GlobError {
        if (pattern.len == 0) {
            return GlobError.EmptyPattern;
        }

        // Fast-path: pure literal pattern (no glob magic bytes) with case-sensitive matching
        if (!opts.case_insensitive && simdIndexOfGlobMagic(pattern) == null) {
            if (pattern.len >= GLOB_MAX_TOKENS) {
                return GlobError.InvalidPattern;
            }
            const lit_copy = alloc.dupe(u8, pattern) catch return GlobError.OutOfMemory;
            return GlobMatcher{
                alloc: alloc,
                tokens: [],
                prefix_bytes: lit_copy,
                suffix_bytes: [],
                is_literal_only: true,
                opts: opts,
            };
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
                    byte: globFoldByte(pattern[i], opts.case_insensitive),
                    bitmap: globZeroBitmap(),
                }) catch return GlobError.OutOfMemory;
                i += 1;
            } else if (c == '?') {
                tokens.append(alloc, Token{
                    kind: .AnyChar,
                    byte: 0,
                    bitmap: globZeroBitmap(),
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

                let bitmap = globZeroBitmap();
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
                            globSetClassByte(&bitmap, ch as u8, opts.case_insensitive);
                        }
                    } else {
                        globSetClassByte(&bitmap, c1, opts.case_insensitive);
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
                }
                // BurntSushi/globset SLASHLIT parity: character classes never match `/` when literal_separator is true
                if (opts.literal_separator) {
                    globClearBit(&bitmap, '/');
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
                        tokens.items[tokens.items.len - 1].byte == '/') ||
                        tokens.items[tokens.items.len - 1].kind == .LeadingDoubleStarSlash;
                    if (at_segment_start && i < pattern.len && pattern[i] == '/') {
                        i += 1;
                        // Collapse redundant consecutive `/**/**/` into a single state
                        if (tokens.items.len == 0 || tokens.items[tokens.items.len - 1].kind != .LeadingDoubleStarSlash) {
                            tokens.append(alloc, Token{
                                kind: .LeadingDoubleStarSlash,
                                byte: 0,
                                bitmap: globZeroBitmap(),
                            }) catch return GlobError.OutOfMemory;
                        }
                    } else {
                        // Collapse redundant consecutive `**` into a single state
                        if (tokens.items.len == 0 || tokens.items[tokens.items.len - 1].kind != .DoubleStar) {
                            tokens.append(alloc, Token{
                                kind: .DoubleStar,
                                byte: 0,
                                bitmap: globZeroBitmap(),
                            }) catch return GlobError.OutOfMemory;
                        }
                    }
                } else {
                    if (tokens.items.len == 0 || tokens.items[tokens.items.len - 1].kind != .Star) {
                        tokens.append(alloc, Token{
                            kind: .Star,
                            byte: 0,
                            bitmap: globZeroBitmap(),
                        }) catch return GlobError.OutOfMemory;
                    }
                    i += 1;
                }
            } else {
                tokens.append(alloc, Token{
                    kind: .Literal,
                    byte: globFoldByte(c, opts.case_insensitive),
                    bitmap: globZeroBitmap(),
                }) catch return GlobError.OutOfMemory;
                i += 1;
            }
        }

        if (tokens.items.len >= GLOB_MAX_TOKENS) {
            return GlobError.InvalidPattern;
        }

        const owned = tokens.toOwnedSlice(alloc) catch return GlobError.OutOfMemory;
        errdefer alloc.free(owned);

        // Extract static literal prefix & suffix for 16-lane SIMD pre-filtering
        let prefix_bytes: []u8 = [];
        let suffix_bytes: []u8 = [];
        let is_literal_only = false;

        if (!opts.case_insensitive && owned.len > 0) {
            let p_len: usize = 0;
            while (p_len < owned.len && owned[p_len].kind == .Literal) : (p_len += 1) {}
            if (p_len > 0) {
                prefix_bytes = alloc.alloc(u8, p_len) catch return GlobError.OutOfMemory;
                let k: usize = 0;
                while (k < p_len) : (k += 1) {
                    prefix_bytes[k] = owned[k].byte;
                }
            }
            if (p_len == owned.len) {
                is_literal_only = true;
            } else {
                let s_len: usize = 0;
                while (s_len < owned.len && owned[owned.len - 1 - s_len].kind == .Literal) : (s_len += 1) {}
                if (s_len > 0) {
                    suffix_bytes = alloc.alloc(u8, s_len) catch {
                        if (prefix_bytes.len > 0) alloc.free(prefix_bytes);
                        return GlobError.OutOfMemory;
                    };
                    let k: usize = 0;
                    while (k < s_len) : (k += 1) {
                        suffix_bytes[k] = owned[owned.len - s_len + k].byte;
                    }
                }
            }
        }

        return GlobMatcher{
            alloc: alloc,
            tokens: owned,
            prefix_bytes: prefix_bytes,
            suffix_bytes: suffix_bytes,
            is_literal_only: is_literal_only,
            opts: opts,
        };
    }

    /// Releases the compiled token slice and SIMD literal buffers.
    pub fn deinit(*_) {
        if (_.tokens.len > 0) {
            _.alloc.free(_.tokens);
            _.tokens = [];
        }
        if (_.prefix_bytes.len > 0) {
            _.alloc.free(_.prefix_bytes);
            _.prefix_bytes = [];
        }
        if (_.suffix_bytes.len > 0) {
            _.alloc.free(_.suffix_bytes);
            _.suffix_bytes = [];
        }
        _.is_literal_only = false;
    }

    /// Returns `true` if `path` matches the compiled glob pattern in strict O(NM) time.
    pub fn matches(path: string): bool {
        // 16-lane SIMD literal fast-paths
        if (_.is_literal_only) {
            return simdBytesEqual(path, _.prefix_bytes);
        }
        if (_.prefix_bytes.len > 0 || _.suffix_bytes.len > 0) {
            if (path.len < _.prefix_bytes.len + _.suffix_bytes.len) {
                return false;
            }
            if (_.prefix_bytes.len > 0 && !simdBytesEqual(path[0.._.prefix_bytes.len], _.prefix_bytes)) {
                return false;
            }
            if (_.suffix_bytes.len > 0 && !simdBytesEqual(path[(path.len - _.suffix_bytes.len)..path.len], _.suffix_bytes)) {
                return false;
            }
        }

        const m = _.tokens.len;
        let curr: [1024]bool = undefined;
        let next: [1024]bool = undefined;
        // `curr_mid[s]` tracks being inside a directory segment consumed by `LeadingDoubleStarSlash`
        // (cannot epsilon-skip to `s + 1` until a literal `/` boundary is consumed).
        let curr_mid: [1024]bool = undefined;
        let next_mid: [1024]bool = undefined;

        @memset(curr[0..(m + 1)], false);
        @memset(curr_mid[0..(m + 1)], false);
        curr[0] = true;

        // Initial epsilon closure (at directory boundary)
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
            const folded = globFoldByte(ch, _.opts.case_insensitive);
            @memset(next[0..(m + 1)], false);
            @memset(next_mid[0..(m + 1)], false);

            s = 0;
            while (s < m) : (s += 1) {
                const tok = _.tokens[s];
                if (curr[s]) {
                    if (tok.kind == .Literal) {
                        if (folded == tok.byte) {
                            next[s + 1] = true;
                        }
                    } else if (tok.kind == .AnyChar) {
                        if (!_.opts.literal_separator || ch != '/') {
                            next[s + 1] = true;
                        }
                    } else if (tok.kind == .CharClass) {
                        if (globHasBit(&tok.bitmap, ch)) {
                            next[s + 1] = true;
                        }
                    } else if (tok.kind == .Star) {
                        if (!_.opts.literal_separator || ch != '/') {
                            next[s] = true;
                        }
                    } else if (tok.kind == .DoubleStar) {
                        next[s] = true;
                    } else if (tok.kind == .LeadingDoubleStarSlash) {
                        next_mid[s] = true;
                        if (ch == '/') {
                            next[s] = true;
                        }
                    }
                }
                if (curr_mid[s]) {
                    next_mid[s] = true;
                    if (ch == '/') {
                        next[s] = true;
                    }
                }
            }

            // Epsilon closure on `next` (only directory-boundary states may skip `**/`)
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
            @memcpy(curr_mid[0..(m + 1)], next_mid[0..(m + 1)]);
        }

        return curr[m];
    }
}
