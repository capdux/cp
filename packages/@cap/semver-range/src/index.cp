// @cap/semver-range — Zero-heap SemVer 2.0.0 & npm range engine (node-semver parity)
// Clean-room MIT CP implementation of npm `semver` (400M+/wk).

fn svrIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn svrIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn svrIsIdentChar(c: u8): bool {
    return svrIsDigit(c) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 45;
}

fn svrIsWildcardStr(s: string): bool {
    return s.len == 1 && (s[0] == 42 || s[0] == 120 || s[0] == 88); // '*' | 'x' | 'X'
}

fn svrTrimBounds(s: string, out_start: *usize, out_end: *usize) {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && svrIsSpace(s[a])) {
        a = a + 1;
    }
    while (b > a && svrIsSpace(s[b - 1])) {
        b = b - 1;
    }
    out_start.* = a;
    out_end.* = b;
}

fn svrParseU32Strict(s: string, out_val: *u32): bool {
    if (s.len == 0 || s.len > 9) {
        return false;
    }
    if (s.len > 1 && s[0] == 48) {
        return false; // Disallow leading zeros (SemVer 2.0.0)
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!svrIsDigit(s[i])) {
            return false;
        }
        val = val * 10 + ((s[i] - 48) as u32);
        i = i + 1;
    }
    out_val.* = val;
    return true;
}

fn svrIsAllDigits(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!svrIsDigit(s[i])) {
            return false;
        }
        i = i + 1;
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
            i = i + 1;
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
        i = i + 1;
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
            i = i + 1;
        }
        if (i == start) {
            return false;
        }
        if (i >= s.len) {
            break;
        }
        i = i + 1;
    }
    return true;
}

fn svrCopyBytes(src: string, dst: []u8): usize {
    const n = if (src.len < dst.len) { src.len } else { dst.len };
    let i: usize = 0;
    while (i < n) {
        dst[i] = src[i];
        i = i + 1;
    }
    return n;
}

fn svrAppendU32(out: []u8, pos: *usize, val: u32) {
    let tmp: [12]u8 = [0; 12];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = ((v % 10) + 48) as u8;
            v = v / 10;
            n = n + 1;
        }
    }
    while (n > 0) {
        n = n - 1;
        if (pos.* < out.len) {
            out[pos.*] = tmp[n];
            pos.* = pos.* + 1;
        }
    }
}

fn svrAppendSlice(out: []u8, pos: *usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len && pos.* < out.len) {
        out[pos.*] = src[i];
        pos.* = pos.* + 1;
        i = i + 1;
    }
}

fn svrAppendByte(out: []u8, pos: *usize, b: u8) {
    if (pos.* < out.len) {
        out[pos.*] = b;
        pos.* = pos.* + 1;
    }
}

pub struct SemVer {
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
    pub pre: [32]u8,
    pub pre_len: usize,
    pub build: [32]u8,
    pub build_len: usize,
    pub valid: bool,

    pub static fn invalid(): SemVer {
        return SemVer {
            major: 0,
            minor: 0,
            patch: 0,
            pre: [0; 32],
            pre_len: 0,
            build: [0; 32],
            build_len: 0,
            valid: false,
        };
    }

    pub static fn make(major: u32, minor: u32, patch: u32): SemVer {
        return SemVer {
            major: major,
            minor: minor,
            patch: patch,
            pre: [0; 32],
            pre_len: 0,
            build: [0; 32],
            build_len: 0,
            valid: true,
        };
    }

    pub fn preStr(): []const u8 {
        return _.pre[0.._.pre_len];
    }

    pub fn buildStr(): []const u8 {
        return _.build[0.._.build_len];
    }
}

/// Parses a SemVer 2.0.0 string (optional leading `v` or `=`, optional `-pre`, optional `+build`).
pub fn parseSemver(s: string): SemVer {
    let a: usize = 0;
    let b: usize = 0;
    svrTrimBounds(s, &a, &b);
    if (a >= b) {
        return SemVer.invalid();
    }

    let i = a;
    if (s[i] == 61) { // '='
        i = i + 1;
    }
    if (i < b && (s[i] == 118 || s[i] == 86)) { // 'v' | 'V'
        i = i + 1;
    }
    if (i >= b) {
        return SemVer.invalid();
    }

    // Split off +build first, then -pre
    let core_end = b;
    let build_start = b;
    let build_end = b;
    let has_build = false;
    let k = i;
    while (k < b) {
        if (s[k] == 43) { // '+'
            core_end = k;
            build_start = k + 1;
            build_end = b;
            has_build = true;
            break;
        }
        k = k + 1;
    }

    let num_end = core_end;
    let pre_start = core_end;
    let pre_end = core_end;
    let has_pre = false;
    k = i;
    while (k < core_end) {
        if (s[k] == 45) { // '-'
            num_end = k;
            pre_start = k + 1;
            pre_end = core_end;
            has_pre = true;
            break;
        }
        k = k + 1;
    }

    // Parse major.minor.patch
    let dot1 = num_end;
    k = i;
    while (k < num_end) {
        if (s[k] == 46) {
            dot1 = k;
            break;
        }
        k = k + 1;
    }
    if (dot1 >= num_end) {
        return SemVer.invalid();
    }

    let dot2 = num_end;
    k = dot1 + 1;
    while (k < num_end) {
        if (s[k] == 46) {
            dot2 = k;
            break;
        }
        k = k + 1;
    }
    if (dot2 >= num_end) {
        return SemVer.invalid();
    }

    let maj: u32 = 0;
    let min: u32 = 0;
    let pat: u32 = 0;
    if (!svrParseU32Strict(s[i..dot1], &maj)) { return SemVer.invalid(); }
    if (!svrParseU32Strict(s[dot1 + 1..dot2], &min)) { return SemVer.invalid(); }
    if (!svrParseU32Strict(s[dot2 + 1..num_end], &pat)) { return SemVer.invalid(); }

    let out = SemVer.make(maj, min, pat);
    if (has_pre) {
        const pre_s = s[pre_start..pre_end];
        if (!svrValidatePre(pre_s)) {
            return SemVer.invalid();
        }
        out.pre_len = svrCopyBytes(pre_s, out.pre[0..32]);
    }
    if (has_build) {
        const bld_s = s[build_start..build_end];
        if (!svrValidateBuild(bld_s)) {
            return SemVer.invalid();
        }
        out.build_len = svrCopyBytes(bld_s, out.build[0..32]);
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
        while (i < a.len) {
            if (a[i] < b[i]) { return -1; }
            if (a[i] > b[i]) { return 1; }
            i = i + 1;
        }
        return 0;
    }
    if (a_num && !b_num) { return -1; } // SemVer §11.4.3
    if (!a_num && b_num) { return 1; }

    const min_len = if (a.len < b.len) { a.len } else { b.len };
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) { return -1; }
        if (a[i] > b[i]) { return 1; }
        i = i + 1;
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
        while (ia < a.pre_len && a.pre[ia] != 46) { ia = ia + 1; }
        const sb = ib;
        while (ib < b.pre_len && b.pre[ib] != 46) { ib = ib + 1; }

        const cmp = svrComparePreIdent(a.pre[sa..ia], b.pre[sb..ib]);
        if (cmp != 0) {
            return cmp;
        }
        if (ia < a.pre_len && a.pre[ia] == 46) { ia = ia + 1; }
        if (ib < b.pre_len && b.pre[ib] == 46) { ib = ib + 1; }
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
        pre: [0; 32],
        pre_len: 0,
        valid: false,
    };

    let a: usize = 0;
    let b: usize = 0;
    svrTrimBounds(raw, &a, &b);
    if (a >= b) { return p; }

    let i = a;
    if (raw[i] == 118 || raw[i] == 86) { i = i + 1; }
    if (i >= b) { return p; }

    if (svrIsWildcardStr(raw[i..b])) {
        p.valid = true;
        return p;
    }

    // Strip +build
    let core_end = b;
    let k = i;
    while (k < b) {
        if (raw[k] == 43) {
            core_end = k;
            break;
        }
        k = k + 1;
    }

    // Split -pre
    let num_end = core_end;
    let has_pre = false;
    let pre_start = core_end;
    k = i;
    while (k < core_end) {
        if (raw[k] == 45) {
            num_end = k;
            pre_start = k + 1;
            has_pre = true;
            break;
        }
        k = k + 1;
    }

    // Split by '.' into up to 3 parts
    let seg_idx: usize = 0;
    let cur = i;
    while (cur <= num_end) {
        const s0 = cur;
        while (cur < num_end && raw[cur] != 46) {
            cur = cur + 1;
        }
        const seg = raw[s0..cur];
        if (seg.len == 0) { return p; }

        if (seg_idx == 0) {
            if (svrIsWildcardStr(seg)) {
                p.has_major = false;
            } else {
                let v: u32 = 0;
                if (!svrParseU32Strict(seg, &v)) { return p; }
                p.major = v;
                p.has_major = true;
            }
        } else if (seg_idx == 1) {
            if (svrIsWildcardStr(seg)) {
                p.has_minor = false;
            } else {
                if (!p.has_major) { return p; }
                let v: u32 = 0;
                if (!svrParseU32Strict(seg, &v)) { return p; }
                p.minor = v;
                p.has_minor = true;
            }
        } else if (seg_idx == 2) {
            if (svrIsWildcardStr(seg)) {
                p.has_patch = false;
            } else {
                if (!p.has_minor) { return p; }
                let v: u32 = 0;
                if (!svrParseU32Strict(seg, &v)) { return p; }
                p.patch = v;
                p.has_patch = true;
            }
        } else {
            return p;
        }

        seg_idx = seg_idx + 1;
        if (cur >= num_end) { break; }
        cur = cur + 1;
    }

    if (has_pre) {
        if (!p.has_patch) { return p; }
        const pre_s = raw[pre_start..core_end];
        if (!svrValidatePre(pre_s)) { return p; }
        p.pre_len = svrCopyBytes(pre_s, p.pre[0..32]);
    }

    p.valid = true;
    return p;
}

fn svrPartialToSemver(p: SvrPartial): SemVer {
    let v = SemVer.make(
        if (p.has_major) { p.major } else { 0 },
        if (p.has_minor) { p.minor } else { 0 },
        if (p.has_patch) { p.patch } else { 0 },
    );
    let i: usize = 0;
    while (i < p.pre_len) {
        v.pre[i] = p.pre[i];
        i = i + 1;
    }
    v.pre_len = p.pre_len;
    return v;
}

fn svrSameTuplePre(v: SemVer, p: SvrPartial): bool {
    return p.pre_len > 0
        && p.has_major && p.has_minor && p.has_patch
        && v.major == p.major && v.minor == p.minor && v.patch == p.patch;
}

fn svrMatchSingleComparator(v: SemVer, tok: string, allow_pre: *bool): bool {
    let a: usize = 0;
    let b: usize = 0;
    svrTrimBounds(tok, &a, &b);
    if (a >= b) { return false; }
    const s = tok[a..b];

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
        return false;
    }
    if (svrSameTuplePre(v, p)) {
        allow_pre.* = true;
    }

    const low = svrPartialToSemver(p);

    // Caret ^
    if (op == 5) {
        if (!p.has_major) {
            return true;
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
        return compareSemver(v, low) >= 0 && compareSemver(v, hi) < 0;
    }

    // Tilde ~
    if (op == 6) {
        if (!p.has_major) {
            return true;
        }
        let hi = SemVer.make(0, 0, 0);
        if (!p.has_minor) {
            hi = SemVer.make(p.major + 1, 0, 0);
        } else {
            hi = SemVer.make(p.major, p.minor + 1, 0);
        }
        return compareSemver(v, low) >= 0 && compareSemver(v, hi) < 0;
    }

    // Exact or X-range (no operator or '=')
    if (op == 0) {
        if (!p.has_major) {
            return true; // '*' / 'x'
        }
        if (!p.has_minor) {
            const hi = SemVer.make(p.major + 1, 0, 0);
            return compareSemver(v, low) >= 0 && compareSemver(v, hi) < 0;
        }
        if (!p.has_patch) {
            const hi = SemVer.make(p.major, p.minor + 1, 0);
            return compareSemver(v, low) >= 0 && compareSemver(v, hi) < 0;
        }
        return compareSemver(v, low) == 0;
    }

    // Comparison operators >, >=, <, <=
    if (op == 1) { // '>'
        if (!p.has_major) { return false; }
        if (!p.has_minor) { return compareSemver(v, SemVer.make(p.major + 1, 0, 0)) >= 0; }
        if (!p.has_patch) { return compareSemver(v, SemVer.make(p.major, p.minor + 1, 0)) >= 0; }
        return compareSemver(v, low) > 0;
    }
    if (op == 2) { // '>='
        if (!p.has_major) { return true; }
        return compareSemver(v, low) >= 0;
    }
    if (op == 3) { // '<'
        if (!p.has_major) { return false; }
        return compareSemver(v, low) < 0;
    }
    if (op == 4) { // '<='
        if (!p.has_major) { return true; }
        if (!p.has_minor) { return compareSemver(v, SemVer.make(p.major + 1, 0, 0)) < 0; }
        if (!p.has_patch) { return compareSemver(v, SemVer.make(p.major, p.minor + 1, 0)) < 0; }
        return compareSemver(v, low) <= 0;
    }
    return false;
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
    let a: usize = 0;
    let b: usize = 0;
    svrTrimBounds(clause, &a, &b);
    if (a >= b) {
        return v.pre_len == 0; // empty clause == '*'
    }
    const s = clause[a..b];

    // Check hyphen range: "<left> - <right>"
    let hyphen_pos: usize = s.len;
    let k: usize = 1;
    while (k + 1 < s.len) {
        if (s[k] == 45 && svrIsSpace(s[k - 1]) && svrIsSpace(s[k + 1])) {
            hyphen_pos = k;
            break;
        }
        k = k + 1;
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
        while (i < s.len && svrIsSpace(s[i])) { i = i + 1; }
        if (i >= s.len) { break; }
        const t0 = i;
        while (i < s.len && !svrIsSpace(s[i])) { i = i + 1; }
        const tok = s[t0..i];

        if (svrIsBareOperator(tok)) {
            while (i < s.len && svrIsSpace(s[i])) { i = i + 1; }
            if (i >= s.len) { return false; }
            const v0 = i;
            while (i < s.len && !svrIsSpace(s[i])) { i = i + 1; }
            let merged: [64]u8 = [0; 64];
            let mpos: usize = 0;
            svrAppendSlice(merged[0..64], &mpos, tok);
            svrAppendSlice(merged[0..64], &mpos, s[v0..i]);
            if (!svrMatchSingleComparator(v, merged[0..mpos], &allow_pre)) {
                return false;
            }
        } else {
            if (!svrMatchSingleComparator(v, tok, &allow_pre)) {
                return false;
            }
        }
    }

    if (v.pre_len > 0 && !allow_pre) {
        return false;
    }
    return true;
}

/// Evaluates whether `version` satisfies npm semver `range` (supports `||`, space AND, `^`, `~`, `*`/`x`, `-`, comparators).
pub fn satisfies(version: string, range: string): bool {
    const v = parseSemver(version);
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
            i = i + 2;
            start = i;
            continue;
        }
        i = i + 1;
    }
    return false;
}

/// Returns the index of the highest version in `versions` satisfying `range`, or -1 if none.
pub fn maxSatisfying(versions: []const string, range: string): i32 {
    let best_idx: i32 = -1;
    let best_ver = SemVer.invalid();
    let i: usize = 0;
    while (i < versions.len) {
        if (satisfies(versions[i], range)) {
            const cur = parseSemver(versions[i]);
            if (best_idx < 0 || compareSemver(cur, best_ver) > 0) {
                best_idx = i as i32;
                best_ver = cur;
            }
        }
        i = i + 1;
    }
    return best_idx;
}

/// Returns the index of the lowest version in `versions` satisfying `range`, or -1 if none.
pub fn minSatisfying(versions: []const string, range: string): i32 {
    let best_idx: i32 = -1;
    let best_ver = SemVer.invalid();
    let i: usize = 0;
    while (i < versions.len) {
        if (satisfies(versions[i], range)) {
            const cur = parseSemver(versions[i]);
            if (best_idx < 0 || compareSemver(cur, best_ver) < 0) {
                best_idx = i as i32;
                best_ver = cur;
            }
        }
        i = i + 1;
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
    svrAppendU32(out, &pos, v.major);
    svrAppendByte(out, &pos, 46);
    svrAppendU32(out, &pos, v.minor);
    svrAppendByte(out, &pos, 46);
    svrAppendU32(out, &pos, v.patch);
    if (v.pre_len > 0) {
        svrAppendByte(out, &pos, 45);
        svrAppendSlice(out, &pos, v.pre[0..v.pre_len]);
    }
    if (v.build_len > 0) {
        svrAppendByte(out, &pos, 43);
        svrAppendSlice(out, &pos, v.build[0..v.build_len]);
    }
    return pos;
}

export {
    SemVer,
    parseSemver,
    compareSemver,
    satisfies,
    maxSatisfying,
    minSatisfying,
    incMajor,
    incMinor,
    incPatch,
    formatSemver,
};
