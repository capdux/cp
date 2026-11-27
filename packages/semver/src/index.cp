// @cap/semver — Unified Zero-Heap SemVer 2.0.0 Engine (npm `semver` + crates.io `semver`)
// Clean-room MIT superset sharing a single zero-copy parser & evaluator for both
// npm node-semver ranges (`||`, `-`, `^`, `~`, `*`/`x`, `valid`/`gt`/`lt`/`compare`/`inc`/`maxSatisfying`)
// and Rust Cargo/crates.io `semver` (`Version`, `VersionReq`, `parseVersion`, `matchesVersionReq`, `bump*`).

import { isSpace, isDigit, isAlnum, trim, strEq } from "cap:ascii";
import { writeU64, writeStr } from "cap:fmt";

fn svrIsIdentChar(c: u8): bool {
    return isAlnum(c) || c == 45;
}

fn svrIsWildcardStr(s: string): bool {
    return s.len == 1 && (s[0] == 42 || s[0] == 120 || s[0] == 88); // '*' | 'x' | 'X'
}

fn svrParseU32Strict(s: string): u32? {
    if (s.len == 0 || s.len > 9) {
        return null;
    }
    if (s.len > 1 && s[0] == 48) {
        return null; // Disallow leading zeros (SemVer 2.0.0)
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!isDigit(s[i])) {
            return null;
        }
        val = val * 10 + ((s[i] - 48) as u32);
    }
    return val;
}

fn svrIsAllDigits(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!isDigit(s[i])) {
            return false;
        }
    }
    return true;
}

fn svrValidatePre(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i <= s.len) {
        const start = i;
        while (i < s.len && s[i] != 46) {
            if (!svrIsIdentChar(s[i])) {
                return false;
            }
            i += 1;
        }
        const seg = s[start..i];
        if (seg.len == 0) {
            return false;
        }
        if (svrIsAllDigits(seg) && seg.len > 1 && seg[0] == 48) {
            return false; // Numeric pre-release identifier cannot have leading zero
        }
        if (i >= s.len) {
            break;
        }
        i += 1;
    }
    return true;
}

fn svrValidateBuild(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i <= s.len) {
        const start = i;
        while (i < s.len && s[i] != 46) {
            if (!svrIsIdentChar(s[i])) {
                return false;
            }
            i += 1;
        }
        if (i == start) {
            return false;
        }
        if (i >= s.len) {
            break;
        }
        i += 1;
    }
    return true;
}

fn svrWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

/// Core SemVer 2.0.0 value (shared by both npm `SemVer` and Rust `Version`).
pub struct SemVer {
    major: u32,
    minor: u32,
    patch: u32,
    pre: [32]u8,
    pre_len: usize,
    build: [32]u8,
    build_len: usize,
    valid: bool,

    static fn invalid(): SemVer {
        return SemVer {
            major: 0,
            minor: 0,
            patch: 0,
            pre: undefined,
            pre_len: 0,
            build: undefined,
            build_len: 0,
            valid: false,
        };
    }

    static fn make(maj: u32, min: u32, pat: u32): SemVer {
        return SemVer {
            major: maj,
            minor: min,
            patch: pat,
            pre: undefined,
            pre_len: 0,
            build: undefined,
            build_len: 0,
            valid: true,
        };
    }

    static fn parse(s: string): SemVer {
        return parseSemver(s);
    }

    fn preStr(): []const u8 {
        return _.pre[0.._.pre_len];
    }

    fn buildStr(): []const u8 {
        return _.build[0.._.build_len];
    }

    fn isPrerelease(): bool {
        return _.valid && _.pre_len > 0;
    }

    fn writeTo(out: []u8): usize {
        return formatSemver(_, out);
    }
}

/// Rust `semver::Version` type alias (zero-cost alias of `SemVer`).
pub type Version = SemVer;

/// Parses a SemVer 2.0.0 string (optional leading `v` or `=`, optional `-pre`, optional `+build`).
pub fn parseSemver(s: string): SemVer {
    const trimmed = trim(s);
    if (trimmed.len == 0) {
        return SemVer.invalid();
    }

    let i: usize = 0;
    if (trimmed[i] == 61) { // '='
        i += 1;
    }
    if (i < trimmed.len && (trimmed[i] == 118 || trimmed[i] == 86)) { // 'v' | 'V'
        i += 1;
    }
    if (i >= trimmed.len) {
        return SemVer.invalid();
    }

    const b = trimmed.len;
    // Split off +build first, then -pre
    let core_end = b;
    let build_start = b;
    let has_build = false;
    let k = i;
    while (k < b) : (k += 1) {
        if (trimmed[k] == 43) { // '+'
            core_end = k;
            build_start = k + 1;
            has_build = true;
            break;
        }
    }

    let num_end = core_end;
    let pre_start = core_end;
    let has_pre = false;
    k = i;
    while (k < core_end) : (k += 1) {
        if (trimmed[k] == 45) { // '-'
            num_end = k;
            pre_start = k + 1;
            has_pre = true;
            break;
        }
    }

    // Parse major.minor.patch
    let dot1 = num_end;
    k = i;
    while (k < num_end) : (k += 1) {
        if (trimmed[k] == 46) {
            dot1 = k;
            break;
        }
    }
    if (dot1 >= num_end) {
        return SemVer.invalid();
    }

    let dot2 = num_end;
    k = dot1 + 1;
    while (k < num_end) : (k += 1) {
        if (trimmed[k] == 46) {
            dot2 = k;
            break;
        }
    }
    if (dot2 >= num_end) {
        return SemVer.invalid();
    }

    const maj_opt = svrParseU32Strict(trimmed[i..dot1]);
    const min_opt = svrParseU32Strict(trimmed[dot1 + 1..dot2]);
    const pat_opt = svrParseU32Strict(trimmed[dot2 + 1..num_end]);
    if (maj_opt == null || min_opt == null || pat_opt == null) {
        return SemVer.invalid();
    }

    let out = SemVer.make(maj_opt ?? 0, min_opt ?? 0, pat_opt ?? 0);
    if (has_pre) {
        const pre_s = trimmed[pre_start..core_end];
        if (!svrValidatePre(pre_s)) {
            return SemVer.invalid();
        }
        out.pre_len = writeStr(pre_s, out.pre[0..32], 0);
    }
    if (has_build) {
        const bld_s = trimmed[build_start..b];
        if (!svrValidateBuild(bld_s)) {
            return SemVer.invalid();
        }
        out.build_len = writeStr(bld_s, out.build[0..32], 0);
    }
    return out;
}

fn svrComparePreIdent(a: []const u8, b: []const u8): i32 {
    const a_num = svrIsAllDigits(a);
    const b_num = svrIsAllDigits(b);
    if (a_num && b_num) {
        if (a.len < b.len) { return -1; }
        if (a.len > b.len) { return 1; }
        let i: usize = 0;
        while (i < a.len) : (i += 1) {
            if (a[i] < b[i]) { return -1; }
            if (a[i] > b[i]) { return 1; }
        }
        return 0;
    }
    if (a_num && !b_num) { return -1; } // SemVer §11.4.3
    if (!a_num && b_num) { return 1; }

    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        if (a[i] < b[i]) { return -1; }
        if (a[i] > b[i]) { return 1; }
    }
    if (a.len < b.len) { return -1; }
    if (a.len > b.len) { return 1; }
    return 0;
}

/// Compares two SemVer values strictly per SemVer 2.0.0 §11 (returns -1, 0, or 1).
pub fn compareSemver(a: SemVer, b: SemVer): i32 {
    if (a.major < b.major) { return -1; }
    if (a.major > b.major) { return 1; }
    if (a.minor < b.minor) { return -1; }
    if (a.minor > b.minor) { return 1; }
    if (a.patch < b.patch) { return -1; }
    if (a.patch > b.patch) { return 1; }

    if (a.pre_len == 0 && b.pre_len == 0) { return 0; }
    if (a.pre_len == 0 && b.pre_len > 0) { return 1; }
    if (a.pre_len > 0 && b.pre_len == 0) { return -1; }

    let ia: usize = 0;
    let ib: usize = 0;
    while (ia < a.pre_len && ib < b.pre_len) {
        const sa = ia;
        while (ia < a.pre_len && a.pre[ia] != 46) { ia += 1; }
        const sb = ib;
        while (ib < b.pre_len && b.pre[ib] != 46) { ib += 1; }

        const cmp = svrComparePreIdent(a.pre[sa..ia], b.pre[sb..ib]);
        if (cmp != 0) {
            return cmp;
        }
        if (ia < a.pre_len && a.pre[ia] == 46) { ia += 1; }
        if (ib < b.pre_len && b.pre[ib] == 46) { ib += 1; }
    }

    if (ia < a.pre_len) { return 1; }
    if (ib < b.pre_len) { return -1; }
    return 0;
}

/// Partial version representation for range bounds (`1`, `1.2`, `1.x`, `1.2.3-rc.1`).
struct SvrPartial {
    major: u32,
    minor: u32,
    patch: u32,
    has_major: bool,
    has_minor: bool,
    has_patch: bool,
    pre: [32]u8,
    pre_len: usize,
    valid: bool,
}

fn svrParsePartial(raw: string): SvrPartial {
    let p = SvrPartial {
        major: 0,
        minor: 0,
        patch: 0,
        has_major: false,
        has_minor: false,
        has_patch: false,
        pre: undefined,
        pre_len: 0,
        valid: false,
    };

    const s = trim(raw);
    if (s.len == 0) { return p; }

    let i: usize = 0;
    if (s[i] == 118 || s[i] == 86) { i += 1; }
    if (i >= s.len) { return p; }

    const b = s.len;
    if (svrIsWildcardStr(s[i..b])) {
        p.valid = true;
        return p;
    }

    // Strip +build
    let core_end = b;
    let k = i;
    while (k < b) : (k += 1) {
        if (s[k] == 43) {
            core_end = k;
            break;
        }
    }

    // Split -pre
    let num_end = core_end;
    let has_pre = false;
    let pre_start = core_end;
    k = i;
    while (k < core_end) : (k += 1) {
        if (s[k] == 45) {
            num_end = k;
            pre_start = k + 1;
            has_pre = true;
            break;
        }
    }

    // Split by '.' into up to 3 parts
    let seg_idx: usize = 0;
    let cur = i;
    while (cur <= num_end) {
        const s0 = cur;
        while (cur < num_end && s[cur] != 46) {
            cur += 1;
        }
        const seg = s[s0..cur];
        if (seg.len == 0) { return p; }

        if (seg_idx == 0) {
            if (svrIsWildcardStr(seg)) {
                p.has_major = false;
            } else {
                if (v := svrParseU32Strict(seg)) {
                    p.major = v;
                    p.has_major = true;
                } else {
                    return p;
                }
            }
        } else if (seg_idx == 1) {
            if (svrIsWildcardStr(seg)) {
                p.has_minor = false;
            } else {
                if (!p.has_major) { return p; }
                if (v := svrParseU32Strict(seg)) {
                    p.minor = v;
                    p.has_minor = true;
                } else {
                    return p;
                }
            }
        } else if (seg_idx == 2) {
            if (svrIsWildcardStr(seg)) {
                p.has_patch = false;
            } else {
                if (!p.has_minor) { return p; }
                if (v := svrParseU32Strict(seg)) {
                    p.patch = v;
                    p.has_patch = true;
                } else {
                    return p;
                }
            }
        } else {
            return p;
        }

        seg_idx += 1;
        if (cur >= num_end) { break; }
        cur += 1;
    }

    if (has_pre) {
        if (!p.has_patch) { return p; }
        const pre_s = s[pre_start..core_end];
        if (!svrValidatePre(pre_s)) { return p; }
        p.pre_len = writeStr(pre_s, p.pre[0..32], 0);
    }

    p.valid = true;
    return p;
}

fn svrPartialToSemver(p: SvrPartial): SemVer {
    let v = SemVer.make(
        if (p.has_major) p.major else 0,
        if (p.has_minor) p.minor else 0,
        if (p.has_patch) p.patch else 0,
    );
    let i: usize = 0;
    while (i < p.pre_len) : (i += 1) {
        v.pre[i] = p.pre[i];
    }
    v.pre_len = p.pre_len;
    return v;
}

fn svrSameTuplePre(v: SemVer, p: SvrPartial): bool {
    return p.pre_len > 0
        && p.has_major && p.has_minor && p.has_patch
        && v.major == p.major && v.minor == p.minor && v.patch == p.patch;
}

struct SvrCompResult {
    matched: bool,
    allow_pre: bool,
}

fn svrMatchSingleComparator(v: SemVer, tok: string): SvrCompResult {
    const s = trim(tok);
    if (s.len == 0) {
        return SvrCompResult { matched: false, allow_pre: false };
    }

    // Identify operator prefix: >=, <=, >, <, =, ^, ~, or none
    let op: u8 = 0; // 0:none/eq, 1:>, 2:>=, 3:<, 4:<=, 5:^, 6:~
    let body_start: usize = 0;
    if (s.len >= 2 && s[0] == 62 && s[1] == 61) { op = 2; body_start = 2; }
    else if (s.len >= 2 && s[0] == 60 && s[1] == 61) { op = 4; body_start = 2; }
    else if (s[0] == 62) { op = 1; body_start = 1; }
    else if (s[0] == 60) { op = 3; body_start = 1; }
    else if (s[0] == 61) { op = 0; body_start = 1; }
    else if (s[0] == 94) { op = 5; body_start = 1; }
    else if (s[0] == 126) { op = 6; body_start = 1; }

    const p = svrParsePartial(s[body_start..s.len]);
    if (!p.valid) {
        return SvrCompResult { matched: false, allow_pre: false };
    }
    const allow = svrSameTuplePre(v, p);
    const low = svrPartialToSemver(p);

    // Caret ^
    if (op == 5) {
        if (!p.has_major) {
            return SvrCompResult { matched: true, allow_pre: allow };
        }
        let hi = SemVer.make(0, 0, 0);
        if (!p.has_minor) {
            hi = SemVer.make(p.major + 1, 0, 0);
        } else if (!p.has_patch) {
            if (p.major == 0) {
                hi = SemVer.make(0, p.minor + 1, 0);
            } else {
                hi = SemVer.make(p.major + 1, 0, 0);
            }
        } else {
            if (p.major > 0) {
                hi = SemVer.make(p.major + 1, 0, 0);
            } else if (p.minor > 0) {
                hi = SemVer.make(0, p.minor + 1, 0);
            } else {
                hi = SemVer.make(0, 0, p.patch + 1);
            }
        }
        return SvrCompResult {
            matched: compareSemver(v, low) >= 0 && compareSemver(v, hi) < 0,
            allow_pre: allow,
        };
    }

    // Tilde ~
    if (op == 6) {
        if (!p.has_major) {
            return SvrCompResult { matched: true, allow_pre: allow };
        }
        let hi = SemVer.make(0, 0, 0);
        if (!p.has_minor) {
            hi = SemVer.make(p.major + 1, 0, 0);
        } else {
            hi = SemVer.make(p.major, p.minor + 1, 0);
        }
        return SvrCompResult {
            matched: compareSemver(v, low) >= 0 && compareSemver(v, hi) < 0,
            allow_pre: allow,
        };
    }

    // Exact or X-range (no operator or '=')
    if (op == 0) {
        if (!p.has_major) {
            return SvrCompResult { matched: true, allow_pre: allow };
        }
        if (!p.has_minor) {
            const hi = SemVer.make(p.major + 1, 0, 0);
            return SvrCompResult {
                matched: compareSemver(v, low) >= 0 && compareSemver(v, hi) < 0,
                allow_pre: allow,
            };
        }
        if (!p.has_patch) {
            const hi = SemVer.make(p.major, p.minor + 1, 0);
            return SvrCompResult {
                matched: compareSemver(v, low) >= 0 && compareSemver(v, hi) < 0,
                allow_pre: allow,
            };
        }
        return SvrCompResult { matched: compareSemver(v, low) == 0, allow_pre: allow };
    }

    // Comparison operators >, >=, <, <=
    if (op == 1) { // '>'
        if (!p.has_major) { return SvrCompResult { matched: false, allow_pre: allow }; }
        if (!p.has_minor) {
            return SvrCompResult { matched: compareSemver(v, SemVer.make(p.major + 1, 0, 0)) >= 0, allow_pre: allow };
        }
        if (!p.has_patch) {
            return SvrCompResult { matched: compareSemver(v, SemVer.make(p.major, p.minor + 1, 0)) >= 0, allow_pre: allow };
        }
        return SvrCompResult { matched: compareSemver(v, low) > 0, allow_pre: allow };
    }
    if (op == 2) { // '>='
        if (!p.has_major) { return SvrCompResult { matched: true, allow_pre: allow }; }
        return SvrCompResult { matched: compareSemver(v, low) >= 0, allow_pre: allow };
    }
    if (op == 3) { // '<'
        if (!p.has_major) { return SvrCompResult { matched: false, allow_pre: allow }; }
        return SvrCompResult { matched: compareSemver(v, low) < 0, allow_pre: allow };
    }
    if (op == 4) { // '<='
        if (!p.has_major) { return SvrCompResult { matched: true, allow_pre: allow }; }
        if (!p.has_minor) {
            return SvrCompResult { matched: compareSemver(v, SemVer.make(p.major + 1, 0, 0)) < 0, allow_pre: allow };
        }
        if (!p.has_patch) {
            return SvrCompResult { matched: compareSemver(v, SemVer.make(p.major, p.minor + 1, 0)) < 0, allow_pre: allow };
        }
        return SvrCompResult { matched: compareSemver(v, low) <= 0, allow_pre: allow };
    }
    return SvrCompResult { matched: false, allow_pre: false };
}

fn svrIsBareOperator(tok: string): bool {
    if (tok.len == 1) {
        const c = tok[0];
        return c == 62 || c == 60 || c == 61 || c == 94 || c == 126;
    }
    if (tok.len == 2) {
        return (tok[0] == 62 && tok[1] == 61) || (tok[0] == 60 && tok[1] == 61);
    }
    return false;
}

/// Evaluates a single AND-clause (no `||` inside).
fn svrMatchClause(v: SemVer, clause: string): bool {
    const s = trim(clause);
    if (s.len == 0) {
        return v.pre_len == 0; // empty clause == '*'
    }

    // Check hyphen range: "<left> - <right>"
    let hyphen_pos: usize = s.len;
    let k: usize = 1;
    while (k + 1 < s.len) : (k += 1) {
        if (s[k] == 45 && isSpace(s[k - 1]) && isSpace(s[k + 1])) {
            hyphen_pos = k;
            break;
        }
    }

    let allow_pre = false;
    if (hyphen_pos < s.len) {
        const left_p = svrParsePartial(s[0..hyphen_pos]);
        const right_p = svrParsePartial(s[hyphen_pos + 1..s.len]);
        if (!left_p.valid || !right_p.valid) {
            return false;
        }
        if (svrSameTuplePre(v, left_p) || svrSameTuplePre(v, right_p)) {
            allow_pre = true;
        }
        const low = svrPartialToSemver(left_p);
        if (compareSemver(v, low) < 0) {
            return false;
        }
        let hi_ok = true;
        if (right_p.has_major) {
            if (!right_p.has_minor) {
                hi_ok = compareSemver(v, SemVer.make(right_p.major + 1, 0, 0)) < 0;
            } else if (!right_p.has_patch) {
                hi_ok = compareSemver(v, SemVer.make(right_p.major, right_p.minor + 1, 0)) < 0;
            } else {
                hi_ok = compareSemver(v, svrPartialToSemver(right_p)) <= 0;
            }
        }
        if (!hi_ok) {
            return false;
        }
        if (v.pre_len > 0 && !allow_pre) {
            return false;
        }
        return true;
    }

    // Space-separated comparator intersection
    let i: usize = 0;
    while (i < s.len) {
        while (i < s.len && isSpace(s[i])) { i += 1; }
        if (i >= s.len) { break; }
        const t0 = i;
        while (i < s.len && !isSpace(s[i])) { i += 1; }
        const tok = s[t0..i];

        if (svrIsBareOperator(tok)) {
            while (i < s.len && isSpace(s[i])) { i += 1; }
            if (i >= s.len) { return false; }
            const v0 = i;
            while (i < s.len && !isSpace(s[i])) { i += 1; }
            let merged: [64]u8 = undefined;
            let mpos: usize = 0;
            mpos = writeStr(tok, merged[0..64], mpos);
            mpos = writeStr(s[v0..i], merged[0..64], mpos);
            const r = svrMatchSingleComparator(v, merged[0..mpos]);
            if (r.allow_pre) { allow_pre = true; }
            if (!r.matched) { return false; }
        } else {
            const r = svrMatchSingleComparator(v, tok);
            if (r.allow_pre) { allow_pre = true; }
            if (!r.matched) { return false; }
        }
    }

    if (v.pre_len > 0 && !allow_pre) {
        return false;
    }
    return true;
}

fn svrSatisfiesVer(v: SemVer, range: string): bool {
    if (!v.valid) {
        return false;
    }
    let i: usize = 0;
    let start: usize = 0;
    while (i <= range.len) {
        const is_or = (i + 1 < range.len) && range[i] == 124 && range[i + 1] == 124;
        if (i == range.len || is_or) {
            if (svrMatchClause(v, range[start..i])) {
                return true;
            }
            if (i == range.len) {
                break;
            }
            i += 2;
            start = i;
            continue;
        }
        i += 1;
    }
    return false;
}

fn svrValidateReqPart(part: string): bool {
    if (part.len == 0) { return false; }
    let body_start: usize = 0;
    if (part.len >= 2 && ((part[0] == 62 && part[1] == 61) || (part[0] == 60 && part[1] == 61))) {
        body_start = 2;
    } else if (part[0] == 62 || part[0] == 60 || part[0] == 61 || part[0] == 94 || part[0] == 126) {
        body_start = 1;
    }
    const p = svrParsePartial(part[body_start..part.len]);
    return p.valid;
}

fn svrMatchesVersionReq(v: SemVer, req: string): bool {
    if (!v.valid) { return false; }
    const trimmed = trim(req);
    if (trimmed.len == 0) { return false; }
    if (trimmed.len == 1 && trimmed[0] == 42) { return v.pre_len == 0; } // "*"

    let pos: usize = 0;
    let clause_count: usize = 0;
    while (pos <= trimmed.len) {
        let sep = pos;
        while (sep < trimmed.len && trimmed[sep] != 44) { sep += 1; }
        const part = trim(trimmed[pos..sep]);
        if (part.len > 0) {
            clause_count += 1;
            // Cargo default: bare version starting with digit implies caret '^' unless wildcard '*'
            let has_star = false;
            let k: usize = 0;
            while (k < part.len) : (k += 1) {
                if (part[k] == 42 || part[k] == 120 || part[k] == 88) {
                    has_star = true;
                    break;
                }
            }
            if (!has_star && isDigit(part[0])) {
                let buf: [64]u8 = undefined;
                if (part.len + 1 <= 64) {
                    buf[0] = 94; // '^'
                    let i: usize = 0;
                    while (i < part.len) : (i += 1) {
                        buf[i + 1] = part[i];
                    }
                    if (!svrSatisfiesVer(v, buf[0..part.len + 1])) {
                        return false;
                    }
                } else {
                    return false;
                }
            } else {
                if (!svrSatisfiesVer(v, part)) {
                    return false;
                }
            }
        }
        if (sep >= trimmed.len) { break; }
        pos = sep + 1;
    }
    return clause_count > 0;
}

// ============================================================================
// Rust Cargo / crates.io `semver` API (`Version`, `VersionReq`, `parseVersion`, ...)
// ============================================================================

/// Zero-heap Rust Cargo `semver::VersionReq` matcher (comma-separated clauses).
pub struct VersionReq {
    raw: string,
    valid: bool,

    static fn parse(req: string): VersionReq {
        const trimmed = trim(req);
        if (trimmed.len == 0) {
            return VersionReq { raw: "", valid: false };
        }
        if (trimmed.len == 1 && trimmed[0] == 42) {
            return VersionReq { raw: trimmed, valid: true };
        }
        let pos: usize = 0;
        let clause_count: usize = 0;
        while (pos <= trimmed.len) {
            let sep = pos;
            while (sep < trimmed.len && trimmed[sep] != 44) { sep += 1; }
            const part = trim(trimmed[pos..sep]);
            if (part.len > 0) {
                if (!svrValidateReqPart(part)) {
                    return VersionReq { raw: "", valid: false };
                }
                clause_count += 1;
            }
            if (sep >= trimmed.len) { break; }
            pos = sep + 1;
        }
        if (clause_count == 0) {
            return VersionReq { raw: "", valid: false };
        }
        return VersionReq { raw: trimmed, valid: true };
    }

    fn matches(v: Version): bool {
        if (!_.valid) { return false; }
        return svrMatchesVersionReq(v, _.raw);
    }

    fn matchesStr(version_str: string): bool {
        if (!_.valid) { return false; }
        return svrMatchesVersionReq(parseSemver(version_str), _.raw);
    }
}

/// Parses a strict SemVer 2.0 version string (`semver::Version::parse`).
pub fn parseVersion(s: string): Version {
    return parseSemver(s);
}

/// Compares two `Version` values (-1, 0, 1) per SemVer 2.0 precedence rules.
pub fn compareVersions(a: Version, b: Version): i32 {
    return compareSemver(a, b);
}

/// Returns true if the version has a non-empty pre-release tag (e.g. `1.0.0-alpha.1`).
pub fn isPrerelease(v: Version): bool {
    return v.valid && v.pre_len > 0;
}

/// Evaluates a Cargo/Rust comma-separated `VersionReq` (e.g. `">=1.2.3, <1.8.0"`, `"^1.2"`, `"~1.2.3"`, `"1.2.*"`, `"*"`).
/// In Rust's `semver` crate, bare versions like `"1.2.3"` default to caret (`"^1.2.3"`).
pub fn matchesVersionReq(version_str: string, req: string): bool {
    return svrMatchesVersionReq(parseSemver(version_str), req);
}

/// Checks if `ver` is strictly compatible with `target` under Cargo caret semantics (`^target`).
pub fn isCaretCompatible(ver: string, target: string): bool {
    return matchesVersionReq(ver, target);
}

/// Formats `Version` into `out`, returning bytes written.
pub fn formatVersion(v: Version, out: []u8): usize {
    return formatSemver(v, out);
}

/// Increments major, minor, or patch version cleanly (Rust naming).
pub fn bumpMajor(v: Version): Version { return incMajor(v); }
pub fn bumpMinor(v: Version): Version { return incMinor(v); }
pub fn bumpPatch(v: Version): Version { return incPatch(v); }

// ============================================================================
// npm `semver` API (`satisfies`, `valid`, `gt`, `lt`, `compare`, `inc`, `maxSatisfying`, ...)
// ============================================================================

/// Evaluates whether `version` satisfies npm semver `range` (supports `||`, space AND, `^`, `~`, `*`/`x`, `-`, comparators).
pub fn satisfies(version: string, range: string): bool {
    return svrSatisfiesVer(parseSemver(version), range);
}

/// Returns true if `s` is a valid SemVer 2.0.0 string (npm `semver.valid`).
pub fn valid(s: string): bool {
    return parseSemver(s).valid;
}

/// Compares two SemVer strings (-1, 0, or 1; npm `semver.compare`).
pub fn compare(a: string, b: string): i32 {
    const va = parseSemver(a);
    const vb = parseSemver(b);
    if (!va.valid && !vb.valid) { return 0; }
    if (!va.valid) { return -1; }
    if (!vb.valid) { return 1; }
    return compareSemver(va, vb);
}

/// Returns true if `a > b` (npm `semver.gt`).
pub fn gt(a: string, b: string): bool {
    const va = parseSemver(a);
    const vb = parseSemver(b);
    return va.valid && vb.valid && compareSemver(va, vb) > 0;
}

/// Returns true if `a >= b` (npm `semver.gte`).
pub fn gte(a: string, b: string): bool {
    const va = parseSemver(a);
    const vb = parseSemver(b);
    return va.valid && vb.valid && compareSemver(va, vb) >= 0;
}

/// Returns true if `a < b` (npm `semver.lt`).
pub fn lt(a: string, b: string): bool {
    const va = parseSemver(a);
    const vb = parseSemver(b);
    return va.valid && vb.valid && compareSemver(va, vb) < 0;
}

/// Returns true if `a <= b` (npm `semver.lte`).
pub fn lte(a: string, b: string): bool {
    const va = parseSemver(a);
    const vb = parseSemver(b);
    return va.valid && vb.valid && compareSemver(va, vb) <= 0;
}

/// Returns true if `a == b` (npm `semver.eq`).
pub fn eq(a: string, b: string): bool {
    const va = parseSemver(a);
    const vb = parseSemver(b);
    return va.valid && vb.valid && compareSemver(va, vb) == 0;
}

/// Returns true if `a != b` (npm `semver.neq`).
pub fn neq(a: string, b: string): bool {
    const va = parseSemver(a);
    const vb = parseSemver(b);
    return va.valid && vb.valid && compareSemver(va, vb) != 0;
}

/// Returns the major version number, or `null` if invalid (npm `semver.major`).
pub fn major(s: string): u32? {
    const v = parseSemver(s);
    if (!v.valid) { return null; }
    return v.major;
}

/// Returns the minor version number, or `null` if invalid (npm `semver.minor`).
pub fn minor(s: string): u32? {
    const v = parseSemver(s);
    if (!v.valid) { return null; }
    return v.minor;
}

/// Returns the patch version number, or `null` if invalid (npm `semver.patch`).
pub fn patch(s: string): u32? {
    const v = parseSemver(s);
    if (!v.valid) { return null; }
    return v.patch;
}

/// Normalizes a version string (e.g. `" =v1.2.3 "` -> `"1.2.3"`) into `out`, returning bytes written.
pub fn clean(version: string, out: []u8): usize {
    const v = parseSemver(version);
    if (!v.valid) { return 0; }
    return formatSemver(v, out);
}

/// Increments a version string by `"major"`, `"minor"`, or `"patch"` into `out` (npm `semver.inc`), returning bytes written (0 on error).
pub fn inc(version: string, release: string, out: []u8): usize {
    const v = parseSemver(version);
    if (!v.valid) { return 0; }
    if (strEq(release, "major")) {
        return formatSemver(incMajor(v), out);
    }
    if (strEq(release, "minor")) {
        return formatSemver(incMinor(v), out);
    }
    if (strEq(release, "patch")) {
        return formatSemver(incPatch(v), out);
    }
    return 0;
}

/// Returns the index of the highest version in `versions` satisfying `range`, or -1 if none.
pub fn maxSatisfying(versions: []const string, range: string): i32 {
    let best_idx: i32 = -1;
    let best_ver = SemVer.invalid();
    let i: usize = 0;
    while (i < versions.len) : (i += 1) {
        const cur = parseSemver(versions[i]);
        if (svrSatisfiesVer(cur, range)) {
            if (best_idx < 0 || compareSemver(cur, best_ver) > 0) {
                best_idx = i as i32;
                best_ver = cur;
            }
        }
    }
    return best_idx;
}

/// Returns the index of the lowest version in `versions` satisfying `range`, or -1 if none.
pub fn minSatisfying(versions: []const string, range: string): i32 {
    let best_idx: i32 = -1;
    let best_ver = SemVer.invalid();
    let i: usize = 0;
    while (i < versions.len) : (i += 1) {
        const cur = parseSemver(versions[i]);
        if (svrSatisfiesVer(cur, range)) {
            if (best_idx < 0 || compareSemver(cur, best_ver) < 0) {
                best_idx = i as i32;
                best_ver = cur;
            }
        }
    }
    return best_idx;
}

pub fn incMajor(v: SemVer): SemVer {
    if (!v.valid) { return SemVer.invalid(); }
    if (v.pre_len > 0 && v.minor == 0 && v.patch == 0) {
        return SemVer.make(v.major, 0, 0);
    }
    return SemVer.make(v.major + 1, 0, 0);
}

pub fn incMinor(v: SemVer): SemVer {
    if (!v.valid) { return SemVer.invalid(); }
    if (v.pre_len > 0 && v.patch == 0) {
        return SemVer.make(v.major, v.minor, 0);
    }
    return SemVer.make(v.major, v.minor + 1, 0);
}

pub fn incPatch(v: SemVer): SemVer {
    if (!v.valid) { return SemVer.invalid(); }
    if (v.pre_len > 0) {
        return SemVer.make(v.major, v.minor, v.patch);
    }
    return SemVer.make(v.major, v.minor, v.patch + 1);
}

/// Formats `v` as `major.minor.patch[-pre][+build]` into `out`, returning bytes written.
pub fn formatSemver(v: SemVer, out: []u8): usize {
    if (!v.valid) {
        return 0;
    }
    let pos: usize = 0;
    pos = writeU64(v.major as u64, out, pos);
    pos = svrWriteByte(out, pos, 46);
    pos = writeU64(v.minor as u64, out, pos);
    pos = svrWriteByte(out, pos, 46);
    pos = writeU64(v.patch as u64, out, pos);
    if (v.pre_len > 0) {
        pos = svrWriteByte(out, pos, 45);
        pos = writeStr(v.pre[0..v.pre_len], out, pos);
    }
    if (v.build_len > 0) {
        pos = svrWriteByte(out, pos, 43);
        pos = writeStr(v.build[0..v.build_len], out, pos);
    }
    return pos;
}

export {
    SemVer,
    Version,
    VersionReq,
    parseSemver,
    parseVersion,
    compareSemver,
    compareVersions,
    isPrerelease,
    matchesVersionReq,
    isCaretCompatible,
    formatSemver,
    formatVersion,
    bumpMajor,
    bumpMinor,
    bumpPatch,
    satisfies,
    valid,
    compare,
    gt,
    gte,
    lt,
    lte,
    eq,
    neq,
    major,
    minor,
    patch,
    clean,
    inc,
    maxSatisfying,
    minSatisfying,
    incMajor,
    incMinor,
    incPatch,
};
