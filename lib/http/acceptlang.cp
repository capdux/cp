// Source: Absorbed from /data/dev/bun/src/http/ | Synced: 2026-11-22T11:12:13Z (RFC 9110 Accept-Language Negotiator)

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) {
        start += 1;
    }
    while (end > start && isOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isAlpha(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn isAlnum(c: u8): bool {
    return isAlpha(c) || (c >= '0' && c <= '9');
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// RFC 4647 §2.1 `language-range = (1*8ALPHA *("-" 1*8alphanum)) / "*"`
fn isValidLanguageRange(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    if (s.len == 1 && s[0] == '*') {
        return true;
    }

    let pos: usize = 0;
    let subtag_idx: usize = 0;
    while (pos < s.len) {
        const seg_start = pos;
        while (pos < s.len && s[pos] != '-') {
            const c = s[pos];
            if (subtag_idx == 0) {
                if (!isAlpha(c)) {
                    return false;
                }
            } else {
                if (!isAlnum(c)) {
                    return false;
                }
            }
            pos += 1;
        }
        const seg_len = pos - seg_start;
        if (seg_len == 0 || seg_len > 8) {
            return false;
        }
        if (pos < s.len && s[pos] == '-') {
            pos += 1;
            if (pos == s.len) {
                return false;
            }
        }
        subtag_idx += 1;
    }
    return subtag_idx > 0;
}

/// RFC 9110 §12.4.2 `qvalue` 解析为整数千分比 `0..=1000`
fn parseQValue(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5) {
        return null;
    }
    if (s[0] == '1') {
        if (s.len == 1) {
            return u16(1000);
        }
        if (s[1] != '.') {
            return null;
        }
        let i: usize = 2;
        while (i < s.len) {
            if (s[i] != '0') {
                return null;
            }
            i += 1;
        }
        return u16(1000);
    }
    if (s[0] == '0') {
        if (s.len == 1) {
            return u16(0);
        }
        if (s[1] != '.') {
            return null;
        }
        let val: u16 = 0;
        let factor: u16 = 100;
        let i: usize = 2;
        while (i < s.len) {
            const c = s[i];
            if (c < '0' || c > '9') {
                return null;
            }
            val += u16(c - '0') * factor;
            factor /= 10;
            i += 1;
        }
        return val;
    }
    return null;
}

fn parseRangeItem(raw: []const u8): LanguageRange? {
    const item = trimOws(raw);
    if (item.len == 0) {
        return null;
    }

    let pos: usize = 0;
    while (pos < item.len && item[pos] != ';') {
        pos += 1;
    }
    const tag = trimOws(item[0..pos]);
    if (!isValidLanguageRange(tag)) {
        return null;
    }

    let q_permille: u16 = 1000;
    let has_q = false;

    while (pos < item.len) {
        pos += 1; // skip ';'
        const param_start = pos;
        while (pos < item.len && item[pos] != ';') {
            pos += 1;
        }
        const param = trimOws(item[param_start..pos]);
        if (param.len == 0) {
            return null;
        }

        let eq_pos: usize = 0;
        let found_eq = false;
        while (eq_pos < param.len) {
            if (param[eq_pos] == '=') {
                found_eq = true;
                break;
            }
            eq_pos += 1;
        }
        if (!found_eq) {
            return null;
        }

        const key = trimOws(param[0..eq_pos]);
        const val = trimOws(param[(eq_pos + 1)..param.len]);
        if (!eqlIgnoreCaseAscii(key, "q") || has_q) {
            return null;
        }
        if (q := parseQValue(val)) {
            q_permille = q;
            has_q = true;
        } else {
            return null;
        }
    }

    return LanguageRange{
        tag: tag,
        q_permille: q_permille,
    };
}

/// RFC 9110 §12.5.4 + RFC 4647 语言范围与千分比权重
export struct LanguageRange {
    tag: []const u8;
    q_permille: u16;

    /// 判断是否为通配符 `"*"`
    fn isWildcard(): bool {
        return _.tag.len == 1 && _.tag[0] == '*';
    }

    /// RFC 4647 基本前缀匹配（大小写不敏感；`"en"` 匹配 `"en"` 与 `"en-US"`，不匹配 `"eng"`；`"*"` 匹配任意非空标签）
    fn matchesTag(candidate: []const u8): bool {
        if (candidate.len == 0) {
            return false;
        }
        if (_.isWildcard()) {
            return true;
        }
        if (candidate.len < _.tag.len) {
            return false;
        }
        if (!eqlIgnoreCaseAscii(candidate[0.._.tag.len], _.tag)) {
            return false;
        }
        if (candidate.len == _.tag.len) {
            return true;
        }
        return candidate[_.tag.len] == '-';
    }
}

/// RFC 9110 `Accept-Language` 零分配语言范围迭代器
export struct AcceptLanguageIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): AcceptLanguageIter {
        return AcceptLanguageIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): LanguageRange? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const seg = _.src[seg_start.._.pos];
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (parsed := parseRangeItem(seg)) {
                return parsed;
            }
        }
        return null;
    }
}

/// 按 RFC 9110 §12.5.4 规则对 `supported` 候选语言列表执行权重与最具体标签优先级协商，
/// 排除 `q=0` 显式拒绝项，返回最佳匹配语言；无匹配返回 `null`
export fn negotiateLanguage(header: []const u8, supported: []const []const u8): ([]const u8)? {
    let best_found = false;
    let best_lang: []const u8 = "";
    let best_q: u16 = 0;
    let best_spec: usize = 0;

    let i: usize = 0;
    while (i < supported.len) {
        const cand = supported[i];
        i += 1;
        if (cand.len == 0) {
            continue;
        }

        let matched = false;
        let cand_spec: usize = 0;
        let cand_q: u16 = 0;

        let it = AcceptLanguageIter.init(header);
        while (range := it.next()) {
            if (!range.matchesTag(cand)) {
                continue;
            }
            const spec: usize = if (range.isWildcard()) 0 else range.tag.len;
            if (!matched || spec > cand_spec || (spec == cand_spec && range.q_permille > cand_q)) {
                matched = true;
                cand_spec = spec;
                cand_q = range.q_permille;
            }
        }

        if (!matched || cand_q == 0) {
            continue;
        }

        if (!best_found || cand_q > best_q || (cand_q == best_q && cand_spec > best_spec)) {
            best_found = true;
            best_lang = cand;
            best_q = cand_q;
            best_spec = cand_spec;
        }
    }

    if (best_found) {
        return best_lang;
    }
    return null;
}
