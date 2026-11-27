// @cap/publicsuffix — Zero-heap pure-CP Public Suffix List (PSL) parser
// Clean-room MIT equivalent of crates.io `publicsuffix`.

pub const PSL_MAX_RULES: usize = 64;
pub const PSL_RULE_NORMAL: u8 = 0;
pub const PSL_RULE_WILDCARD: u8 = 1;
pub const PSL_RULE_EXCEPTION: u8 = 2;

fn pslToLower(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn pslIsLabelChar(b: u8): bool {
    if (b >= 97 && b <= 122) { return true; } // a-z
    if (b >= 48 && b <= 57) { return true; }  // 0-9
    if (b == 45) { return true; }             // '-'
    return false;
}

fn pslBufEqLowerStr(buf: [256]u8, start: usize, len: usize, s: string): bool {
    if (len != s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[start + i] != pslToLower(s[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn pslCountPatternLabels(pat: string): usize {
    if (pat.len == 0) {
        return 0;
    }
    let cnt: usize = 1;
    let i: usize = 0;
    while (i < pat.len) {
        if (pat[i] == 46) {
            cnt = cnt + 1;
        }
        i = i + 1;
    }
    return cnt;
}

fn pslMatchSuffixPattern(
    norm: [256]u8,
    norm_len: usize,
    label_starts: [128]usize,
    label_count: usize,
    pat: string
): usize {
    let pat_labels: usize = pslCountPatternLabels(pat);
    if (pat_labels == 0 || pat_labels > label_count) {
        return 0;
    }
    let tail_start: usize = label_starts[label_count - pat_labels];
    let tail_len: usize = norm_len - tail_start;
    if (pslBufEqLowerStr(norm, tail_start, tail_len, pat)) {
        return pat_labels;
    }
    return 0;
}

fn pslMatchBuiltin3(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool {
    if (label_count < 3) {
        return false;
    }
    let start: usize = label_starts[label_count - 3];
    let len: usize = norm_len - start;
    if (pslBufEqLowerStr(norm, start, len, "s3.amazonaws.com")) { return true; }
    return false;
}

fn pslMatchBuiltin2(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool {
    if (label_count < 2) {
        return false;
    }
    let start: usize = label_starts[label_count - 2];
    let len: usize = norm_len - start;
    if (pslBufEqLowerStr(norm, start, len, "co.uk")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "org.uk")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "ac.uk")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "gov.uk")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "me.uk")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "net.uk")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "com.cn")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "net.cn")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "org.cn")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "gov.cn")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "edu.cn")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "ac.cn")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "co.jp")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "ne.jp")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "or.jp")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "ac.jp")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "go.jp")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "ed.jp")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "com.au")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "net.au")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "org.au")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "edu.au")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "gov.au")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "com.br")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "net.br")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "org.br")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "co.kr")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "or.kr")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "com.tw")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "com.hk")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "com.sg")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "co.in")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "co.nz")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "github.io")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "gitlab.io")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "pages.dev")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "workers.dev")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "amazonaws.com")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "appspot.com")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "herokuapp.com")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "netlify.app")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "vercel.app")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "fly.dev")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "cloudfront.net")) { return true; }
    return false;
}

fn pslMatchBuiltin1(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool {
    if (label_count < 1) {
        return false;
    }
    let start: usize = label_starts[label_count - 1];
    let len: usize = norm_len - start;
    if (pslBufEqLowerStr(norm, start, len, "com")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "org")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "net")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "io")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "dev")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "ai")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "cn")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "jp")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "uk")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "de")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "fr")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "au")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "app")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "xyz")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "edu")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "gov")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "info")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "biz")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "me")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "co")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "us")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "ca")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "br")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "kr")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "in")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "nl")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "se")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "ch")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "tw")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "hk")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "sg")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "nz")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "sh")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "gg")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "tv")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "cc")) { return true; }
    if (pslBufEqLowerStr(norm, start, len, "cloud")) { return true; }
    return false;
}

pub struct PslRule {
    pub pattern: string,
    pub kind: u8,
}

pub struct DomainInfo {
    pub norm: [256]u8,
    pub norm_len: usize,
    pub suffix_start: usize,
    pub suffix_len: usize,
    pub root_start: usize,
    pub root_len: usize,
    pub has_root: bool,
    pub is_known_suffix: bool,

    pub fn writeHost(out: []u8): usize {
        let n: usize = if (_.norm_len < out.len) { _.norm_len } else { out.len };
        let i: usize = 0;
        while (i < n) {
            out[i] = _.norm[i];
            i = i + 1;
        }
        return n;
    }

    pub fn writeSuffix(out: []u8): usize {
        let n: usize = if (_.suffix_len < out.len) { _.suffix_len } else { out.len };
        let i: usize = 0;
        while (i < n) {
            out[i] = _.norm[_.suffix_start + i];
            i = i + 1;
        }
        return n;
    }

    pub fn writeRegistrable(out: []u8): usize {
        if (!_.has_root) {
            return 0;
        }
        let n: usize = if (_.root_len < out.len) { _.root_len } else { out.len };
        let i: usize = 0;
        while (i < n) {
            out[i] = _.norm[_.root_start + i];
            i = i + 1;
        }
        return n;
    }

    pub fn suffixEq(s: string): bool {
        return pslBufEqLowerStr(_.norm, _.suffix_start, _.suffix_len, s);
    }

    pub fn registrableEq(s: string): bool {
        if (!_.has_root) {
            return false;
        }
        return pslBufEqLowerStr(_.norm, _.root_start, _.root_len, s);
    }

    pub fn hostEq(s: string): bool {
        return pslBufEqLowerStr(_.norm, 0, _.norm_len, s);
    }
}

pub struct PslRuleList {
    pub rules: [64]PslRule,
    pub count: usize,
    pub include_builtin: bool,

    pub static fn new(): PslRuleList {
        let empty_rules: [64]PslRule = [PslRule { pattern: "", kind: 0 }; 64];
        return PslRuleList {
            rules: empty_rules,
            count: 0,
            include_builtin: true,
        };
    }

    pub static fn empty(): PslRuleList {
        let empty_rules: [64]PslRule = [PslRule { pattern: "", kind: 0 }; 64];
        return PslRuleList {
            rules: empty_rules,
            count: 0,
            include_builtin: false,
        };
    }

    pub fn addRule(*_, rule: string): bool {
        if (_.count >= PSL_MAX_RULES || rule.len == 0) {
            return false;
        }
        let kind: u8 = PSL_RULE_NORMAL;
        let pat: string = rule;
        if (rule[0] == 33) { // '!'
            if (rule.len <= 1) {
                return false;
            }
            kind = PSL_RULE_EXCEPTION;
            pat = rule[1..rule.len];
        } else if (rule.len >= 2 && rule[0] == 42 && rule[1] == 46) { // "*."
            if (rule.len <= 2) {
                return false;
            }
            kind = PSL_RULE_WILDCARD;
            pat = rule[2..rule.len];
        }
        _.rules[_.count] = PslRule { pattern: pat, kind: kind };
        _.count = _.count + 1;
        return true;
    }

    pub fn parseDomain(host: string): DomainInfo? {
        if (host.len == 0) {
            return null;
        }
        let raw_len: usize = host.len;
        if (host[raw_len - 1] == 46) {
            raw_len = raw_len - 1;
        }
        if (raw_len == 0 || raw_len > 253) {
            return null;
        }

        let norm: [256]u8 = undefined;
        let label_starts: [128]usize = undefined;
        let label_count: usize = 0;

        let i: usize = 0;
        let label_len: usize = 0;
        let label_start: usize = 0;
        while (i < raw_len) {
            let ch: u8 = pslToLower(host[i]);
            if (ch == 46) { // '.'
                if (label_len == 0 || label_len > 63) {
                    return null;
                }
                if (norm[label_start] == 45 || norm[i - 1] == 45) {
                    return null;
                }
                if (label_count >= 128) {
                    return null;
                }
                label_starts[label_count] = label_start;
                label_count = label_count + 1;
                norm[i] = 46;
                label_start = i + 1;
                label_len = 0;
            } else {
                if (!pslIsLabelChar(ch)) {
                    return null;
                }
                norm[i] = ch;
                label_len = label_len + 1;
            }
            i = i + 1;
        }
        if (label_len == 0 || label_len > 63) {
            return null;
        }
        if (norm[label_start] == 45 || norm[raw_len - 1] == 45) {
            return null;
        }
        if (label_count >= 128) {
            return null;
        }
        label_starts[label_count] = label_start;
        label_count = label_count + 1;

        // Evaluate matching rules: Exception > Longest Exact/Wildcard > Builtin > Default 1-label TLD
        let best_exc: usize = 0;
        let best_exact: usize = 0;
        let best_wild: usize = 0;
        let known: bool = false;

        let r: usize = 0;
        while (r < _.count) {
            let rule = _.rules[r];
            let matched = pslMatchSuffixPattern(norm, raw_len, label_starts, label_count, rule.pattern);
            if (matched > 0) {
                if (rule.kind == PSL_RULE_EXCEPTION) {
                    if (matched > best_exc) {
                        best_exc = matched;
                    }
                    known = true;
                } else if (rule.kind == PSL_RULE_NORMAL) {
                    if (matched > best_exact) {
                        best_exact = matched;
                    }
                    known = true;
                } else if (rule.kind == PSL_RULE_WILDCARD) {
                    if (label_count >= matched + 1) {
                        let eff: usize = matched + 1;
                        if (eff > best_wild) {
                            best_wild = eff;
                        }
                        known = true;
                    }
                }
            }
            r = r + 1;
        }

        if (_.include_builtin) {
            if (pslMatchBuiltin3(norm, raw_len, label_starts, label_count)) {
                if (3 > best_exact) { best_exact = 3; }
                known = true;
            } else if (pslMatchBuiltin2(norm, raw_len, label_starts, label_count)) {
                if (2 > best_exact) { best_exact = 2; }
                known = true;
            } else if (pslMatchBuiltin1(norm, raw_len, label_starts, label_count)) {
                if (1 > best_exact) { best_exact = 1; }
                known = true;
            }
        }

        let suffix_labels: usize = 1;
        if (best_exc > 0) {
            suffix_labels = if (best_exc > 1) { best_exc - 1 } else { 1 };
        } else if (best_exact > 0 || best_wild > 0) {
            if (best_exact >= best_wild) {
                suffix_labels = best_exact;
            } else {
                suffix_labels = best_wild;
            }
        } else {
            suffix_labels = 1;
        }

        let suffix_start: usize = label_starts[label_count - suffix_labels];
        let suffix_len: usize = raw_len - suffix_start;

        let has_root: bool = label_count > suffix_labels;
        let root_start: usize = 0;
        let root_len: usize = 0;
        if (has_root) {
            root_start = label_starts[label_count - suffix_labels - 1];
            root_len = raw_len - root_start;
        }

        return DomainInfo {
            norm: norm,
            norm_len: raw_len,
            suffix_start: suffix_start,
            suffix_len: suffix_len,
            root_start: root_start,
            root_len: root_len,
            has_root: has_root,
            is_known_suffix: known,
        };
    }

    pub fn suffix(host: string): string? {
        let info_opt = _.parseDomain(host);
        if (info_opt == null) {
            return null;
        }
        let info = info_opt ?? DomainInfo {
            norm: undefined,
            norm_len: 0,
            suffix_start: 0,
            suffix_len: 0,
            root_start: 0,
            root_len: 0,
            has_root: false,
            is_known_suffix: false,
        };
        return host[info.suffix_start..(info.suffix_start + info.suffix_len)];
    }

    pub fn registrableDomain(host: string): string? {
        let info_opt = _.parseDomain(host);
        if (info_opt == null) {
            return null;
        }
        let info = info_opt ?? DomainInfo {
            norm: undefined,
            norm_len: 0,
            suffix_start: 0,
            suffix_len: 0,
            root_start: 0,
            root_len: 0,
            has_root: false,
            is_known_suffix: false,
        };
        if (!info.has_root) {
            return null;
        }
        return host[info.root_start..(info.root_start + info.root_len)];
    }

    pub fn isPublicSuffix(host: string): bool {
        let info_opt = _.parseDomain(host);
        if (info_opt == null) {
            return false;
        }
        let info = info_opt ?? DomainInfo {
            norm: undefined,
            norm_len: 0,
            suffix_start: 0,
            suffix_len: 0,
            root_start: 0,
            root_len: 0,
            has_root: false,
            is_known_suffix: false,
        };
        return !info.has_root && info.is_known_suffix;
    }
}

pub fn parseDomain(host: string): DomainInfo? {
    let list = PslRuleList.new();
    return list.parseDomain(host);
}

pub fn suffix(host: string): string? {
    let list = PslRuleList.new();
    return list.suffix(host);
}

pub fn registrableDomain(host: string): string? {
    let list = PslRuleList.new();
    return list.registrableDomain(host);
}

pub fn isPublicSuffix(host: string): bool {
    let list = PslRuleList.new();
    return list.isPublicSuffix(host);
}
