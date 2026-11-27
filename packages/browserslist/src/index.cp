// @cap/browserslist-query — Zero-heap Browserslist query clause parser & matcher
// Clean-room CP implementation inspired by browserslist (MIT).

import { isSpace, isDigit, trim, eqIgnoreCase, findByte } from "cap:ascii";

pub enum BrowserOp {
    Ge,
    Gt,
    Le,
    Lt,
    Eq,
    LastN,
    Defaults,
    Dead,
    Unknown,
}

pub struct BrowserClause {
    negated: bool,
    is_and: bool,
    browser: string,
    op: BrowserOp,
    version_major: u32,
    version_minor: u32,
    count: u32,
}

fn blqStartsWithWord(s: string, word: string): bool {
    if (s.len <= word.len) return false;
    return eqIgnoreCase(s[0..word.len], word) && isSpace(s[word.len]);
}

fn blqParseU32(s: string): u32 {
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len && isDigit(s[i])) : (i += 1) {
        val = val * 10 + u32(s[i] - '0');
    }
    return val;
}

fn blqIsDeadBrowser(browser: string): bool {
    if (eqIgnoreCase(browser, "ie")) return true;
    if (eqIgnoreCase(browser, "ie_mob")) return true;
    if (eqIgnoreCase(browser, "bb")) return true;
    if (eqIgnoreCase(browser, "op_mini")) return true;
    if (eqIgnoreCase(browser, "baidu")) return true;
    if (eqIgnoreCase(browser, "kaios")) return true;
    return false;
}

fn blqCmpVersion(maj_a: u32, min_a: u32, maj_b: u32, min_b: u32): i32 {
    if (maj_a < maj_b) return -1;
    if (maj_a > maj_b) return 1;
    if (min_a < min_b) return -1;
    if (min_a > min_b) return 1;
    return 0;
}

pub fn parseBrowserClause(clause: string): BrowserClause {
    let res = BrowserClause{
        negated: false,
        is_and: false,
        browser: "",
        op: BrowserOp.Unknown,
        version_major: 0,
        version_minor: 0,
        count: 0,
    };

    let s: string = trim(clause);
    if (s.len == 0) return res;

    if (blqStartsWithWord(s, "and")) {
        res.is_and = true;
        s = trim(s[3..s.len]);
    }

    if (blqStartsWithWord(s, "not")) {
        res.negated = true;
        s = trim(s[3..s.len]);
    }

    if (eqIgnoreCase(s, "defaults")) {
        res.op = BrowserOp.Defaults;
        return res;
    }

    if (eqIgnoreCase(s, "dead")) {
        res.op = BrowserOp.Dead;
        return res;
    }

    // "last N [browser] version(s)"
    if (blqStartsWithWord(s, "last")) {
        let rest: string = trim(s[4..s.len]);
        let num_end: usize = 0;
        while (num_end < rest.len && isDigit(rest[num_end])) : (num_end += 1) {}
        if (num_end == 0) return res;
        res.count = blqParseU32(rest[0..num_end]);
        rest = trim(rest[num_end..rest.len]);

        // Strip trailing "versions" or "version"
        let last_space: usize = rest.len;
        let idx: usize = rest.len;
        while (idx > 0) {
            idx -= 1;
            if (isSpace(rest[idx])) {
                last_space = idx;
                break;
            }
        }
        if (last_space < rest.len) {
            const tail: string = rest[(last_space + 1)..rest.len];
            if (eqIgnoreCase(tail, "versions") || eqIgnoreCase(tail, "version")) {
                res.browser = trim(rest[0..last_space]);
                res.op = BrowserOp.LastN;
                return res;
            }
        } else if (eqIgnoreCase(rest, "versions") || eqIgnoreCase(rest, "version")) {
            res.browser = "";
            res.op = BrowserOp.LastN;
            return res;
        }
        return res;
    }

    // "<browser> <op> <major>[.<minor>]"
    let op_start: usize = s.len;
    let op_len: usize = 0;
    let op_kind = BrowserOp.Unknown;

    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c == '>' && i + 1 < s.len && s[i + 1] == '=') {
            op_start = i;
            op_len = 2;
            op_kind = BrowserOp.Ge;
            break;
        } else if (c == '<' && i + 1 < s.len && s[i + 1] == '=') {
            op_start = i;
            op_len = 2;
            op_kind = BrowserOp.Le;
            break;
        } else if (c == '>') {
            op_start = i;
            op_len = 1;
            op_kind = BrowserOp.Gt;
            break;
        } else if (c == '<') {
            op_start = i;
            op_len = 1;
            op_kind = BrowserOp.Lt;
            break;
        } else if (c == '=') {
            op_start = i;
            op_len = 1;
            op_kind = BrowserOp.Eq;
            break;
        }
    }

    if (op_len == 0) return res;

    res.browser = trim(s[0..op_start]);
    res.op = op_kind;

    const ver_str: string = trim(s[(op_start + op_len)..s.len]);
    const dot_idx: usize = findByte(ver_str, '.', 0) ?? ver_str.len;

    res.version_major = blqParseU32(ver_str[0..dot_idx]);
    if (dot_idx + 1 < ver_str.len) {
        res.version_minor = blqParseU32(ver_str[(dot_idx + 1)..ver_str.len]);
    }
    return res;
}

pub fn matchBrowserVersion(
    clause: BrowserClause,
    browser: string,
    major: u32,
    minor: u32,
    latest_major: u32,
): bool {
    let raw_match: bool = false;

    if (clause.op == BrowserOp.Defaults) {
        raw_match = !blqIsDeadBrowser(browser) && major <= latest_major && major + 2 > latest_major;
    } else if (clause.op == BrowserOp.Dead) {
        raw_match = blqIsDeadBrowser(browser);
    } else if (clause.op == BrowserOp.LastN) {
        const name_ok: bool = clause.browser.len == 0 || eqIgnoreCase(clause.browser, browser);
        raw_match = name_ok && clause.count > 0 && major <= latest_major && major + clause.count > latest_major;
    } else if (clause.op == BrowserOp.Unknown) {
        raw_match = false;
    } else {
        if (eqIgnoreCase(clause.browser, browser)) {
            const cmp: i32 = blqCmpVersion(major, minor, clause.version_major, clause.version_minor);
            if (clause.op == BrowserOp.Ge) {
                raw_match = cmp >= 0;
            } else if (clause.op == BrowserOp.Gt) {
                raw_match = cmp > 0;
            } else if (clause.op == BrowserOp.Le) {
                raw_match = cmp <= 0;
            } else if (clause.op == BrowserOp.Lt) {
                raw_match = cmp < 0;
            } else if (clause.op == BrowserOp.Eq) {
                raw_match = cmp == 0;
            }
        }
    }

    if (clause.negated) return !raw_match;
    return raw_match;
}
