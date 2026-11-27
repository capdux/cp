// Source: Absorbed from /data/dev/bun/src/install/semver/ | Synced: 2026-11-21T16:39:55Z (NPM/Bun Semver Range Matcher & Comparator)
// Pure-CP zero-allocation SemVer 2.0.0 parser, comparator, and single-pass NPM/Bun range evaluator.

/// Parsed SemVer 2.0.0 version (zero-copy `pre` slice referencing the input).
pub struct SemverVersion {
    major: u64;
    minor: u64;
    patch: u64;
    pre: []const u8;

    /// Parse a SemVer 2.0.0 version string with optional leading `v`/`V` and ignored `+build`.
    /// Rejects leading zeros in numeric components, malformed prerelease/build identifiers, or overflow.
    static fn parse(input: []const u8): SemverVersion? {
        if (input.len == 0) return null;
        let pos: usize = 0;
        if (input[0] == 'v' || input[0] == 'V') {
            pos = 1;
            if (pos >= input.len) return null;
        }

        // Split off optional +build
        let core_end = input.len;
        let build_start: usize = input.len;
        let has_build = false;
        let scan = pos;
        while (scan < input.len) {
            if (input[scan] == '+') {
                core_end = scan;
                build_start = scan + 1;
                has_build = true;
                break;
            }
            scan += 1;
        }
        if (has_build) {
            if (!isValidBuildMetadata(input[build_start..input.len])) return null;
        }

        // Split off optional -pre
        let ver_end = core_end;
        let pre: []const u8 = "";
        scan = pos;
        while (scan < core_end) {
            if (input[scan] == '-') {
                ver_end = scan;
                pre = input[(scan + 1)..core_end];
                if (!isValidPrerelease(pre)) return null;
                break;
            }
            scan += 1;
        }

        // Parse major.minor.patch
        const nums = input[pos..ver_end];
        let dot1: usize = nums.len;
        let dot2: usize = nums.len;
        let dot_count: usize = 0;
        let k: usize = 0;
        while (k < nums.len) {
            if (nums[k] == '.') {
                if (dot_count == 0) {
                    dot1 = k;
                } else if (dot_count == 1) {
                    dot2 = k;
                } else {
                    return null;
                }
                dot_count += 1;
            }
            k += 1;
        }
        if (dot_count != 2) return null;

        const major = parseStrictUint(nums[0..dot1]) orelse return null;
        const minor = parseStrictUint(nums[(dot1 + 1)..dot2]) orelse return null;
        const patch = parseStrictUint(nums[(dot2 + 1)..nums.len]) orelse return null;

        return SemverVersion {
            major: major,
            minor: minor,
            patch: patch,
            pre: pre,
        };
    }

    /// Compare this version against `other` per SemVer 2.0.0 precedence rules.
    /// Returns -1 if less, 0 if equal, 1 if greater.
    fn compare(other: SemverVersion): i32 {
        if (_.major < other.major) return -1;
        if (_.major > other.major) return 1;
        if (_.minor < other.minor) return -1;
        if (_.minor > other.minor) return 1;
        if (_.patch < other.patch) return -1;
        if (_.patch > other.patch) return 1;

        if (_.pre.len == 0 && other.pre.len == 0) return 0;
        if (_.pre.len == 0) return 1;
        if (other.pre.len == 0) return -1;
        return comparePrerelease(_.pre, other.pre);
    }
}

/// Internal representation of a partial or wildcard version inside a range comparator.
struct PartialVersion {
    major: u64;
    minor: u64;
    patch: u64;
    has_major: bool;
    has_minor: bool;
    has_patch: bool;
    pre: []const u8;
}

/// Comparator operator kind.
enum OpKind {
    Exact,
    Gt,
    Gte,
    Lt,
    Lte,
    Caret,
    Tilde,
}

/// Result of evaluating a single comparator set (AND group).
struct SetEvalResult {
    valid: bool;
    matched: bool;
}

type SemverVec16 = @Vector(16, u8);

fn makeSemverCharLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let f: u8 = 0;
        if (i == ' ' || i == '\t' || i == '\r' || i == '\n') f |= 1;
        if (i >= '0' && i <= '9') f |= 2 | 4;
        if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z') || i == '-') f |= 4;
        lut[i] = f;
    }
    return lut;
}

const SEMVER_CHAR_LUT: [u8; 256] = makeSemverCharLut();

fn isSpace(b: u8): bool {
    return (SEMVER_CHAR_LUT[usize(b)] & 1) != 0;
}

fn isDigit(b: u8): bool {
    return (SEMVER_CHAR_LUT[usize(b)] & 2) != 0;
}

fn isIdentChar(b: u8): bool {
    return (SEMVER_CHAR_LUT[usize(b)] & 4) != 0;
}

fn isWildcardSegment(seg: []const u8): bool {
    return seg.len == 1 && (seg[0] == '*' || seg[0] == 'x' || seg[0] == 'X');
}

fn isAllDigits(seg: []const u8): bool {
    if (seg.len == 0) return false;
    let i: usize = 0;
    const v0: SemverVec16 = @splat('0' as u8);
    const v9: SemverVec16 = @splat('9' as u8);
    while (i + 16 <= seg.len) : (i += 16) {
        const v: SemverVec16 = seg[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v < v0) | (v > v9))) return false;
    }
    while (i < seg.len) {
        if (!isDigit(seg[i])) return false;
        i += 1;
    }
    return true;
}

/// Parse a non-negative u64 with strict SemVer no-leading-zero validation.
fn parseStrictUint(seg: []const u8): u64? {
    if (seg.len == 0) return null;
    if (seg.len > 1 && seg[0] == '0') return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < seg.len) {
        const b = seg[i];
        if (!isDigit(b)) return null;
        const digit: u64 = (b - '0') as u64;
        if (val > 1844674407370955161) return null;
        if (val == 1844674407370955161 && digit > 5) return null;
        val = val * 10 + digit;
        i += 1;
    }
    return val;
}

/// Validate a SemVer 2.0.0 prerelease string (dot-separated non-empty identifiers).
fn isValidPrerelease(pre: []const u8): bool {
    if (pre.len == 0) return false;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= pre.len) {
        if (i == pre.len || pre[i] == '.') {
            const seg = pre[start..i];
            if (seg.len == 0) return false;
            let all_num = true;
            let j: usize = 0;
            while (j < seg.len) {
                if (!isIdentChar(seg[j])) return false;
                if (!isDigit(seg[j])) all_num = false;
                j += 1;
            }
            if (all_num && seg.len > 1 && seg[0] == '0') return false;
            start = i + 1;
        }
        i += 1;
    }
    return true;
}

/// Validate a SemVer 2.0.0 build metadata string (dot-separated non-empty identifiers).
fn isValidBuildMetadata(build: []const u8): bool {
    if (build.len == 0) return false;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= build.len) {
        if (i == build.len || build[i] == '.') {
            const seg = build[start..i];
            if (seg.len == 0) return false;
            let j: usize = 0;
            while (j < seg.len) {
                if (!isIdentChar(seg[j])) return false;
                j += 1;
            }
            start = i + 1;
        }
        i += 1;
    }
    return true;
}

/// Compare two dot-separated prerelease strings per SemVer 2.0.0 Section 11.
fn comparePrerelease(a: []const u8, b: []const u8): i32 {
    let pa: usize = 0;
    let pb: usize = 0;
    const vdot: SemverVec16 = @splat('.' as u8);
    while (pa < a.len && pb < b.len) {
        let ea = pa;
        while (ea + 16 <= a.len) : (ea += 16) {
            const v: SemverVec16 = a[ea .. ea + 16][0..16].*;
            if (@reduce(.Or, v == vdot)) break;
        }
        while (ea < a.len && a[ea] != '.') ea += 1;
        let eb = pb;
        while (eb + 16 <= b.len) : (eb += 16) {
            const v: SemverVec16 = b[eb .. eb + 16][0..16].*;
            if (@reduce(.Or, v == vdot)) break;
        }
        while (eb < b.len && b[eb] != '.') eb += 1;

        const sa = a[pa..ea];
        const sb = b[pb..eb];
        const na = isAllDigits(sa);
        const nb = isAllDigits(sb);

        if (na && nb) {
            // Strip any leading zeros in case SemverVersion was manually constructed
            let la: usize = 0;
            while (la + 1 < sa.len && sa[la] == '0') la += 1;
            let lb: usize = 0;
            while (lb + 1 < sb.len && sb[lb] == '0') lb += 1;
            const ta = sa[la..sa.len];
            const tb = sb[lb..sb.len];
            if (ta.len < tb.len) return -1;
            if (ta.len > tb.len) return 1;
            let k: usize = 0;
            while (k + 16 <= ta.len) : (k += 16) {
                const va: SemverVec16 = ta[k .. k + 16][0..16].*;
                const vb: SemverVec16 = tb[k .. k + 16][0..16].*;
                if (@reduce(.Or, va ^ vb) != 0) break;
            }
            while (k < ta.len) {
                if (ta[k] < tb[k]) return -1;
                if (ta[k] > tb[k]) return 1;
                k += 1;
            }
        } else if (na && !nb) {
            return -1;
        } else if (!na && nb) {
            return 1;
        } else {
            const min_len = if (sa.len < sb.len) sa.len else sb.len;
            let k: usize = 0;
            while (k + 16 <= min_len) : (k += 16) {
                const va: SemverVec16 = sa[k .. k + 16][0..16].*;
                const vb: SemverVec16 = sb[k .. k + 16][0..16].*;
                if (@reduce(.Or, va ^ vb) != 0) break;
            }
            while (k < min_len) {
                if (sa[k] < sb[k]) return -1;
                if (sa[k] > sb[k]) return 1;
                k += 1;
            }
            if (sa.len < sb.len) return -1;
            if (sa.len > sb.len) return 1;
        }

        pa = if (ea < a.len) ea + 1 else a.len;
        pb = if (eb < b.len) eb + 1 else b.len;
    }

    if (pa >= a.len && pb >= b.len) return 0;
    if (pa >= a.len) return -1;
    return 1;
}

/// Strict upper bound `< maj.min.pat-0` (excludes all prereleases of `maj.min.pat`).
fn isLessThanReleaseTuple(ver: SemverVersion, maj: u64, min: u64, pat: u64): bool {
    if (ver.major != maj) return ver.major < maj;
    if (ver.minor != min) return ver.minor < min;
    return ver.patch < pat;
}

/// Lower bound `>= maj.min.pat` (exact release with empty prerelease).
fn isGteReleaseTuple(ver: SemverVersion, maj: u64, min: u64, pat: u64): bool {
    const bound = SemverVersion { major: maj, minor: min, patch: pat, pre: "" };
    return ver.compare(bound) >= 0;
}

/// Parse a partial or wildcard version token (e.g., `*`, `1.x`, `1.2`, `1.2.3-alpha.1`).
fn parsePartial(raw: []const u8): PartialVersion? {
    if (raw.len == 0) return null;
    let pos: usize = 0;
    if (raw[0] == 'v' || raw[0] == 'V') {
        pos = 1;
        if (pos >= raw.len) return null;
    }

    // Split optional +build
    let core_end = raw.len;
    let scan = pos;
    while (scan < raw.len) {
        if (raw[scan] == '+') {
            core_end = scan;
            if (!isValidBuildMetadata(raw[(scan + 1)..raw.len])) return null;
            break;
        }
        scan += 1;
    }

    // Split optional -pre
    let ver_end = core_end;
    let pre: []const u8 = "";
    scan = pos;
    while (scan < core_end) {
        if (raw[scan] == '-') {
            ver_end = scan;
            pre = raw[(scan + 1)..core_end];
            if (!isValidPrerelease(pre)) return null;
            break;
        }
        scan += 1;
    }

    const nums = raw[pos..ver_end];
    if (nums.len == 0) return null;

    // Split up to 3 dot-separated segments
    let seg_starts: [3]usize = [0; 3];
    let seg_ends: [3]usize = [0; 3];
    let seg_count: usize = 0;
    let s: usize = 0;
    let i: usize = 0;
    while (i <= nums.len) {
        if (i == nums.len || nums[i] == '.') {
            if (seg_count >= 3) return null;
            if (i == s) return null;
            seg_starts[seg_count] = s;
            seg_ends[seg_count] = i;
            seg_count += 1;
            s = i + 1;
        }
        i += 1;
    }

    let res = PartialVersion {
        major: 0,
        minor: 0,
        patch: 0,
        has_major: false,
        has_minor: false,
        has_patch: false,
        pre: pre,
    };

    const seg0 = nums[seg_starts[0]..seg_ends[0]];
    if (!isWildcardSegment(seg0)) {
        res.major = parseStrictUint(seg0) orelse return null;
        res.has_major = true;
    }

    if (seg_count >= 2) {
        const seg1 = nums[seg_starts[1]..seg_ends[1]];
        if (!isWildcardSegment(seg1)) {
            if (!res.has_major) return null;
            res.minor = parseStrictUint(seg1) orelse return null;
            res.has_minor = true;
        }
    }

    if (seg_count == 3) {
        const seg2 = nums[seg_starts[2]..seg_ends[2]];
        if (!isWildcardSegment(seg2)) {
            if (!res.has_minor) return null;
            res.patch = parseStrictUint(seg2) orelse return null;
            res.has_patch = true;
        }
    }

    // Prerelease tag is only valid on a complete major.minor.patch tuple
    if (pre.len > 0 && !res.has_patch) return null;
    return res;
}

/// Evaluate a single operator + partial version comparator against `ver`.
fn matchComparator(ver: SemverVersion, op: OpKind, p: PartialVersion): bool {
    if (op == OpKind.Exact) {
        if (!p.has_major) return true;
        if (!p.has_minor) {
            return isGteReleaseTuple(ver, p.major, 0, 0) &&
                isLessThanReleaseTuple(ver, p.major + 1, 0, 0);
        }
        if (!p.has_patch) {
            return isGteReleaseTuple(ver, p.major, p.minor, 0) &&
                isLessThanReleaseTuple(ver, p.major, p.minor + 1, 0);
        }
        const target = SemverVersion { major: p.major, minor: p.minor, patch: p.patch, pre: p.pre };
        return ver.compare(target) == 0;
    }

    if (op == OpKind.Gt) {
        if (!p.has_major) return false;
        if (!p.has_minor) return isGteReleaseTuple(ver, p.major + 1, 0, 0);
        if (!p.has_patch) return isGteReleaseTuple(ver, p.major, p.minor + 1, 0);
        const target = SemverVersion { major: p.major, minor: p.minor, patch: p.patch, pre: p.pre };
        return ver.compare(target) > 0;
    }

    if (op == OpKind.Gte) {
        if (!p.has_major) return true;
        if (!p.has_minor) return isGteReleaseTuple(ver, p.major, 0, 0);
        if (!p.has_patch) return isGteReleaseTuple(ver, p.major, p.minor, 0);
        const target = SemverVersion { major: p.major, minor: p.minor, patch: p.patch, pre: p.pre };
        return ver.compare(target) >= 0;
    }

    if (op == OpKind.Lt) {
        if (!p.has_major) return false;
        if (!p.has_minor) return isLessThanReleaseTuple(ver, p.major, 0, 0);
        if (!p.has_patch) return isLessThanReleaseTuple(ver, p.major, p.minor, 0);
        const target = SemverVersion { major: p.major, minor: p.minor, patch: p.patch, pre: p.pre };
        return ver.compare(target) < 0;
    }

    if (op == OpKind.Lte) {
        if (!p.has_major) return true;
        if (!p.has_minor) return isLessThanReleaseTuple(ver, p.major + 1, 0, 0);
        if (!p.has_patch) return isLessThanReleaseTuple(ver, p.major, p.minor + 1, 0);
        const target = SemverVersion { major: p.major, minor: p.minor, patch: p.patch, pre: p.pre };
        return ver.compare(target) <= 0;
    }

    if (op == OpKind.Caret) {
        if (!p.has_major) return true;
        if (!p.has_minor) {
            return isGteReleaseTuple(ver, p.major, 0, 0) &&
                isLessThanReleaseTuple(ver, p.major + 1, 0, 0);
        }
        if (!p.has_patch) {
            if (p.major > 0) {
                return isGteReleaseTuple(ver, p.major, p.minor, 0) &&
                    isLessThanReleaseTuple(ver, p.major + 1, 0, 0);
            }
            return isGteReleaseTuple(ver, 0, p.minor, 0) &&
                isLessThanReleaseTuple(ver, 0, p.minor + 1, 0);
        }
        const lower = SemverVersion { major: p.major, minor: p.minor, patch: p.patch, pre: p.pre };
        if (ver.compare(lower) < 0) return false;
        if (p.major > 0) return isLessThanReleaseTuple(ver, p.major + 1, 0, 0);
        if (p.minor > 0) return isLessThanReleaseTuple(ver, 0, p.minor + 1, 0);
        return isLessThanReleaseTuple(ver, 0, 0, p.patch + 1);
    }

    // OpKind.Tilde
    if (!p.has_major) return true;
    if (!p.has_minor) {
        return isGteReleaseTuple(ver, p.major, 0, 0) &&
            isLessThanReleaseTuple(ver, p.major + 1, 0, 0);
    }
    if (!p.has_patch) {
        return isGteReleaseTuple(ver, p.major, p.minor, 0) &&
            isLessThanReleaseTuple(ver, p.major, p.minor + 1, 0);
    }
    const lower = SemverVersion { major: p.major, minor: p.minor, patch: p.patch, pre: p.pre };
    return ver.compare(lower) >= 0 && isLessThanReleaseTuple(ver, p.major, p.minor + 1, 0);
}

/// Evaluate a hyphen range `left - right` (inclusive of both bounds per NPM semver).
fn matchHyphenRange(ver: SemverVersion, left: PartialVersion, right: PartialVersion): bool {
    if (!matchComparator(ver, OpKind.Gte, left)) return false;
    return matchComparator(ver, OpKind.Lte, right);
}

/// Check if a partial version comparator authorizes `ver`'s prerelease tuple.
fn allowsPrereleaseTuple(ver: SemverVersion, p: PartialVersion): bool {
    return p.pre.len > 0 &&
        p.has_major &&
        p.has_minor &&
        p.has_patch &&
        p.major == ver.major &&
        p.minor == ver.minor &&
        p.patch == ver.patch;
}

/// Trim leading and trailing ASCII whitespace.
fn trimSpaces(src: []const u8): []const u8 {
    let start: usize = 0;
    while (start < src.len && isSpace(src[start])) start += 1;
    let end = src.len;
    while (end > start && isSpace(src[end - 1])) end -= 1;
    return src[start..end];
}

/// Check if a token is a standalone operator (`>=`, `<=`, `>`, `<`, `=`, `^`, `~`).
fn parseStandaloneOp(tok: []const u8): OpKind? {
    if (tok.len == 2) {
        if (tok[0] == '>' && tok[1] == '=') return OpKind.Gte;
        if (tok[0] == '<' && tok[1] == '=') return OpKind.Lte;
        return null;
    }
    if (tok.len == 1) {
        if (tok[0] == '>') return OpKind.Gt;
        if (tok[0] == '<') return OpKind.Lt;
        if (tok[0] == '=') return OpKind.Exact;
        if (tok[0] == '^') return OpKind.Caret;
        if (tok[0] == '~') return OpKind.Tilde;
    }
    return null;
}

/// Extract leading operator prefix from a token (returning OpKind and byte length of prefix).
fn splitOpPrefix(tok: []const u8): usize {
    if (tok.len >= 2 && ((tok[0] == '>' && tok[1] == '=') || (tok[0] == '<' && tok[1] == '='))) {
        return 2;
    }
    if (tok.len >= 1) {
        const c = tok[0];
        if (c == '>' || c == '<' || c == '=' || c == '^' || c == '~') {
            return 1;
        }
    }
    return 0;
}

/// Token byte bounds inside a comparator set.
struct TokenSpan {
    start: usize;
    end: usize;
}

/// Read the next whitespace-delimited token from `src` starting at `pos`.
/// Returns a `TokenSpan` with `end == start` if no more tokens remain.
fn nextTokenSpan(src: []const u8, start_pos: usize): TokenSpan {
    let i = start_pos;
    while (i < src.len && isSpace(src[i])) i += 1;
    const tok_start = i;
    while (i < src.len && !isSpace(src[i])) i += 1;
    return TokenSpan { start: tok_start, end: i };
}

/// Evaluate a single AND comparator set against `ver` (when `eval_ver` is true)
/// or purely validate syntax (when `eval_ver` is false).
fn evalComparatorSet(ver: SemverVersion, set_raw: []const u8, eval_ver: bool): SetEvalResult {
    const set = trimSpaces(set_raw);
    if (set.len == 0) {
        // Empty set acts as `*` (matches any non-prerelease version)
        const ok = if (eval_ver) ver.pre.len == 0 else true;
        return SetEvalResult { valid: true, matched: ok };
    }

    let all_ok = true;
    let pre_allowed = false;
    let pos: usize = 0;

    while (pos < set.len) {
        const sp1 = nextTokenSpan(set, pos);
        if (sp1.start == sp1.end) break;
        const tok1 = set[sp1.start..sp1.end];
        pos = sp1.end;

        // Standalone "-" without a left operand is invalid
        if (tok1.len == 1 && tok1[0] == '-') {
            return SetEvalResult { valid: false, matched: false };
        }

        // Check if tok1 is a standalone operator followed by a version token (e.g. ">= 1.2.3")
        if (standalone_op := parseStandaloneOp(tok1)) {
            const sp2 = nextTokenSpan(set, pos);
            if (sp2.start == sp2.end) {
                return SetEvalResult { valid: false, matched: false };
            }
            const tok2 = set[sp2.start..sp2.end];
            pos = sp2.end;
            const partial = parsePartial(tok2) orelse {
                return SetEvalResult { valid: false, matched: false };
            };
            if (eval_ver) {
                if (allowsPrereleaseTuple(ver, partial)) pre_allowed = true;
                if (all_ok && !matchComparator(ver, standalone_op, partial)) all_ok = false;
            }
            continue;
        }

        // Check for hyphen range: `left - right` (only when tok1 has no operator prefix)
        const prefix_len = splitOpPrefix(tok1);
        if (prefix_len == 0) {
            const peek = nextTokenSpan(set, pos);
            if (peek.end - peek.start == 1 && set[peek.start] == '-') {
                // Consume "-" and parse right operand
                const right_sp = nextTokenSpan(set, peek.end);
                if (right_sp.start == right_sp.end) {
                    return SetEvalResult { valid: false, matched: false };
                }
                const right_tok = set[right_sp.start..right_sp.end];
                pos = right_sp.end;

                const left_p = parsePartial(tok1) orelse {
                    return SetEvalResult { valid: false, matched: false };
                };
                const right_p = parsePartial(right_tok) orelse {
                    return SetEvalResult { valid: false, matched: false };
                };
                if (eval_ver) {
                    if (allowsPrereleaseTuple(ver, left_p) || allowsPrereleaseTuple(ver, right_p)) {
                        pre_allowed = true;
                    }
                    if (all_ok && !matchHyphenRange(ver, left_p, right_p)) all_ok = false;
                }
                continue;
            }
        }

        // Standard prefixed or bare comparator token
        const op = if (prefix_len == 0)
            OpKind.Exact
        else
            (parseStandaloneOp(tok1[0..prefix_len]) orelse {
                return SetEvalResult { valid: false, matched: false };
            });

        const ver_tok = tok1[prefix_len..tok1.len];
        const partial = parsePartial(ver_tok) orelse {
            return SetEvalResult { valid: false, matched: false };
        };

        if (eval_ver) {
            if (allowsPrereleaseTuple(ver, partial)) pre_allowed = true;
            if (all_ok && !matchComparator(ver, op, partial)) all_ok = false;
        }
    }

    if (eval_ver && ver.pre.len > 0 && !pre_allowed) {
        all_ok = false;
    }
    return SetEvalResult { valid: true, matched: all_ok };
}

/// Evaluate whether a parsed `SemverVersion` satisfies an NPM/Bun range expression.
/// Returns `false` if the range expression is syntactically invalid or does not match.
pub fn satisfiesVersion(ver: SemverVersion, range_str: []const u8): bool {
    let any_matched = false;
    let start: usize = 0;
    let i: usize = 0;
    const vpipe: SemverVec16 = @splat('|' as u8);
    while (i <= range_str.len) {
        if (i + 16 <= range_str.len) {
            const v: SemverVec16 = range_str[i .. i + 16][0..16].*;
            if (!@reduce(.Or, v == vpipe)) {
                i += 16;
                continue;
            }
        }
        const is_or = (i + 1 < range_str.len && range_str[i] == '|' && range_str[i + 1] == '|');
        if (i == range_str.len || is_or) {
            const segment = range_str[start..i];
            const res = evalComparatorSet(ver, segment, true);
            if (!res.valid) return false;
            if (res.matched) any_matched = true;
            if (is_or) {
                i += 2;
                start = i;
                continue;
            }
            break;
        }
        i += 1;
    }
    return any_matched;
}

/// Parse `version_str` as SemVer 2.0.0 and evaluate whether it satisfies `range_str`.
/// Returns `false` if either the version or the range expression is invalid.
pub fn satisfiesRange(version_str: []const u8, range_str: []const u8): bool {
    const ver = SemverVersion.parse(version_str) orelse return false;
    return satisfiesVersion(ver, range_str);
}

/// Validate whether `range_str` is a syntactically valid NPM/Bun Semver range expression.
pub fn isValidSemverRange(range_str: []const u8): bool {
    const dummy = SemverVersion { major: 0, minor: 0, patch: 0, pre: "" };
    let start: usize = 0;
    let i: usize = 0;
    const vpipe: SemverVec16 = @splat('|' as u8);
    while (i <= range_str.len) {
        if (i + 16 <= range_str.len) {
            const v: SemverVec16 = range_str[i .. i + 16][0..16].*;
            if (!@reduce(.Or, v == vpipe)) {
                i += 16;
                continue;
            }
        }
        const is_or = (i + 1 < range_str.len && range_str[i] == '|' && range_str[i + 1] == '|');
        if (i == range_str.len || is_or) {
            const segment = range_str[start..i];
            const res = evalComparatorSet(dummy, segment, false);
            if (!res.valid) return false;
            if (is_or) {
                i += 2;
                start = i;
                continue;
            }
            break;
        }
        i += 1;
    }
    return true;
}
