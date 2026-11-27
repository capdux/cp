// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 9110 Accept / Accept-Encoding / Accept-Language Content Negotiation)
// Pure-CP RFC 9110 content negotiation: q-value parser, header iterators, and best-match selection.

type AcceptVec16 = @Vector(16, u8);

fn isAsciiSpace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isAsciiSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isAsciiSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: AcceptVec16 = a[i..][0..16].*;
        const vb: AcceptVec16 = b[i..][0..16].*;
        const m_eq: u16 = @bitCast(va == vb);
        if (m_eq == 0xFFFF) {
            i += 16;
            continue;
        }
        let j: usize = 0;
        while (j < 16) : (j += 1) {
            if (toLowerAscii(a[i + j]) != toLowerAscii(b[i + j])) return false;
        }
        i += 16;
    }
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn indexOfByte(s: []const u8, target: u8): usize? {
    let i: usize = 0;
    const n = s.len;
    const v_t: AcceptVec16 = @splat(target);
    while (i + 16 <= n) {
        const chunk: AcceptVec16 = s[i..][0..16].*;
        const mask: u16 = @bitCast(chunk == v_t);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) {
        if (s[i] == target) return i;
        i += 1;
    }
    return null;
}

fn acceptSimdFindAny2(s: []const u8, start: usize, a: u8, b: u8): usize {
    let i: usize = start;
    const n = s.len;
    const va: AcceptVec16 = @splat(a);
    const vb: AcceptVec16 = @splat(b);
    while (i + 16 <= n) {
        const chunk: AcceptVec16 = s[i..][0..16].*;
        const ma: u16 = @bitCast(chunk == va);
        const mb: u16 = @bitCast(chunk == vb);
        const mask: u16 = ma | mb;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const c = s[i];
        if (c == a || c == b) return i;
    }
    return n;
}

/// Strict RFC 9110 §12.4.2 quality value parser -> integer millis in [0, 1000].
/// Valid forms: "0", "0.", "0.d", "0.dd", "0.ddd", "1", "1.", "1.0", "1.00", "1.000".
export fn parseQValue(raw: []const u8): u16? {
    const s = trimAscii(raw);
    if (s.len == 0 || s.len > 5) return null;
    if (s[0] == '1') {
        if (s.len == 1) return 1000;
        if (s[1] != '.') return null;
        let i: usize = 2;
        while (i < s.len) {
            if (s[i] != '0') return null;
            i += 1;
        }
        return 1000;
    }
    if (s[0] == '0') {
        if (s.len == 1) return 0;
        if (s[1] != '.') return null;
        let val: u16 = 0;
        let mult: u16 = 100;
        let i: usize = 2;
        while (i < s.len) {
            const c = s[i];
            if (c < '0' || c > '9') return null;
            val += @as(u16, c - '0') * mult;
            mult /= 10;
            i += 1;
        }
        return val;
    }
    return null;
}

/// Split next comma-separated item while respecting double-quoted strings and backslash escapes.
fn nextCommaItem(src: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < src.len) {
        const start = pos.*;
        let in_quote = false;
        let escaped = false;
        while (pos.* < src.len) {
            if (in_quote) {
                if (escaped) {
                    escaped = false;
                    pos.* += 1;
                    continue;
                }
                pos.* = acceptSimdFindAny2(src, pos.*, '"', '\\');
                if (pos.* >= src.len) break;
                if (src[pos.*] == '\\') {
                    escaped = true;
                } else {
                    in_quote = false;
                }
                pos.* += 1;
            } else {
                pos.* = acceptSimdFindAny2(src, pos.*, '"', ',');
                if (pos.* >= src.len) break;
                if (src[pos.*] == '"') {
                    in_quote = true;
                    pos.* += 1;
                } else {
                    const seg = trimAscii(src[start..pos.*]);
                    pos.* += 1;
                    if (seg.len > 0) return seg;
                    break;
                }
            }
        }
        if (pos.* >= src.len) {
            const seg = trimAscii(src[start..src.len]);
            if (seg.len > 0) return seg;
            return null;
        }
    }
    return null;
}

/// Extract the q-value (0..=1000) from semicolon-delimited parameters (default 1000).
/// Returns null if an explicit `q=...` parameter is malformed.
fn parseQFromParams(params: []const u8): u16? {
    let i: usize = 0;
    while (i < params.len) {
        while (i < params.len && (params[i] == ';' || isAsciiSpace(params[i]))) {
            i += 1;
        }
        if (i >= params.len) break;
        const start = i;
        let in_quote = false;
        let escaped = false;
        while (i < params.len) {
            if (in_quote) {
                if (escaped) {
                    escaped = false;
                    i += 1;
                    continue;
                }
                i = acceptSimdFindAny2(params, i, '"', '\\');
                if (i >= params.len) break;
                if (params[i] == '\\') {
                    escaped = true;
                } else {
                    in_quote = false;
                }
                i += 1;
            } else {
                i = acceptSimdFindAny2(params, i, '"', ';');
                if (i >= params.len) break;
                if (params[i] == '"') {
                    in_quote = true;
                    i += 1;
                } else {
                    break;
                }
            }
        }
        const kv = trimAscii(params[start..i]);
        if (eq_pos := indexOfByte(kv, '=')) {
            const k = trimAscii(kv[0..eq_pos]);
            if (k.len == 1 && (k[0] == 'q' || k[0] == 'Q')) {
                const v = trimAscii(kv[eq_pos + 1..]);
                return parseQValue(v);
            }
        }
    }
    return 1000;
}

export struct MediaRange {
    type_name: []const u8,
    subtype: []const u8,
    q_millis: u16,

    /// Specificity rank: 3 = exact type/subtype, 2 = type/*, 1 = */*, 0 = invalid.
    pub fn specificity(): u8 {
        if (_.type_name.len == 0 || _.subtype.len == 0) return 0;
        const t_wild = _.type_name.len == 1 && _.type_name[0] == '*';
        const s_wild = _.subtype.len == 1 && _.subtype[0] == '*';
        if (t_wild && s_wild) return 1;
        if (!t_wild && s_wild) return 2;
        if (!t_wild && !s_wild) return 3;
        return 0;
    }

    /// Check if this media range matches an offered media type like "application/json; charset=utf-8".
    pub fn matches(offered: []const u8): bool {
        let base = offered;
        if (semi := indexOfByte(offered, ';')) {
            base = offered[0..semi];
        }
        const trimmed = trimAscii(base);
        const slash = indexOfByte(trimmed, '/') ?? return false;
        const ot = trimAscii(trimmed[0..slash]);
        const os = trimAscii(trimmed[slash + 1..]);
        if (ot.len == 0 || os.len == 0) return false;
        const t_wild = _.type_name.len == 1 && _.type_name[0] == '*';
        const s_wild = _.subtype.len == 1 && _.subtype[0] == '*';
        if (t_wild && s_wild) return true;
        if (!t_wild && !eqIgnoreCase(_.type_name, ot)) return false;
        if (s_wild) return true;
        return eqIgnoreCase(_.subtype, os);
    }
}

export struct AcceptIter {
    src: []const u8,
    pos: usize,

    pub static fn init(header: []const u8): AcceptIter {
        return AcceptIter { src: header, pos: 0 };
    }

    pub fn next(*_): MediaRange? {
        while (item := nextCommaItem(_.src, &_.pos)) {
            let main_part = item;
            let params: []const u8 = "";
            if (semi := indexOfByte(item, ';')) {
                main_part = trimAscii(item[0..semi]);
                params = item[semi + 1..];
            }
            const slash = indexOfByte(main_part, '/') ?? continue;
            const tn = trimAscii(main_part[0..slash]);
            const st = trimAscii(main_part[slash + 1..]);
            if (tn.len == 0 || st.len == 0) continue;
            if (tn.len == 1 && tn[0] == '*' && !(st.len == 1 && st[0] == '*')) continue;
            const q = parseQFromParams(params) ?? continue;
            return MediaRange {
                type_name: tn,
                subtype: st,
                q_millis: q,
            };
        }
        return null;
    }
}

export struct EncodingPref {
    coding: []const u8,
    q_millis: u16,
}

export struct AcceptEncodingIter {
    src: []const u8,
    pos: usize,

    pub static fn init(header: []const u8): AcceptEncodingIter {
        return AcceptEncodingIter { src: header, pos: 0 };
    }

    pub fn next(*_): EncodingPref? {
        while (item := nextCommaItem(_.src, &_.pos)) {
            let coding = item;
            let params: []const u8 = "";
            if (semi := indexOfByte(item, ';')) {
                coding = trimAscii(item[0..semi]);
                params = item[semi + 1..];
            }
            if (coding.len == 0) continue;
            const q = parseQFromParams(params) ?? continue;
            return EncodingPref { coding: coding, q_millis: q };
        }
        return null;
    }
}

export struct LangPref {
    tag: []const u8,
    q_millis: u16,
}

export struct AcceptLanguageIter {
    src: []const u8,
    pos: usize,

    pub static fn init(header: []const u8): AcceptLanguageIter {
        return AcceptLanguageIter { src: header, pos: 0 };
    }

    pub fn next(*_): LangPref? {
        while (item := nextCommaItem(_.src, &_.pos)) {
            let tag = item;
            let params: []const u8 = "";
            if (semi := indexOfByte(item, ';')) {
                tag = trimAscii(item[0..semi]);
                params = item[semi + 1..];
            }
            if (tag.len == 0) continue;
            const q = parseQFromParams(params) ?? continue;
            return LangPref { tag: tag, q_millis: q };
        }
        return null;
    }
}

/// Select the best offered media type index for an `Accept` header (RFC 9110 §12.5.1).
/// Empty `accept_header` matches any offered type (returns 0). Returns null if none acceptable.
export fn negotiateMediaType(accept_header: []const u8, offered: []const []const u8): usize? {
    if (offered.len == 0) return null;
    const trimmed = trimAscii(accept_header);
    if (trimmed.len == 0) return 0;

    let best_idx: usize? = null;
    let best_q: u16 = 0;
    let best_spec: u8 = 0;

    let oi: usize = 0;
    while (oi < offered.len) {
        let matched = false;
        let eff_spec: u8 = 0;
        let eff_q: u16 = 0;

        let it = AcceptIter.init(trimmed);
        while (mr := it.next()) {
            if (!mr.matches(offered[oi])) continue;
            const sp = mr.specificity();
            if (!matched || sp > eff_spec || (sp == eff_spec && mr.q_millis > eff_q)) {
                matched = true;
                eff_spec = sp;
                eff_q = mr.q_millis;
            }
        }

        if (matched && eff_q > 0) {
            if (best_idx == null || eff_q > best_q || (eff_q == best_q && eff_spec > best_spec)) {
                best_idx = oi;
                best_q = eff_q;
                best_spec = eff_spec;
            }
        }
        oi += 1;
    }

    return best_idx;
}

/// Select the best offered content-coding index for an `Accept-Encoding` header (RFC 9110 §12.5.3).
/// Exact coding match overrides `*`; `q=0` explicitly rejects a coding.
export fn negotiateEncoding(accept_encoding: []const u8, offered: []const []const u8): usize? {
    if (offered.len == 0) return null;
    const trimmed = trimAscii(accept_encoding);
    if (trimmed.len == 0) return 0;

    let best_idx: usize? = null;
    let best_q: u16 = 0;
    let best_exact = false;

    let oi: usize = 0;
    while (oi < offered.len) {
        let exact_found = false;
        let exact_q: u16 = 0;
        let wild_found = false;
        let wild_q: u16 = 0;

        let it = AcceptEncodingIter.init(trimmed);
        while (pref := it.next()) {
            if (pref.coding.len == 1 && pref.coding[0] == '*') {
                if (!wild_found || pref.q_millis > wild_q) {
                    wild_found = true;
                    wild_q = pref.q_millis;
                }
            } else if (eqIgnoreCase(pref.coding, offered[oi])) {
                if (!exact_found || pref.q_millis > exact_q) {
                    exact_found = true;
                    exact_q = pref.q_millis;
                }
            }
        }

        let eff_q: u16 = 0;
        let is_exact = false;
        if (exact_found) {
            eff_q = exact_q;
            is_exact = true;
        } else if (wild_found) {
            eff_q = wild_q;
            is_exact = false;
        }

        if (eff_q > 0) {
            if (best_idx == null || eff_q > best_q || (eff_q == best_q && is_exact && !best_exact)) {
                best_idx = oi;
                best_q = eff_q;
                best_exact = is_exact;
            }
        }
        oi += 1;
    }

    return best_idx;
}

fn langPrefixMatches(range_tag: []const u8, offered_tag: []const u8): bool {
    if (range_tag.len >= offered_tag.len) return false;
    if (offered_tag[range_tag.len] != '-') return false;
    return eqIgnoreCase(range_tag, offered_tag[0..range_tag.len]);
}

/// Select the best offered language tag index for an `Accept-Language` header (RFC 9110 §12.5.4).
/// Match rank: exact tag (3) > subtag prefix match (2, e.g. "en" -> "en-US") > "*" (1).
export fn negotiateLanguage(accept_language: []const u8, offered: []const []const u8): usize? {
    if (offered.len == 0) return null;
    const trimmed = trimAscii(accept_language);
    if (trimmed.len == 0) return 0;

    let best_idx: usize? = null;
    let best_q: u16 = 0;
    let best_rank: u8 = 0;

    let oi: usize = 0;
    while (oi < offered.len) {
        let matched = false;
        let eff_rank: u8 = 0;
        let eff_q: u16 = 0;

        let it = AcceptLanguageIter.init(trimmed);
        while (pref := it.next()) {
            let rank: u8 = 0;
            if (eqIgnoreCase(pref.tag, offered[oi])) {
                rank = 3;
            } else if (langPrefixMatches(pref.tag, offered[oi])) {
                rank = 2;
            } else if (pref.tag.len == 1 && pref.tag[0] == '*') {
                rank = 1;
            } else {
                continue;
            }
            if (!matched || rank > eff_rank || (rank == eff_rank && pref.q_millis > eff_q)) {
                matched = true;
                eff_rank = rank;
                eff_q = pref.q_millis;
            }
        }

        if (matched && eff_q > 0) {
            if (best_idx == null || eff_q > best_q || (eff_q == best_q && eff_rank > best_rank)) {
                best_idx = oi;
                best_q = eff_q;
                best_rank = eff_rank;
            }
        }
        oi += 1;
    }

    return best_idx;
}
