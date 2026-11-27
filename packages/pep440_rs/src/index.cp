// @cap/pep440-rs — Zero-heap PEP 440 version parser, comparator & specifier matcher (crates.io:pep440_rs)

pub enum PreKind {
    None,
    Alpha,
    Beta,
    Rc,
}

pub struct Pep440Version {
    valid: bool,
    epoch: u32,
    release: [4]u32,
    release_len: u8,
    pre_kind: PreKind,
    pre_num: u32,
    has_post: bool,
    post_num: u32,
    has_dev: bool,
    dev_num: u32,
}

fn pepInvalid(): Pep440Version {
    const z: u32 = 0;
    return Pep440Version{
        valid: false,
        epoch: 0,
        release: [z, z, z, z],
        release_len: 0,
        pre_kind: PreKind.None,
        pre_num: 0,
        has_post: false,
        post_num: 0,
        has_dev: false,
        dev_num: 0,
    };
}

fn pepIsDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn pepIsSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn pepToLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn pepIsSep(c: u8): bool {
    return c == '.' || c == '-' || c == '_';
}

fn pepMatchWord(s: string, pos: usize, end: usize, word: string): bool {
    if (pos + word.len > end) {
        return false;
    }
    let i: usize = 0;
    while (i < word.len) {
        if (pepToLower(s[pos + i]) != word[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

struct PepNumRes {
    ok: bool,
    val: u32,
    next: usize,
}

fn pepReadU32(s: string, pos: usize, end: usize): PepNumRes {
    let i = pos;
    let val: u32 = 0;
    let digits: usize = 0;
    while (i < end && pepIsDigit(s[i])) {
        const d: u32 = (s[i] - ('0' as u8)) as u32;
        val = val *% 10 +% d;
        i += 1;
        digits += 1;
    }
    return PepNumRes{
        ok: digits > 0,
        val: val,
        next: i,
    };
}

/// Parse a PEP 440 version string into `Pep440Version`.
/// Check `.valid` to determine whether parsing succeeded.
pub fn parsePep440(s: string): Pep440Version {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && pepIsSpace(s[start])) {
        start += 1;
    }
    while (end > start && pepIsSpace(s[end - 1])) {
        end -= 1;
    }
    if (start >= end) {
        return pepInvalid();
    }

    let pos = start;
    // Optional leading 'v' / 'V'
    if (s[pos] == 'v' || s[pos] == 'V') {
        pos += 1;
        if (pos >= end) {
            return pepInvalid();
        }
    }

    const z: u32 = 0;
    let ver = Pep440Version{
        valid: false,
        epoch: 0,
        release: [z, z, z, z],
        release_len: 0,
        pre_kind: PreKind.None,
        pre_num: 0,
        has_post: false,
        post_num: 0,
        has_dev: false,
        dev_num: 0,
    };

    // Optional epoch: N!
    const ep = pepReadU32(s, pos, end);
    if (ep.ok && ep.next < end && s[ep.next] == '!') {
        ver.epoch = ep.val;
        pos = ep.next + 1;
    }

    // Release segment: N(.N)*
    const first_rel = pepReadU32(s, pos, end);
    if (!first_rel.ok) {
        return pepInvalid();
    }
    ver.release[0] = first_rel.val;
    ver.release_len = 1;
    pos = first_rel.next;

    while (pos < end && s[pos] == '.') {
        const nxt = pepReadU32(s, pos + 1, end);
        if (!nxt.ok) {
            break;
        }
        if (ver.release_len < 4) {
            ver.release[ver.release_len as usize] = nxt.val;
            ver.release_len += 1;
        }
        pos = nxt.next;
    }

    // Optional pre-release: [._-]?(a|alpha|b|beta|c|rc|pre|preview)[._-]?N?
    if (pos < end) {
        let p = pos;
        if (pepIsSep(s[p])) {
            p += 1;
        }
        let matched_pre = PreKind.None;
        let word_len: usize = 0;
        if (pepMatchWord(s, p, end, "alpha")) {
            matched_pre = PreKind.Alpha;
            word_len = 5;
        } else if (pepMatchWord(s, p, end, "beta")) {
            matched_pre = PreKind.Beta;
            word_len = 4;
        } else if (pepMatchWord(s, p, end, "preview")) {
            matched_pre = PreKind.Rc;
            word_len = 7;
        } else if (pepMatchWord(s, p, end, "pre")) {
            matched_pre = PreKind.Rc;
            word_len = 3;
        } else if (pepMatchWord(s, p, end, "rc")) {
            matched_pre = PreKind.Rc;
            word_len = 2;
        } else if (pepMatchWord(s, p, end, "a")) {
            matched_pre = PreKind.Alpha;
            word_len = 1;
        } else if (pepMatchWord(s, p, end, "b")) {
            matched_pre = PreKind.Beta;
            word_len = 1;
        } else if (pepMatchWord(s, p, end, "c")) {
            matched_pre = PreKind.Rc;
            word_len = 1;
        }

        if (matched_pre != PreKind.None) {
            p += word_len;
            if (p < end && pepIsSep(s[p])) {
                p += 1;
            }
            const pre_n = pepReadU32(s, p, end);
            ver.pre_kind = matched_pre;
            ver.pre_num = if (pre_n.ok) pre_n.val else 0;
            pos = pre_n.next;
        }
    }

    // Optional post-release: ([._-]?(post|rev|r)[._-]?N?) | (-N)
    if (pos < end) {
        let p = pos;
        if (s[p] == '-' && p + 1 < end && pepIsDigit(s[p + 1])) {
            const pn = pepReadU32(s, p + 1, end);
            ver.has_post = true;
            ver.post_num = pn.val;
            pos = pn.next;
        } else {
            if (pepIsSep(s[p])) {
                p += 1;
            }
            let post_wlen: usize = 0;
            if (pepMatchWord(s, p, end, "post")) {
                post_wlen = 4;
            } else if (pepMatchWord(s, p, end, "rev")) {
                post_wlen = 3;
            } else if (pepMatchWord(s, p, end, "r")) {
                post_wlen = 1;
            }
            if (post_wlen > 0) {
                p += post_wlen;
                if (p < end && pepIsSep(s[p])) {
                    p += 1;
                }
                const pn = pepReadU32(s, p, end);
                ver.has_post = true;
                ver.post_num = if (pn.ok) pn.val else 0;
                pos = pn.next;
            }
        }
    }

    // Optional dev-release: [._-]?dev[._-]?N?
    if (pos < end) {
        let p = pos;
        if (pepIsSep(s[p])) {
            p += 1;
        }
        if (pepMatchWord(s, p, end, "dev")) {
            p += 3;
            if (p < end && pepIsSep(s[p])) {
                p += 1;
            }
            const dn = pepReadU32(s, p, end);
            ver.has_dev = true;
            ver.dev_num = if (dn.ok) dn.val else 0;
            pos = dn.next;
        }
    }

    // Optional local version: +<alnum/._->
    if (pos < end && s[pos] == '+') {
        pos += 1;
        if (pos >= end) {
            return pepInvalid();
        }
        while (pos < end) {
            const c = s[pos];
            const ok = pepIsDigit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || pepIsSep(c);
            if (!ok) {
                return pepInvalid();
            }
            pos += 1;
        }
    }

    if (pos != end) {
        return pepInvalid();
    }

    ver.valid = true;
    return ver;
}

fn pepCmpU32(a: u32, b: u32): i32 {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

fn pepPreRank(k: PreKind): i32 {
    if (k == PreKind.Alpha) return 1;
    if (k == PreKind.Beta) return 2;
    if (k == PreKind.Rc) return 3;
    return 0;
}

// PEP 440 stage ordering within same (epoch, release):
// 0 = pure dev (1.0.dev0)
// 1 = pre-release (1.0a0 / 1.0b0 / 1.0rc0, with optional post/dev)
// 2 = final (1.0)
// 3 = post-release (1.0.post0, with optional dev)
fn pepStageRank(v: Pep440Version): i32 {
    if (v.pre_kind == PreKind.None && !v.has_post && v.has_dev) {
        return 0;
    }
    if (v.pre_kind != PreKind.None) {
        return 1;
    }
    if (!v.has_post) {
        return 2;
    }
    return 3;
}

/// Compare two `Pep440Version` values according to PEP 440 total ordering:
/// `dev < a < b < rc < final < post`. Returns `-1`, `0`, or `1`.
pub fn comparePep440(a: Pep440Version, b: Pep440Version): i32 {
    const ep = pepCmpU32(a.epoch, b.epoch);
    if (ep != 0) return ep;

    let i: usize = 0;
    while (i < 4) {
        const rc = pepCmpU32(a.release[i], b.release[i]);
        if (rc != 0) return rc;
        i += 1;
    }

    const sa = pepStageRank(a);
    const sb = pepStageRank(b);
    if (sa < sb) return -1;
    if (sa > sb) return 1;

    if (sa == 0) {
        return pepCmpU32(a.dev_num, b.dev_num);
    }
    if (sa == 1) {
        const pa = pepPreRank(a.pre_kind);
        const pb = pepPreRank(b.pre_kind);
        if (pa < pb) return -1;
        if (pa > pb) return 1;
        const pn = pepCmpU32(a.pre_num, b.pre_num);
        if (pn != 0) return pn;
        if (a.has_post != b.has_post) {
            return if (a.has_post) 1 else -1;
        }
        if (a.has_post) {
            const post_c = pepCmpU32(a.post_num, b.post_num);
            if (post_c != 0) return post_c;
        }
        if (a.has_dev != b.has_dev) {
            return if (a.has_dev) -1 else 1;
        }
        if (a.has_dev) {
            return pepCmpU32(a.dev_num, b.dev_num);
        }
        return 0;
    }
    if (sa == 2) {
        return 0;
    }
    // sa == 3 (post-release)
    const post_cmp = pepCmpU32(a.post_num, b.post_num);
    if (post_cmp != 0) return post_cmp;
    if (a.has_dev != b.has_dev) {
        return if (a.has_dev) -1 else 1;
    }
    if (a.has_dev) {
        return pepCmpU32(a.dev_num, b.dev_num);
    }
    return 0;
}

fn pepPrefixMatch(v: Pep440Version, target: Pep440Version, prefix_len: u8): bool {
    if (v.epoch != target.epoch) {
        return false;
    }
    let i: u8 = 0;
    while (i < prefix_len && i < 4) {
        if (v.release[i as usize] != target.release[i as usize]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn pepMatchSingle(v: Pep440Version, raw: string): bool {
    let start: usize = 0;
    let end: usize = raw.len;
    while (start < end && pepIsSpace(raw[start])) {
        start += 1;
    }
    while (end > start && pepIsSpace(raw[end - 1])) {
        end -= 1;
    }
    if (start >= end) {
        return false;
    }

    // Operator parsing: ==, !=, >=, <=, ~=, >, <
    let op_kind: u8 = 0; // 1:==, 2:!=, 3:>=, 4:<=, 5:~=, 6:>, 7:<
    let rhs_start = start;
    if (start + 1 < end && raw[start] == '=' && raw[start + 1] == '=') {
        op_kind = 1;
        rhs_start = start + 2;
    } else if (start + 1 < end && raw[start] == '!' && raw[start + 1] == '=') {
        op_kind = 2;
        rhs_start = start + 2;
    } else if (start + 1 < end && raw[start] == '>' && raw[start + 1] == '=') {
        op_kind = 3;
        rhs_start = start + 2;
    } else if (start + 1 < end && raw[start] == '<' && raw[start + 1] == '=') {
        op_kind = 4;
        rhs_start = start + 2;
    } else if (start + 1 < end && raw[start] == '~' && raw[start + 1] == '=') {
        op_kind = 5;
        rhs_start = start + 2;
    } else if (raw[start] == '>') {
        op_kind = 6;
        rhs_start = start + 1;
    } else if (raw[start] == '<') {
        op_kind = 7;
        rhs_start = start + 1;
    } else {
        return false;
    }

    while (rhs_start < end && pepIsSpace(raw[rhs_start])) {
        rhs_start += 1;
    }
    if (rhs_start >= end) {
        return false;
    }

    // Wildcard support for == and != (e.g. ==1.2.*)
    let has_wildcard: bool = false;
    let rhs_end = end;
    if ((op_kind == 1 || op_kind == 2) && rhs_end >= rhs_start + 2 && raw[rhs_end - 1] == '*' && raw[rhs_end - 2] == '.') {
        has_wildcard = true;
        rhs_end -= 2;
    }

    const target = parsePep440(raw[rhs_start..rhs_end]);
    if (!target.valid) {
        return false;
    }

    if (has_wildcard) {
        const matched = pepPrefixMatch(v, target, target.release_len);
        return if (op_kind == 1) matched else !matched;
    }

    const cmp = comparePep440(v, target);
    if (op_kind == 1) return cmp == 0;
    if (op_kind == 2) return cmp != 0;
    if (op_kind == 3) return cmp >= 0;
    if (op_kind == 4) return cmp <= 0;
    if (op_kind == 6) return cmp > 0;
    if (op_kind == 7) return cmp < 0;

    // op_kind == 5 (~= compatible release)
    if (target.release_len < 2) {
        return false;
    }
    if (cmp < 0) {
        return false;
    }
    return pepPrefixMatch(v, target, target.release_len - 1);
}

/// Evaluate whether `ver` satisfies a PEP 440 specifier expression `spec`.
/// Supports comma-separated AND clauses and operators `==`, `!=`, `>=`, `<=`, `>`, `<`, `~=`.
pub fn matchesSpecifier(ver: string, spec: string): bool {
    const v = parsePep440(ver);
    if (!v.valid || spec.len == 0) {
        return false;
    }

    let pos: usize = 0;
    let clause_count: usize = 0;
    while (pos <= spec.len) {
        let comma = pos;
        while (comma < spec.len && spec[comma] != ',') {
            comma += 1;
        }
        const clause = spec[pos..comma];
        if (!pepMatchSingle(v, clause)) {
            return false;
        }
        clause_count += 1;
        if (comma >= spec.len) {
            break;
        }
        pos = comma + 1;
    }

    return clause_count > 0;
}
