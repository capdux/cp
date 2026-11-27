// is-extglob — Zero-allocation Bash/Micromatch extglob pattern detector
// Clean-room MIT pure CP implementation of npm `is-extglob`.

/// Kind of Bash extglob operator prefix.
pub enum ExtglobKind {
    Optional,    // ?(...)
    ZeroOrMore,  // *(...)
    OneOrMore,   // +(...)
    ExactlyOne,  // @(...)
    Negated,     // !(...)
}

/// Byte span and operator kind of a matched extglob expression `str[start..end]`.
pub struct ExtglobSpan {
    pub kind: ExtglobKind,
    pub start: usize,
    pub end: usize,
}

/// Returns true if `ch` is one of the 5 Bash extglob prefix bytes: `@`, `!`, `*`, `+`, `?`.
pub fn isExtglobPrefix(ch: u8): bool {
    // '@' (64), '!' (33), '*' (42), '+' (43), '?' (63)
    return ch == 64 || ch == 33 || ch == 42 || ch == 43 || ch == 63;
}

/// Maps an extglob prefix byte (`?`, `*`, `+`, `@`, `!`) to `ExtglobKind`, or `null`.
pub fn extglobKindFromByte(ch: u8): ExtglobKind? {
    if (ch == 63) { return .Optional; }
    if (ch == 42) { return .ZeroOrMore; }
    if (ch == 43) { return .OneOrMore; }
    if (ch == 64) { return .ExactlyOne; }
    if (ch == 33) { return .Negated; }
    return null;
}

/// Returns the ASCII prefix byte for an `ExtglobKind`.
pub fn extglobPrefixByte(kind: ExtglobKind): u8 {
    return switch (kind) {
        .Optional => 63,    // '?'
        .ZeroOrMore => 42,  // '*'
        .OneOrMore => 43,   // '+'
        .ExactlyOne => 64,  // '@'
        .Negated => 33,     // '!'
    };
}

/// Given `str[start]` is an extglob prefix and `str[start + 1] == '('`,
/// returns the byte index right after the matching unescaped `')'`, or `0` if unclosed.
pub fn matchExtglobEnd(str: string, start: usize): usize {
    if (start + 2 >= str.len || !isExtglobPrefix(str[start]) || str[start + 1] != 40) {
        return 0;
    }
    let depth: usize = 1;
    let j: usize = start + 2;
    while (j < str.len) {
        let ch: u8 = str[j];
        if (ch == 92) {
            // Backslash escapes the next byte
            if (j + 1 < str.len) {
                j = j + 2;
            } else {
                j = j + 1;
            }
        } else if (ch == 40) {
            depth = depth + 1;
            j = j + 1;
        } else if (ch == 41) {
            depth = depth - 1;
            j = j + 1;
            if (depth == 0) {
                return j;
            }
        } else {
            j = j + 1;
        }
    }
    return 0;
}

/// Finds the first complete unescaped extglob expression starting at or after `start_pos`.
/// Returns `ExtglobSpan` with `start`, `end` (exclusive), and `kind`, or `null` if none found.
pub fn findExtglobFrom(str: string, start_pos: usize): ExtglobSpan? {
    if (str.len < 3 || start_pos >= str.len) {
        return null;
    }
    let i: usize = start_pos;
    while (i < str.len) {
        let ch: u8 = str[i];
        if (ch == 92) {
            if (i + 1 < str.len) {
                i = i + 2;
            } else {
                i = i + 1;
            }
        } else if (isExtglobPrefix(ch) && i + 1 < str.len && str[i + 1] == 40) {
            let end_pos: usize = matchExtglobEnd(str, i);
            if (end_pos > 0) {
                const kind = extglobKindFromByte(ch) ?? .Optional;
                return ExtglobSpan {
                    kind: kind,
                    start: i,
                    end: end_pos,
                };
            }
            i = i + 1;
        } else {
            i = i + 1;
        }
    }
    return null;
}

/// Finds the first complete unescaped extglob expression in `str`, or `null` if none found.
pub fn findExtglob(str: string): ExtglobSpan? {
    return findExtglobFrom(str, 0);
}

pub fn findExtglobStart(str: string): i64 {
    if (span := findExtglob(str)) {
        return span.start as i64;
    }
    return -1;
}

pub fn isExtglob(str: string): bool {
    return findExtglob(str) != null;
}

/// Returns true if `str` contains a nested extglob inside another extglob (e.g. `+(a|!(b|c))`).
pub fn hasNestedExtglob(str: string): bool {
    let pos: usize = 0;
    while (pos < str.len) {
        if (span := findExtglobFrom(str, pos)) {
            // Inner body is str[span.start + 2 .. span.end - 1]
            if (span.end > span.start + 3) {
                const inner = str[(span.start + 2)..(span.end - 1)];
                if (isExtglob(inner)) {
                    return true;
                }
            }
            pos = span.end;
        } else {
            break;
        }
    }
    return false;
}

/// Returns true if `str` contains at least one negated extglob `!(...)` (including nested).
pub fn isNegatedExtglob(str: string): bool {
    let pos: usize = 0;
    while (pos < str.len) {
        if (span := findExtglobFrom(str, pos)) {
            if (span.kind == .Negated) {
                return true;
            }
            if (span.end > span.start + 3) {
                const inner = str[(span.start + 2)..(span.end - 1)];
                if (isNegatedExtglob(inner)) {
                    return true;
                }
            }
            pos = span.end;
        } else {
            break;
        }
    }
    return false;
}

pub fn countExtglobs(str: string): usize {
    if (str.len < 3) {
        return 0;
    }
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < str.len) {
        if (span := findExtglobFrom(str, pos)) {
            count = count + 1;
            pos = span.end;
        } else {
            break;
        }
    }
    return count;
}
