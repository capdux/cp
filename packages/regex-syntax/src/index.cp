// @cap/regex-syntax — Zero-allocation regular expression syntax parser & validator for CP
// Clean-room MIT implementation (Rust `regex-syntax` v0.8 parity).
// Provides metacharacter detection, escaping, stack-based syntax validation,
// capture/anchor/min-length analysis, and static literal prefix extraction.

pub const RSX_OK: i32 = 0;
pub const RSX_ERR_UNCLOSED_GROUP: i32 = 1;
pub const RSX_ERR_UNOPENED_GROUP: i32 = 2;
pub const RSX_ERR_UNCLOSED_CLASS: i32 = 3;
pub const RSX_ERR_INVALID_CLASS_RANGE: i32 = 4;
pub const RSX_ERR_EMPTY_CLASS: i32 = 5;
pub const RSX_ERR_DANGLING_QUANTIFIER: i32 = 6;
pub const RSX_ERR_DOUBLE_QUANTIFIER: i32 = 7;
pub const RSX_ERR_INVALID_QUANTIFIER: i32 = 8;
pub const RSX_ERR_TRAILING_BACKSLASH: i32 = 9;
pub const RSX_ERR_INVALID_ESCAPE: i32 = 10;
pub const RSX_ERR_INVALID_GROUP_HEADER: i32 = 11;
pub const RSX_ERR_NESTING_LIMIT: i32 = 12;

/// Summary returned by `validateRegexSyntax`.
pub struct RegexSyntaxInfo {
    pub valid: bool,
    pub err_code: i32,
    pub err_pos: usize,
    pub capture_count: usize,
    pub has_anchors: bool,
    pub has_backref: bool,
    pub min_len: usize,
    pub literal_prefix_len: usize,

    pub fn isOk(): bool {
        return _.valid && _.err_code == 0;
    }
}

struct RsxStep {
    ok: bool,
    err_code: i32,
    err_pos: usize,
    next_pos: usize,
    val_a: usize,
    val_b: usize,
    flag: bool
}

// ============================================================================
// Public Helpers — Metacharacters, Escaping, Word Bytes
// ============================================================================

/// Returns `true` if `c` is a regex metacharacter (exact parity with `regex_syntax::is_meta_character`).
/// Covers: `\` `.` `+` `*` `?` `(` `)` `|` `[` `]` `{` `}` `^` `$` `#` `&` `-` `~`.
pub fn isMetaCharacter(c: u8): bool {
    if (c == 0x5C || c == 0x2E || c == 0x2B || c == 0x2A || c == 0x3F) {
        return true;
    }
    if (c == 0x28 || c == 0x29 || c == 0x7C || c == 0x5B || c == 0x5D) {
        return true;
    }
    if (c == 0x7B || c == 0x7D || c == 0x5E || c == 0x24) {
        return true;
    }
    if (c == 0x23 || c == 0x26 || c == 0x2D || c == 0x7E) {
        return true;
    }
    return false;
}

/// Escapes all regex metacharacters in `input`, writing into `out` (parity with `regex_syntax::escape`).
/// Returns the number of bytes written to `out`.
pub fn escapeRegex(input: string, out: []u8): usize {
    let written: usize = 0;
    let i: usize = 0;
    while (i < input.len && written < out.len) {
        let b = input[i];
        if (isMetaCharacter(b)) {
            if (written + 2 > out.len) {
                break;
            }
            out[written] = 0x5C;
            out[written + 1] = b;
            written = written + 2;
        } else {
            out[written] = b;
            written = written + 1;
        }
        i = i + 1;
    }
    return written;
}

/// Returns `true` if `b` is an ASCII word byte (`[A-Za-z0-9_]`).
pub fn isWordByte(b: u8): bool {
    return (b >= 65 && b <= 90)
        || (b >= 97 && b <= 122)
        || (b >= 48 && b <= 57)
        || b == 95;
}

// ============================================================================
// Private Helpers (`rsx` prefix to avoid module-flattening symbol collisions)
// ============================================================================

fn rsxIsDigit(b: u8): bool {
    return b >= 48 && b <= 57;
}

fn rsxHexVal(b: u8): i32 {
    if (b >= 48 && b <= 57) {
        return (b - 48) as i32;
    }
    if (b >= 65 && b <= 70) {
        return (b - 55) as i32;
    }
    if (b >= 97 && b <= 102) {
        return (b - 87) as i32;
    }
    return -1;
}

fn rsxIsIdentStart(b: u8): bool {
    return (b >= 65 && b <= 90) || (b >= 97 && b <= 122) || b == 95;
}

fn rsxIsFlagChar(b: u8): bool {
    return b == 0x69 // 'i'
        || b == 0x6D // 'm'
        || b == 0x73 // 's'
        || b == 0x55 // 'U'
        || b == 0x75 // 'u'
        || b == 0x78 // 'x'
        || b == 0x52; // 'R'
}

fn rsxMakeError(code: i32, pos: usize, caps: usize, anchors: bool, backref: bool): RegexSyntaxInfo {
    return RegexSyntaxInfo {
        valid: false,
        err_code: code,
        err_pos: pos,
        capture_count: caps,
        has_anchors: anchors,
        has_backref: backref,
        min_len: 0,
        literal_prefix_len: 0
    };
}

/// Parses an escape sequence starting at `pattern[pos] == '\\'`.
/// Returns:
///   val_a = kind (0 = literal byte, 1 = char class shorthand, 2 = anchor, 3 = backref)
///   val_b = decoded byte value (when kind == 0)
fn rsxParseEscape(pattern: string, pos: usize): RsxStep {
    if (pos + 1 >= pattern.len) {
        return RsxStep { ok: false, err_code: RSX_ERR_TRAILING_BACKSLASH, err_pos: pos, next_pos: pos, val_a: 0, val_b: 0, flag: false };
    }
    let esc = pattern[pos + 1];

    // Metacharacters and common ASCII punctuation escapes
    if (isMetaCharacter(esc) || esc == 0x2F || esc == 0x3A || esc == 0x3B || esc == 0x2C || esc == 0x20 || esc == 0x22 || esc == 0x27 || esc == 0x3C || esc == 0x3E || esc == 0x3D || esc == 0x21 || esc == 0x25 || esc == 0x40 || esc == 0x60) {
        return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 2, val_a: 0, val_b: esc as usize, flag: false };
    }

    // Standard control escapes
    if (esc == 0x6E) { return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 2, val_a: 0, val_b: 0x0A, flag: false }; } // \n
    if (esc == 0x72) { return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 2, val_a: 0, val_b: 0x0D, flag: false }; } // \r
    if (esc == 0x74) { return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 2, val_a: 0, val_b: 0x09, flag: false }; } // \t
    if (esc == 0x66) { return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 2, val_a: 0, val_b: 0x0C, flag: false }; } // \f
    if (esc == 0x76) { return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 2, val_a: 0, val_b: 0x0B, flag: false }; } // \v
    if (esc == 0x61) { return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 2, val_a: 0, val_b: 0x07, flag: false }; } // \a
    if (esc == 0x30) { return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 2, val_a: 0, val_b: 0x00, flag: false }; } // \0

    // Character class shorthands: \d \D \w \W \s \S
    if (esc == 0x64 || esc == 0x44 || esc == 0x77 || esc == 0x57 || esc == 0x73 || esc == 0x53) {
        return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 2, val_a: 1, val_b: 0, flag: false };
    }

    // Anchors & word boundaries: \b \B \A \z \Z
    if (esc == 0x62 || esc == 0x42 || esc == 0x41 || esc == 0x7A || esc == 0x5A) {
        return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 2, val_a: 2, val_b: esc as usize, flag: false };
    }

    // Backreferences \1..\9
    if (esc >= 49 && esc <= 57) {
        return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 2, val_a: 3, val_b: (esc - 48) as usize, flag: false };
    }

    // Hex escape \xHH
    if (esc == 0x78) {
        if (pos + 3 >= pattern.len) {
            return RsxStep { ok: false, err_code: RSX_ERR_INVALID_ESCAPE, err_pos: pos, next_pos: pos, val_a: 0, val_b: 0, flag: false };
        }
        let h1 = rsxHexVal(pattern[pos + 2]);
        let h2 = rsxHexVal(pattern[pos + 3]);
        if (h1 < 0 || h2 < 0) {
            return RsxStep { ok: false, err_code: RSX_ERR_INVALID_ESCAPE, err_pos: pos, next_pos: pos, val_a: 0, val_b: 0, flag: false };
        }
        let bv = ((h1 << 4) | h2) as usize;
        return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 4, val_a: 0, val_b: bv, flag: false };
    }

    return RsxStep { ok: false, err_code: RSX_ERR_INVALID_ESCAPE, err_pos: pos, next_pos: pos, val_a: 0, val_b: 0, flag: false };
}

/// Parses a single item inside `[...]` at `pos`.
/// Returns: val_a = 1 if single scalar byte (eligible for range), 0 if class shorthand; val_b = byte value.
fn rsxParseClassItem(pattern: string, pos: usize): RsxStep {
    if (pattern[pos] == 0x5C) {
        let esc = rsxParseEscape(pattern, pos);
        if (!esc.ok) {
            return esc;
        }
        if (esc.val_a == 2 || esc.val_a == 3) {
            return RsxStep { ok: false, err_code: RSX_ERR_INVALID_ESCAPE, err_pos: pos, next_pos: pos, val_a: 0, val_b: 0, flag: false };
        }
        let isScalar: usize = 0;
        if (esc.val_a == 0) {
            isScalar = 1;
        }
        return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: esc.next_pos, val_a: isScalar, val_b: esc.val_b, flag: false };
    }
    return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: pos + 1, val_a: 1, val_b: pattern[pos] as usize, flag: false };
}

/// Parses and validates a character class `[...]` starting at `pattern[pos] == '['`.
fn rsxParseCharClass(pattern: string, start_pos: usize): RsxStep {
    let len = pattern.len;
    let i = start_pos + 1;
    if (i >= len) {
        return RsxStep { ok: false, err_code: RSX_ERR_UNCLOSED_CLASS, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
    }
    if (pattern[i] == 0x5E) { // '^' negation
        i = i + 1;
        if (i >= len) {
            return RsxStep { ok: false, err_code: RSX_ERR_UNCLOSED_CLASS, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
        }
    }

    let count: usize = 0;
    let has_prev_scalar = false;
    let prev_scalar: usize = 0;

    while (i < len) {
        let b = pattern[i];
        // A ']' closes the class unless it is the very first item
        if (b == 0x5D && count > 0) {
            return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: i + 1, val_a: count, val_b: 0, flag: false };
        }

        // Check for range `prev-next` when '-' is not the final char before ']'
        if (b == 0x2D && has_prev_scalar && i + 1 < len && pattern[i + 1] != 0x5D) {
            let rhs = rsxParseClassItem(pattern, i + 1);
            if (!rhs.ok) {
                return rhs;
            }
            if (rhs.val_a == 0) {
                return RsxStep { ok: false, err_code: RSX_ERR_INVALID_CLASS_RANGE, err_pos: i, next_pos: i, val_a: 0, val_b: 0, flag: false };
            }
            if (prev_scalar > rhs.val_b) {
                return RsxStep { ok: false, err_code: RSX_ERR_INVALID_CLASS_RANGE, err_pos: i, next_pos: i, val_a: 0, val_b: 0, flag: false };
            }
            i = rhs.next_pos;
            has_prev_scalar = false;
            count = count + 1;
            continue;
        }

        let item = rsxParseClassItem(pattern, i);
        if (!item.ok) {
            return item;
        }
        i = item.next_pos;
        count = count + 1;
        if (item.val_a == 1) {
            has_prev_scalar = true;
            prev_scalar = item.val_b;
        } else {
            has_prev_scalar = false;
        }
    }

    return RsxStep { ok: false, err_code: RSX_ERR_UNCLOSED_CLASS, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
}

/// Parses a counted quantifier `{m}`, `{m,}`, or `{m,n}` starting at `pattern[pos] == '{'`.
/// Returns: val_a = min_rep, val_b = max_rep (if flag == true), next_pos = after '}' (and optional lazy '?').
fn rsxParseBraceQuant(pattern: string, start_pos: usize): RsxStep {
    let len = pattern.len;
    let i = start_pos + 1;
    if (i >= len || !rsxIsDigit(pattern[i])) {
        return RsxStep { ok: false, err_code: RSX_ERR_INVALID_QUANTIFIER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
    }
    let min_rep: usize = 0;
    while (i < len && rsxIsDigit(pattern[i])) {
        min_rep = min_rep * 10 + ((pattern[i] - 48) as usize);
        i = i + 1;
    }
    if (i >= len) {
        return RsxStep { ok: false, err_code: RSX_ERR_INVALID_QUANTIFIER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
    }

    let has_max = true;
    let max_rep = min_rep;
    if (pattern[i] == 0x2C) { // ','
        i = i + 1;
        if (i >= len) {
            return RsxStep { ok: false, err_code: RSX_ERR_INVALID_QUANTIFIER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
        }
        if (pattern[i] == 0x7D) { // '}' -> {m,}
            has_max = false;
            max_rep = 0;
        } else if (rsxIsDigit(pattern[i])) {
            max_rep = 0;
            while (i < len && rsxIsDigit(pattern[i])) {
                max_rep = max_rep * 10 + ((pattern[i] - 48) as usize);
                i = i + 1;
            }
            if (min_rep > max_rep) {
                return RsxStep { ok: false, err_code: RSX_ERR_INVALID_QUANTIFIER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
            }
        } else {
            return RsxStep { ok: false, err_code: RSX_ERR_INVALID_QUANTIFIER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
        }
    }

    if (i >= len || pattern[i] != 0x7D) {
        return RsxStep { ok: false, err_code: RSX_ERR_INVALID_QUANTIFIER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
    }
    i = i + 1;
    // Optional non-greedy '?'
    if (i < len && pattern[i] == 0x3F) {
        i = i + 1;
    }
    return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: i, val_a: min_rep, val_b: max_rep, flag: has_max };
}

/// Parses an extended group header starting right after `(?` at `pos`.
/// Returns: val_a = 1 if capturing, 0 if non-capturing; flag = true if self-closing inline flags `(?i)`.
fn rsxParseGroupHeader(pattern: string, start_pos: usize): RsxStep {
    let len = pattern.len;
    let i = start_pos;
    if (i >= len) {
        return RsxStep { ok: false, err_code: RSX_ERR_INVALID_GROUP_HEADER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
    }

    // Non-capturing (?:...)
    if (pattern[i] == 0x3A) {
        return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: i + 1, val_a: 0, val_b: 0, flag: false };
    }

    // Named capture (?P<name>...) or (?<name>...)
    if (pattern[i] == 0x50 || pattern[i] == 0x3C) {
        if (pattern[i] == 0x50) {
            i = i + 1;
            if (i >= len || pattern[i] != 0x3C) {
                return RsxStep { ok: false, err_code: RSX_ERR_INVALID_GROUP_HEADER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
            }
        }
        i = i + 1; // skip '<'
        if (i >= len || !rsxIsIdentStart(pattern[i])) {
            return RsxStep { ok: false, err_code: RSX_ERR_INVALID_GROUP_HEADER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
        }
        while (i < len && isWordByte(pattern[i])) {
            i = i + 1;
        }
        if (i >= len || pattern[i] != 0x3E) {
            return RsxStep { ok: false, err_code: RSX_ERR_INVALID_GROUP_HEADER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
        }
        return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: i + 1, val_a: 1, val_b: 0, flag: false };
    }

    // Flags (?imsUux-imsUux:...) or (?i)
    let seen_flag = false;
    let seen_dash = false;
    while (i < len) {
        let b = pattern[i];
        if (rsxIsFlagChar(b)) {
            seen_flag = true;
            i = i + 1;
        } else if (b == 0x2D && !seen_dash) {
            seen_dash = true;
            i = i + 1;
        } else {
            break;
        }
    }
    if (!seen_flag || i >= len) {
        return RsxStep { ok: false, err_code: RSX_ERR_INVALID_GROUP_HEADER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
    }
    if (pattern[i] == 0x3A) { // ':' scoped flag group
        return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: i + 1, val_a: 0, val_b: 0, flag: false };
    }
    if (pattern[i] == 0x29) { // ')' inline flags
        return RsxStep { ok: true, err_code: RSX_OK, err_pos: 0, next_pos: i + 1, val_a: 0, val_b: 0, flag: true };
    }
    return RsxStep { ok: false, err_code: RSX_ERR_INVALID_GROUP_HEADER, err_pos: start_pos, next_pos: start_pos, val_a: 0, val_b: 0, flag: false };
}

/// Checks if `pattern` has a top-level alternation `|` outside of any group or character class.
fn rsxHasTopLevelAlt(pattern: string): bool {
    let i: usize = 0;
    let depth: usize = 0;
    let in_class = false;
    let class_start: usize = 0;
    while (i < pattern.len) {
        let b = pattern[i];
        if (b == 0x5C) {
            i = i + 2;
            continue;
        }
        if (in_class) {
            if (b == 0x5D && i > class_start + 1) {
                in_class = false;
            }
            i = i + 1;
            continue;
        }
        if (b == 0x5B) {
            in_class = true;
            class_start = i;
            if (i + 1 < pattern.len && pattern[i + 1] == 0x5E) {
                class_start = i + 1;
            }
            i = i + 1;
            continue;
        }
        if (b == 0x28) {
            depth = depth + 1;
        } else if (b == 0x29) {
            if (depth > 0) {
                depth = depth - 1;
            }
        } else if (b == 0x7C && depth == 0) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

/// Core literal prefix extractor. When `write_out` is true, writes bytes into `out`.
/// Returns total prefix length (or bytes written when bounded by `out.len`).
fn rsxExtractPrefixCore(pattern: string, out: []u8, write_out: bool): usize {
    if (pattern.len == 0 || rsxHasTopLevelAlt(pattern)) {
        return 0;
    }
    let i: usize = 0;
    // Skip leading start-of-text anchors (`^` or `\A`)
    while (i < pattern.len) {
        if (pattern[i] == 0x5E) {
            i = i + 1;
        } else if (pattern[i] == 0x5C && i + 1 < pattern.len && pattern[i + 1] == 0x41) {
            i = i + 2;
        } else {
            break;
        }
    }

    let count: usize = 0;
    while (i < pattern.len) {
        let b = pattern[i];
        let lit_byte: u8 = 0;
        let next_i: usize = i;

        if (b == 0x5C) {
            let esc = rsxParseEscape(pattern, i);
            if (!esc.ok || esc.val_a != 0) {
                break;
            }
            lit_byte = esc.val_b as u8;
            next_i = esc.next_pos;
        } else if (b == 0x2E || b == 0x5B || b == 0x28 || b == 0x29 || b == 0x7C || b == 0x24 || b == 0x5E || b == 0x2A || b == 0x2B || b == 0x3F || b == 0x7B) {
            break;
        } else {
            lit_byte = b;
            next_i = i + 1;
        }

        // Look ahead for quantifier on this literal byte
        let must_stop = false;
        let is_optional = false;
        if (next_i < pattern.len) {
            let q = pattern[next_i];
            if (q == 0x3F || q == 0x2A) { // '?' or '*' -> 0 repetitions allowed
                is_optional = true;
                must_stop = true;
            } else if (q == 0x2B) { // '+' -> at least 1 occurrence, then stop
                must_stop = true;
            } else if (q == 0x7B) {
                let bq = rsxParseBraceQuant(pattern, next_i);
                if (!bq.ok || bq.val_a == 0) {
                    is_optional = true;
                    must_stop = true;
                } else {
                    must_stop = true;
                }
            }
        }

        if (is_optional) {
            break;
        }

        if (write_out) {
            if (count >= out.len) {
                break;
            }
            out[count] = lit_byte;
        }
        count = count + 1;
        i = next_i;

        if (must_stop) {
            break;
        }
    }

    return count;
}

// ============================================================================
// Public API — Validation & Literal Prefix Extraction
// ============================================================================

/// Extracts the mandatory static literal byte prefix from the beginning of `pattern` into `out`.
/// Skips leading `^`/`\A`, unescapes literal escapes (e.g. `\.`), and rolls back optional atoms (`a?`, `a*`, `a{0,n}`).
/// Returns the number of bytes written to `out`.
pub fn extractLiteralPrefix(pattern: string, out: []u8): usize {
    return rsxExtractPrefixCore(pattern, out, true);
}

/// Validates `pattern` syntax with zero heap allocation and computes capture count, anchor/backref flags,
/// minimum match length, and static literal prefix length.
pub fn validateRegexSyntax(pattern: string): RegexSyntaxInfo {
    let len = pattern.len;
    let cur_branch_min: [65]usize = undefined;
    let group_alt_min: [65]usize = undefined;
    let group_has_alt: [65]bool = undefined;
    let group_start_pos: [65]usize = undefined;
    cur_branch_min[0] = 0;
    group_alt_min[0] = 0;
    group_has_alt[0] = false;

    let depth: usize = 0;
    let capture_count: usize = 0;
    let has_anchors = false;
    let has_backref = false;
    let can_quantify = false;
    let last_was_quant = false;
    let last_atom_min: usize = 0;

    let i: usize = 0;
    while (i < len) {
        let b = pattern[i];

        if (b == 0x5C) { // '\' escape
            let esc = rsxParseEscape(pattern, i);
            if (!esc.ok) {
                return rsxMakeError(esc.err_code, esc.err_pos, capture_count, has_anchors, has_backref);
            }
            if (esc.val_a == 2) {
                has_anchors = true;
                can_quantify = false;
                last_was_quant = false;
                last_atom_min = 0;
            } else if (esc.val_a == 3) {
                has_backref = true;
                can_quantify = true;
                last_was_quant = false;
                last_atom_min = 0;
            } else {
                cur_branch_min[depth] = cur_branch_min[depth] + 1;
                can_quantify = true;
                last_was_quant = false;
                last_atom_min = 1;
            }
            i = esc.next_pos;
            continue;
        }

        if (b == 0x28) { // '(' group open
            if (depth >= 64) {
                return rsxMakeError(RSX_ERR_NESTING_LIMIT, i, capture_count, has_anchors, has_backref);
            }
            if (i + 1 < len && pattern[i + 1] == 0x3F) {
                let hdr = rsxParseGroupHeader(pattern, i + 2);
                if (!hdr.ok) {
                    return rsxMakeError(hdr.err_code, hdr.err_pos, capture_count, has_anchors, has_backref);
                }
                if (hdr.flag) {
                    // Self-closing inline flags like `(?i)`
                    i = hdr.next_pos;
                    can_quantify = false;
                    last_was_quant = false;
                    last_atom_min = 0;
                    continue;
                }
                capture_count = capture_count + hdr.val_a;
                depth = depth + 1;
                cur_branch_min[depth] = 0;
                group_alt_min[depth] = 0;
                group_has_alt[depth] = false;
                group_start_pos[depth] = i;
                i = hdr.next_pos;
            } else {
                capture_count = capture_count + 1;
                depth = depth + 1;
                cur_branch_min[depth] = 0;
                group_alt_min[depth] = 0;
                group_has_alt[depth] = false;
                group_start_pos[depth] = i;
                i = i + 1;
            }
            can_quantify = false;
            last_was_quant = false;
            last_atom_min = 0;
            continue;
        }

        if (b == 0x29) { // ')' group close
            if (depth == 0) {
                return rsxMakeError(RSX_ERR_UNOPENED_GROUP, i, capture_count, has_anchors, has_backref);
            }
            let gmin = cur_branch_min[depth];
            if (group_has_alt[depth] && group_alt_min[depth] < gmin) {
                gmin = group_alt_min[depth];
            }
            depth = depth - 1;
            cur_branch_min[depth] = cur_branch_min[depth] + gmin;
            can_quantify = true;
            last_was_quant = false;
            last_atom_min = gmin;
            i = i + 1;
            continue;
        }

        if (b == 0x7C) { // '|' alternation
            if (!group_has_alt[depth]) {
                group_alt_min[depth] = cur_branch_min[depth];
                group_has_alt[depth] = true;
            } else if (cur_branch_min[depth] < group_alt_min[depth]) {
                group_alt_min[depth] = cur_branch_min[depth];
            }
            cur_branch_min[depth] = 0;
            can_quantify = false;
            last_was_quant = false;
            last_atom_min = 0;
            i = i + 1;
            continue;
        }

        if (b == 0x5B) { // '[' character class
            let cc = rsxParseCharClass(pattern, i);
            if (!cc.ok) {
                return rsxMakeError(cc.err_code, cc.err_pos, capture_count, has_anchors, has_backref);
            }
            cur_branch_min[depth] = cur_branch_min[depth] + 1;
            can_quantify = true;
            last_was_quant = false;
            last_atom_min = 1;
            i = cc.next_pos;
            continue;
        }

        if (b == 0x2A || b == 0x2B || b == 0x3F) { // '*', '+', '?'
            if (last_was_quant) {
                return rsxMakeError(RSX_ERR_DOUBLE_QUANTIFIER, i, capture_count, has_anchors, has_backref);
            }
            if (!can_quantify) {
                return rsxMakeError(RSX_ERR_DANGLING_QUANTIFIER, i, capture_count, has_anchors, has_backref);
            }
            if (b == 0x2A || b == 0x3F) {
                if (cur_branch_min[depth] >= last_atom_min) {
                    cur_branch_min[depth] = cur_branch_min[depth] - last_atom_min;
                }
                last_atom_min = 0;
            }
            i = i + 1;
            // Optional non-greedy '?'
            if (i < len && pattern[i] == 0x3F) {
                i = i + 1;
            }
            can_quantify = false;
            last_was_quant = true;
            continue;
        }

        if (b == 0x7B) { // '{m,n}'
            if (last_was_quant) {
                return rsxMakeError(RSX_ERR_DOUBLE_QUANTIFIER, i, capture_count, has_anchors, has_backref);
            }
            if (!can_quantify) {
                return rsxMakeError(RSX_ERR_DANGLING_QUANTIFIER, i, capture_count, has_anchors, has_backref);
            }
            let bq = rsxParseBraceQuant(pattern, i);
            if (!bq.ok) {
                return rsxMakeError(bq.err_code, bq.err_pos, capture_count, has_anchors, has_backref);
            }
            if (cur_branch_min[depth] >= last_atom_min) {
                cur_branch_min[depth] = cur_branch_min[depth] - last_atom_min;
            }
            cur_branch_min[depth] = cur_branch_min[depth] + (bq.val_a * last_atom_min);
            last_atom_min = 0;
            can_quantify = false;
            last_was_quant = true;
            i = bq.next_pos;
            continue;
        }

        if (b == 0x5E || b == 0x24) { // '^' or '$' anchors
            has_anchors = true;
            can_quantify = false;
            last_was_quant = false;
            last_atom_min = 0;
            i = i + 1;
            continue;
        }

        // Literal character or '.' wildcard
        cur_branch_min[depth] = cur_branch_min[depth] + 1;
        can_quantify = true;
        last_was_quant = false;
        last_atom_min = 1;
        i = i + 1;
    }

    if (depth > 0) {
        return rsxMakeError(RSX_ERR_UNCLOSED_GROUP, group_start_pos[depth], capture_count, has_anchors, has_backref);
    }

    let total_min = cur_branch_min[0];
    if (group_has_alt[0] && group_alt_min[0] < total_min) {
        total_min = group_alt_min[0];
    }

    let dummy: [1]u8 = [0];
    let prefix_len = rsxExtractPrefixCore(pattern, dummy[0..1], false);

    return RegexSyntaxInfo {
        valid: true,
        err_code: RSX_OK,
        err_pos: 0,
        capture_count: capture_count,
        has_anchors: has_anchors,
        has_backref: has_backref,
        min_len: total_min,
        literal_prefix_len: prefix_len
    };
}

export {
    RSX_OK,
    RSX_ERR_UNCLOSED_GROUP,
    RSX_ERR_UNOPENED_GROUP,
    RSX_ERR_UNCLOSED_CLASS,
    RSX_ERR_INVALID_CLASS_RANGE,
    RSX_ERR_EMPTY_CLASS,
    RSX_ERR_DANGLING_QUANTIFIER,
    RSX_ERR_DOUBLE_QUANTIFIER,
    RSX_ERR_INVALID_QUANTIFIER,
    RSX_ERR_TRAILING_BACKSLASH,
    RSX_ERR_INVALID_ESCAPE,
    RSX_ERR_INVALID_GROUP_HEADER,
    RSX_ERR_NESTING_LIMIT,
    RegexSyntaxInfo,
    isMetaCharacter,
    escapeRegex,
    isWordByte,
    extractLiteralPrefix,
    validateRegexSyntax
};
