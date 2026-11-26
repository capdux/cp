// packages/@cap/semver-range/src/index.cp
// Zero-allocation npm / Cargo SemVer version range parser and matcher.
// MIT clean-room pure CP implementation.

/// Parsed `MAJOR.MINOR.PATCH` semantic version.
pub struct SemVer {
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
    pub valid: bool,
}

const OP_IMPLICIT_EQ: u8 = 0;
const OP_EQ: u8 = 1;
const OP_GT: u8 = 2;
const OP_GE: u8 = 3;
const OP_LT: u8 = 4;
const OP_LE: u8 = 5;
const OP_CARET: u8 = 6;
const OP_TILDE: u8 = 7;

fn srIsSpace(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8) || c == ('\r' as u8) || c == ('\n' as u8);
}

fn srTrim(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && srIsSpace(s[start])) {
        start += 1;
    }
    while (end > start && srIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn srIsWildcardToken(s: []const u8): bool {
    if (s.len != 1) {
        return false;
    }
    const c = s[0];
    return c == ('*' as u8) || c == ('x' as u8) || c == ('X' as u8);
}

fn srParseU32(s: []const u8, out: *u32): bool {
    if (s.len == 0 || s.len > 10) {
        return false;
    }
    let acc: u64 = 0 as u64;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < ('0' as u8) || c > ('9' as u8)) {
            return false;
        }
        acc = acc * (10 as u64) + ((c - ('0' as u8)) as u64);
        if (acc > (4294967295 as u64)) {
            return false;
        }
        i += 1;
    }
    out.* = acc as u32;
    return true;
}

/// Parse a `MAJOR.MINOR.PATCH` version string (optional leading `v`/`V`, ignores `+build` metadata).
pub fn parseVersion(s: string): SemVer {
    let invalid = SemVer{
        major: 0 as u32,
        minor: 0 as u32,
        patch: 0 as u32,
        valid: false,
    };

    let t = srTrim(s);
    if (t.len == 0) {
        return invalid;
    }
    if (t[0] == ('v' as u8) || t[0] == ('V' as u8)) {
        t = t[1..t.len];
    }

    // Strip optional '+build' metadata.
    let plus = t.len;
    let pi: usize = 0;
    while (pi < t.len) {
        if (t[pi] == ('+' as u8)) {
            plus = pi;
            break;
        }
        pi += 1;
    }
    t = t[0..plus];
    if (t.len == 0) {
        return invalid;
    }

    // Find two '.' separators.
    let d1 = t.len;
    let d2 = t.len;
    let i: usize = 0;
    while (i < t.len) {
        if (t[i] == ('.' as u8)) {
            if (d1 == t.len) {
                d1 = i;
            } else if (d2 == t.len) {
                d2 = i;
            } else {
                return invalid;
            }
        }
        i += 1;
    }
    if (d1 == t.len || d2 == t.len) {
        return invalid;
    }

    let maj: u32 = 0 as u32;
    let min: u32 = 0 as u32;
    let pat: u32 = 0 as u32;
    if (!srParseU32(t[0..d1], &maj)) {
        return invalid;
    }
    if (!srParseU32(t[(d1 + 1)..d2], &min)) {
        return invalid;
    }
    if (!srParseU32(t[(d2 + 1)..t.len], &pat)) {
        return invalid;
    }

    return SemVer{
        major: maj,
        minor: min,
        patch: pat,
        valid: true,
    };
}

/// Compare two `SemVer` values: returns `-1` if `a < b`, `0` if `a == b`, `1` if `a > b`.
pub fn compareVersion(a: SemVer, b: SemVer): i32 {
    if (a.major < b.major) { return -1 as i32; }
    if (a.major > b.major) { return 1 as i32; }
    if (a.minor < b.minor) { return -1 as i32; }
    if (a.minor > b.minor) { return 1 as i32; }
    if (a.patch < b.patch) { return -1 as i32; }
    if (a.patch > b.patch) { return 1 as i32; }
    return 0 as i32;
}

/// Evaluate a single comparator (e.g. `^1.2.3`, `~1.2`, `>=1.0.0`, `1.2.x`, `*`) against `ver`.
/// Returns: `1` (matched), `0` (not matched), `-1` (invalid comparator syntax).
fn srEvalComparator(ver: SemVer, rawComp: []const u8, evalVer: bool): i32 {
    let s = srTrim(rawComp);
    if (s.len == 0) {
        return -1 as i32;
    }

    let op: u8 = OP_IMPLICIT_EQ;
    if (s.len >= 2 && s[0] == ('>' as u8) && s[1] == ('=' as u8)) {
        op = OP_GE;
        s = srTrim(s[2..s.len]);
    } else if (s.len >= 2 && s[0] == ('<' as u8) && s[1] == ('=' as u8)) {
        op = OP_LE;
        s = srTrim(s[2..s.len]);
    } else if (s[0] == ('>' as u8)) {
        op = OP_GT;
        s = srTrim(s[1..s.len]);
    } else if (s[0] == ('<' as u8)) {
        op = OP_LT;
        s = srTrim(s[1..s.len]);
    } else if (s[0] == ('=' as u8)) {
        op = OP_EQ;
        s = srTrim(s[1..s.len]);
    } else if (s[0] == ('^' as u8)) {
        op = OP_CARET;
        s = srTrim(s[1..s.len]);
    } else if (s[0] == ('~' as u8)) {
        op = OP_TILDE;
        s = srTrim(s[1..s.len]);
    }

    if (s.len == 0) {
        return -1 as i32;
    }
    if (s[0] == ('v' as u8) || s[0] == ('V' as u8)) {
        s = s[1..s.len];
    }

    // Strip optional '+build' metadata.
    let plus = s.len;
    let pi: usize = 0;
    while (pi < s.len) {
        if (s[pi] == ('+' as u8)) {
            plus = pi;
            break;
        }
        pi += 1;
    }
    s = s[0..plus];
    if (s.len == 0) {
        return -1 as i32;
    }

    // Split up to 3 dot-separated parts.
    let d1 = s.len;
    let d2 = s.len;
    let di: usize = 0;
    while (di < s.len) {
        if (s[di] == ('.' as u8)) {
            if (d1 == s.len) {
                d1 = di;
            } else if (d2 == s.len) {
                d2 = di;
            } else {
                return -1 as i32;
            }
        }
        di += 1;
    }

    let p0 = s[0..d1];
    let hasP1 = false;
    let p1: []const u8 = "";
    let hasP2 = false;
    let p2: []const u8 = "";

    if (d1 < s.len) {
        hasP1 = true;
        if (d2 < s.len) {
            p1 = s[(d1 + 1)..d2];
            hasP2 = true;
            p2 = s[(d2 + 1)..s.len];
        } else {
            p1 = s[(d1 + 1)..s.len];
        }
    }

    let hasMaj = false;
    let maj: u32 = 0 as u32;
    let hasMin = false;
    let min: u32 = 0 as u32;
    let hasPat = false;
    let pat: u32 = 0 as u32;

    if (srIsWildcardToken(p0)) {
        hasMaj = false;
    } else if (srParseU32(p0, &maj)) {
        hasMaj = true;
    } else {
        return -1 as i32;
    }

    if (hasP1) {
        if (srIsWildcardToken(p1)) {
            hasMin = false;
        } else if (hasMaj && srParseU32(p1, &min)) {
            hasMin = true;
        } else {
            return -1 as i32;
        }
    }

    if (hasP2) {
        if (srIsWildcardToken(p2)) {
            hasPat = false;
        } else if (hasMin && srParseU32(p2, &pat)) {
            hasPat = true;
        } else {
            return -1 as i32;
        }
    }

    // Disallow relational/caret/tilde on pure wildcard (e.g. `>*`, `^*`).
    if (!hasMaj && op != OP_IMPLICIT_EQ && op != OP_EQ) {
        return -1 as i32;
    }

    if (!evalVer) {
        return 1 as i32;
    }

    // Wildcard '*' matches all versions.
    if (!hasMaj) {
        return 1 as i32;
    }

    const target = SemVer{
        major: maj,
        minor: min,
        patch: pat,
        valid: true,
    };
    const cmp = compareVersion(ver, target);

    if (op == OP_IMPLICIT_EQ || op == OP_EQ) {
        if (!hasMin) {
            if (ver.major == maj) { return 1 as i32; }
            return 0 as i32;
        }
        if (!hasPat) {
            if (ver.major == maj && ver.minor == min) { return 1 as i32; }
            return 0 as i32;
        }
        if (cmp == (0 as i32)) { return 1 as i32; }
        return 0 as i32;
    }

    if (op == OP_GT) {
        if (cmp > (0 as i32)) { return 1 as i32; }
        return 0 as i32;
    }
    if (op == OP_GE) {
        if (cmp >= (0 as i32)) { return 1 as i32; }
        return 0 as i32;
    }
    if (op == OP_LT) {
        if (cmp < (0 as i32)) { return 1 as i32; }
        return 0 as i32;
    }
    if (op == OP_LE) {
        if (cmp <= (0 as i32)) { return 1 as i32; }
        return 0 as i32;
    }

    if (op == OP_TILDE) {
        if (cmp < (0 as i32)) {
            return 0 as i32;
        }
        if (!hasMin) {
            // ~1 -> <2.0.0
            if (ver.major == maj) { return 1 as i32; }
            return 0 as i32;
        }
        // ~1.2 or ~1.2.3 -> >=1.2.x && <1.3.0
        if (ver.major == maj && ver.minor == min) { return 1 as i32; }
        return 0 as i32;
    }

    if (op == OP_CARET) {
        if (cmp < (0 as i32)) {
            return 0 as i32;
        }
        if (maj > (0 as u32)) {
            // ^1.2.3 -> <2.0.0
            if (ver.major == maj) { return 1 as i32; }
            return 0 as i32;
        }
        // maj == 0
        if (!hasMin) {
            // ^0 -> <1.0.0
            if (ver.major == (0 as u32)) { return 1 as i32; }
            return 0 as i32;
        }
        if (min > (0 as u32)) {
            // ^0.2.3 -> <0.3.0
            if (ver.major == (0 as u32) && ver.minor == min) { return 1 as i32; }
            return 0 as i32;
        }
        // maj == 0 && min == 0
        if (!hasPat) {
            // ^0.0 -> <0.1.0
            if (ver.major == (0 as u32) && ver.minor == (0 as u32)) { return 1 as i32; }
            return 0 as i32;
        }
        // ^0.0.3 -> ==0.0.3
        if (ver.major == (0 as u32) && ver.minor == (0 as u32) && ver.patch == pat) {
            return 1 as i32;
        }
        return 0 as i32;
    }

    return -1 as i32;
}

fn srIsOpOnly(tok: []const u8): bool {
    if (tok.len == 1) {
        const c = tok[0];
        return c == ('>' as u8) || c == ('<' as u8) || c == ('=' as u8) || c == ('^' as u8) || c == ('~' as u8);
    }
    if (tok.len == 2) {
        return (tok[0] == ('>' as u8) || tok[0] == ('<' as u8)) && tok[1] == ('=' as u8);
    }
    return false;
}

/// Evaluate one AND-branch (space/comma-separated comparators).
/// Returns: `1` (matched), `0` (not matched), `-1` (syntax error).
fn srEvalBranch(ver: SemVer, branch: []const u8, evalVer: bool): i32 {
    const t = srTrim(branch);
    if (t.len == 0) {
        return -1 as i32;
    }

    let pos: usize = 0;
    let count: usize = 0;
    let allMatched = true;

    while (pos < t.len) {
        while (pos < t.len && (srIsSpace(t[pos]) || t[pos] == (',' as u8))) {
            pos += 1;
        }
        if (pos >= t.len) {
            break;
        }

        const start = pos;
        while (pos < t.len && !srIsSpace(t[pos]) && t[pos] != (',' as u8)) {
            pos += 1;
        }
        let end = pos;

        // If token is a standalone operator (e.g. `>= 1.0.0`), attach the following operand token.
        if (srIsOpOnly(t[start..end])) {
            while (pos < t.len && srIsSpace(t[pos])) {
                pos += 1;
            }
            if (pos >= t.len || t[pos] == (',' as u8)) {
                return -1 as i32;
            }
            while (pos < t.len && !srIsSpace(t[pos]) && t[pos] != (',' as u8)) {
                pos += 1;
            }
            end = pos;
        }

        const st = srEvalComparator(ver, t[start..end], evalVer);
        if (st < (0 as i32)) {
            return -1 as i32;
        }
        if (st == (0 as i32)) {
            allMatched = false;
        }
        count += 1;
    }

    if (count == 0) {
        return -1 as i32;
    }
    if (allMatched) {
        return 1 as i32;
    }
    return 0 as i32;
}

/// Evaluate an entire range (`branch1 || branch2 || ...`).
/// Returns: `1` (matched), `0` (not matched), `-1` (invalid range syntax).
fn srEvalRange(ver: SemVer, range: string, evalVer: bool): i32 {
    const t = srTrim(range);
    if (t.len == 0) {
        return -1 as i32;
    }

    let pos: usize = 0;
    let anyMatched = false;

    while (pos <= t.len) {
        // Scan until "||" or end of string.
        let end = pos;
        let foundOr = false;
        while (end < t.len) {
            if (end + 1 < t.len && t[end] == ('|' as u8) && t[end + 1] == ('|' as u8)) {
                foundOr = true;
                break;
            }
            // Single '|' is invalid syntax.
            if (t[end] == ('|' as u8)) {
                return -1 as i32;
            }
            end += 1;
        }

        const st = srEvalBranch(ver, t[pos..end], evalVer);
        if (st < (0 as i32)) {
            return -1 as i32;
        }
        if (st == (1 as i32)) {
            anyMatched = true;
        }

        if (!foundOr) {
            break;
        }
        pos = end + 2;
        if (pos >= t.len) {
            // Trailing "||" is invalid.
            return -1 as i32;
        }
    }

    if (anyMatched) {
        return 1 as i32;
    }
    return 0 as i32;
}

/// Validate whether `range` is a syntactically valid SemVer range expression.
pub fn isValidRange(range: string): bool {
    const dummy = SemVer{
        major: 0 as u32,
        minor: 0 as u32,
        patch: 0 as u32,
        valid: true,
    };
    return srEvalRange(dummy, range, false) >= (0 as i32);
}

/// Check whether `version` satisfies `range` (supporting `||`, space/comma AND sets,
/// `=`, `>`, `>=`, `<`, `<=`, `^`, `~`, and `*`/`x`/`X` wildcards).
pub fn satisfies(version: string, range: string): bool {
    const ver = parseVersion(version);
    if (!ver.valid) {
        return false;
    }
    return srEvalRange(ver, range, true) == (1 as i32);
}

/// Return the index of the highest valid version in `versions` satisfying `range`,
/// or `-1` if no version matches or `range` is invalid.
pub fn maxSatisfyingIndex(versions: []const string, range: string): i64 {
    if (!isValidRange(range)) {
        return -1 as i64;
    }
    let bestIdx: i64 = -1 as i64;
    let bestVer = SemVer{
        major: 0 as u32,
        minor: 0 as u32,
        patch: 0 as u32,
        valid: false,
    };

    let i: usize = 0;
    while (i < versions.len) {
        const v = parseVersion(versions[i]);
        if (v.valid && srEvalRange(v, range, true) == (1 as i32)) {
            if (bestIdx < (0 as i64) || compareVersion(v, bestVer) > (0 as i32)) {
                bestIdx = i as i64;
                bestVer = v;
            }
        }
        i += 1;
    }
    return bestIdx;
}

export {
    SemVer,
    parseVersion,
    compareVersion,
    satisfies,
    isValidRange,
    maxSatisfyingIndex,
};
