// Source: Absorbed from /data/dev/bun/src/http/ | Synced: 2026-11-22T12:49:17Z (RFC 9110 Accept-Encoding Negotiator)

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

/// RFC 9110 §5.6.2 `tchar` 字符判定
fn isTchar(c: u8): bool {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
        return true;
    }
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&'
        || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.'
        || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

fn isValidHttpToken(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!isTchar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
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

/// HTTP 内容编码分类
export enum EncodingKind {
    identity,
    gzip,
    deflate,
    br,
    zstd,
    wildcard,
    other,
}

fn classifyEncoding(coding: []const u8): EncodingKind {
    if (coding.len == 1 && coding[0] == '*') {
        return EncodingKind.wildcard;
    }
    if (eqlIgnoreCaseAscii(coding, "identity")) {
        return EncodingKind.identity;
    }
    if (eqlIgnoreCaseAscii(coding, "gzip") || eqlIgnoreCaseAscii(coding, "x-gzip")) {
        return EncodingKind.gzip;
    }
    if (eqlIgnoreCaseAscii(coding, "deflate")) {
        return EncodingKind.deflate;
    }
    if (eqlIgnoreCaseAscii(coding, "br")) {
        return EncodingKind.br;
    }
    if (eqlIgnoreCaseAscii(coding, "zstd")) {
        return EncodingKind.zstd;
    }
    return EncodingKind.other;
}

fn codingsMatch(a: []const u8, b: []const u8): bool {
    if (eqlIgnoreCaseAscii(a, b)) {
        return true;
    }
    const ka = classifyEncoding(a);
    const kb = classifyEncoding(b);
    return ka == EncodingKind.gzip && kb == EncodingKind.gzip;
}

/// RFC 9110 §12.5.3 编码条目与千分比权重
export struct EncodingItem {
    coding: []const u8;
    kind: EncodingKind;
    q_permille: u16;
}

fn parseEncodingItem(raw: []const u8): EncodingItem? {
    const item = trimOws(raw);
    if (item.len == 0) {
        return null;
    }

    let pos: usize = 0;
    while (pos < item.len && item[pos] != ';') {
        pos += 1;
    }
    const coding = trimOws(item[0..pos]);
    if (!isValidHttpToken(coding)) {
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

    return EncodingItem{
        coding: coding,
        kind: classifyEncoding(coding),
        q_permille: q_permille,
    };
}

/// RFC 9110 §12.5.3 `Accept-Encoding` 零分配编码迭代器
export struct AcceptEncodingIter {
    src: []const u8;
    pos: usize;

    static fn init(header: []const u8): AcceptEncodingIter {
        return AcceptEncodingIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): EncodingItem? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const seg = _.src[seg_start.._.pos];
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (parsed := parseEncodingItem(seg)) {
                return parsed;
            }
        }
        return null;
    }
}

struct EvalScore {
    acceptable: bool;
    implicit: bool;
    q_permille: u16;
}

fn evaluateCandidate(header: []const u8, coding: []const u8): EvalScore {
    if (!isValidHttpToken(coding) || (coding.len == 1 && coding[0] == '*')) {
        return EvalScore{ acceptable: false, implicit: false, q_permille: 0 };
    }

    let exact_found = false;
    let exact_q: u16 = 0;
    let star_found = false;
    let star_q: u16 = 0;

    let it = AcceptEncodingIter.init(header);
    while (item := it.next()) {
        if (item.kind == EncodingKind.wildcard) {
            if (!star_found || item.q_permille > star_q) {
                star_found = true;
                star_q = item.q_permille;
            }
        } else if (codingsMatch(item.coding, coding)) {
            if (!exact_found || item.q_permille > exact_q) {
                exact_found = true;
                exact_q = item.q_permille;
            }
        }
    }

    if (exact_found) {
        return EvalScore{
            acceptable: exact_q > 0,
            implicit: false,
            q_permille: exact_q,
        };
    }
    if (star_found) {
        return EvalScore{
            acceptable: star_q > 0,
            implicit: false,
            q_permille: star_q,
        };
    }
    if (eqlIgnoreCaseAscii(coding, "identity")) {
        return EvalScore{
            acceptable: true,
            implicit: true,
            q_permille: 1,
        };
    }
    return EvalScore{ acceptable: false, implicit: false, q_permille: 0 };
}

/// 判断给定编码在 `Accept-Encoding` 头部下是否可接受（RFC 9110 §12.5.3）
export fn isEncodingAcceptable(header: []const u8, coding: []const u8): bool {
    return evaluateCandidate(header, coding).acceptable;
}

/// 按 RFC 9110 §12.5.3 协商最佳编码：
/// - 精确匹配优先于 `*` 通配符（含 `gzip`/`x-gzip` 等价）
/// - `q=0` 显式禁止
/// - `identity` 未被显式拒绝且未被 `*;q=0` 禁止时可作为默认回退
/// - 按最高权重与服务端 `supported` 列表优先级选出最佳编码；无可接受编码返回 `null`
export fn negotiateEncoding(header: []const u8, supported: []const []const u8): ([]const u8)? {
    let best_found = false;
    let best_coding: []const u8 = "";
    let best_implicit = true;
    let best_q: u16 = 0;

    let i: usize = 0;
    while (i < supported.len) {
        const cand = supported[i];
        i += 1;

        const score = evaluateCandidate(header, cand);
        if (!score.acceptable) {
            continue;
        }

        const better = !best_found
            || (!score.implicit && best_implicit)
            || (score.implicit == best_implicit && score.q_permille > best_q);

        if (better) {
            best_found = true;
            best_coding = cand;
            best_implicit = score.implicit;
            best_q = score.q_permille;
        }
    }

    if (best_found) {
        return best_coding;
    }
    return null;
}
