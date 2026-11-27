// @cap/negotiator — Zero-heap RFC 7231 HTTP Content Negotiation engine.
// Clean-room MIT port of npm `negotiator` v0.6; reuses `cap:ascii` & `unicase`.

import { trim, eqIgnoreCase, findByte } from "cap:ascii";
import { eqAscii, startsWithIgnoreCase } from "../../unicase/src/index.cp";

pub const NEG_MAX_SPECS: usize = 32;
pub const SPEC_WILDCARD: u8 = 1;
pub const SPEC_PARTIAL: u8 = 2;
pub const SPEC_EXACT: u8 = 4;

pub struct NegSpec {
    pub start: usize,
    pub len: usize,
    pub q_milli: u16,
    pub specificity: u8,
    pub order: usize,
}

pub struct QualitySpec {
    pub value: string,
    pub q_milli: u32, // 0..1000 (1.000 = 1000, 0.8 = 800, 0 = excluded)
    pub order: usize,
}

fn negIsWs(b: u8): bool {
    return b == 32 || b == 9;
}

fn negTrim(s: string): string {
    let start: usize = 0;
    while (start < s.len && negIsWs(s[start])) { start = start + 1; }
    let end: usize = s.len;
    while (end > start && negIsWs(s[end - 1])) { end = end - 1; }
    return s[start..end];
}

/// Parses an RFC 7231 `qvalue` (`0`, `1`, `0.5`, `0.125`, `1.000`) into milli-units `0..1000`.
pub fn parseQValue(raw: string): u32 {
    let s = negTrim(raw);
    if (s.len == 0) { return 1000; }
    if (s[0] == 49) { // '1'
        return 1000;
    }
    if (s[0] != 48) { return 0; } // must start with '0' or '1'
    if (s.len == 1) { return 0; }
    if (s[1] != 46) { return 0; } // '.'
    let val: u32 = 0;
    let scale: u32 = 100;
    let i: usize = 2;
    while (i < s.len && scale > 0) {
        let b = s[i];
        if (b < 48 || b > 57) { break; }
        val = val + ((b - 48) as u32) * scale;
        scale = scale / 10;
        i = i + 1;
    }
    return val;
}

/// Parses a comma-separated RFC 7231 weighted list (`"br;q=1.0, gzip;q=0.8, *;q=0.1"`)
/// into `out`, sorted by descending `q_milli` (preserving original order on ties).
/// Returns number of specs written.
pub fn parseWeightedList(header: string, out: []QualitySpec): usize {
    let count: usize = 0;
    let pos: usize = 0;
    let idx: usize = 0;
    while (pos <= header.len && count < out.len) {
        let sep = pos;
        while (sep < header.len && header[sep] != 44) { sep = sep + 1; }
        let item = negTrim(header[pos..sep]);
        if (item.len > 0) {
            let semi: usize = 0;
            while (semi < item.len && item[semi] != 59) { semi = semi + 1; }
            let val = negTrim(item[0..semi]);
            let q: u32 = 1000;
            if (semi < item.len) {
                let ppos = semi + 1;
                while (ppos <= item.len) {
                    let psep = ppos;
                    while (psep < item.len && item[psep] != 59) { psep = psep + 1; }
                    let param = negTrim(item[ppos..psep]);
                    if (param.len >= 2 && (param[0] == 113 || param[0] == 81)) { // 'q'/'Q'
                        let eq: usize = 1;
                        while (eq < param.len && negIsWs(param[eq])) { eq = eq + 1; }
                        if (eq < param.len && param[eq] == 61) {
                            q = parseQValue(param[eq + 1..param.len]);
                        }
                    }
                    if (psep >= item.len) { break; }
                    ppos = psep + 1;
                }
            }
            if (val.len > 0) {
                out[count] = QualitySpec { value: val, q_milli: q, order: idx };
                count = count + 1;
                idx = idx + 1;
            }
        }
        if (sep >= header.len) { break; }
        pos = sep + 1;
    }

    // Stable insertion sort by descending q_milli
    let i: usize = 1;
    while (i < count) {
        let cur = out[i];
        let j = i;
        while (j > 0 && out[j - 1].q_milli < cur.q_milli) {
            out[j] = out[j - 1];
            j = j - 1;
        }
        out[j] = cur;
        i = i + 1;
    }
    return count;
}

fn negTokenSpecificity(spec: string, candidate: string, is_lang: bool): u32 {
    if (eqAscii(spec, candidate)) { return 4; }
    if (is_lang) {
        // Language prefix match: "en" matches "en-US" (spec shorter) or "en-US" matches "en"
        if (spec.len < candidate.len && startsWithIgnoreCase(candidate, spec) && candidate[spec.len] == 45) {
            return 2;
        }
        if (candidate.len < spec.len && startsWithIgnoreCase(spec, candidate) && spec[candidate.len] == 45) {
            return 2;
        }
    }
    if (spec.len == 1 && spec[0] == 42) { return 1; } // '*'
    return 0;
}

fn negNegotiateToken(header: string, available: []const string, is_lang: bool): string? {
    if (available.len == 0) { return null; }
    if (negTrim(header).len == 0) { return available[0]; }
    let specs: [32]QualitySpec = undefined;
    let scount = parseWeightedList(header, specs[0..32]);
    if (scount == 0) { return available[0]; }

    let best_idx: usize = available.len;
    let best_q: u32 = 0;
    let best_spec: u32 = 0;

    let ci: usize = 0;
    while (ci < available.len) {
        let cand = available[ci];
        let cand_q: u32 = 0;
        let cand_spec: u32 = 0;
        let si: usize = 0;
        while (si < scount) {
            let sp = negTokenSpecificity(specs[si].value, cand, is_lang);
            if (sp > cand_spec) {
                cand_spec = sp;
                cand_q = specs[si].q_milli;
            }
            si = si + 1;
        }
        if (cand_spec > 0 && cand_q > 0) {
            if (cand_q > best_q || (cand_q == best_q && cand_spec > best_spec)) {
                best_q = cand_q;
                best_spec = cand_spec;
                best_idx = ci;
            }
        }
        ci = ci + 1;
    }

    if (best_idx < available.len) { return available[best_idx]; }
    return null;
}

/// Negotiates the best `Accept-Encoding` (e.g., `["br", "gzip", "identity"]`).
pub fn negotiateEncoding(accept_encoding: string, available: []const string): string? {
    return negNegotiateToken(accept_encoding, available, false);
}

/// Negotiates the best `Accept-Language` (e.g., `["en-US", "zh-CN", "fr"]`).
pub fn negotiateLanguage(accept_language: string, available: []const string): string? {
    return negNegotiateToken(accept_language, available, true);
}

/// Negotiates the best `Accept-Charset` (e.g., `["utf-8", "iso-8859-1"]`).
pub fn negotiateCharset(accept_charset: string, available: []const string): string? {
    return negNegotiateToken(accept_charset, available, false);
}

fn negTrimStart(s: string, lo: usize, hi: usize): usize {
    let i: usize = lo;
    while (i < hi && (s[i] == 32 || s[i] == 9 || s[i] == 13 || s[i] == 10)) { i += 1; }
    return i;
}

fn negTrimEnd(s: string, lo: usize, hi: usize): usize {
    let j: usize = hi;
    while (j > lo && (s[j - 1] == 32 || s[j - 1] == 9 || s[j - 1] == 13 || s[j - 1] == 10)) { j -= 1; }
    return j;
}

fn negParseQMilli(val: string): u16 {
    if (val.len == 0) { return 1000; }
    let i: usize = 0;
    let whole: u32 = 0;
    let saw_digit: bool = false;
    while (i < val.len && val[i] >= 48 && val[i] <= 57) {
        whole = whole * 10 + ((val[i] - 48) as u32);
        saw_digit = true;
        i += 1;
    }
    let frac: u32 = 0;
    let frac_scale: u32 = 100;
    if (i < val.len && val[i] == 46) {
        i += 1;
        while (i < val.len && val[i] >= 48 && val[i] <= 57) {
            saw_digit = true;
            if (frac_scale > 0) {
                frac = frac + ((val[i] - 48) as u32) * frac_scale;
                frac_scale = frac_scale / 10;
            }
            i += 1;
        }
    }
    if (!saw_digit || i < val.len) { return 0; }
    const total: u32 = whole * 1000 + frac;
    if (total > 1000) { return 1000; }
    return total as u16;
}

fn negClassifyToken(tok: string): u8 {
    if (tok.len == 1 && tok[0] == 42) { return SPEC_WILDCARD; }
    if (tok.len == 3 && tok[0] == 42 && tok[1] == 47 && tok[2] == 42) { return SPEC_WILDCARD; }
    if (tok.len >= 2 && tok[tok.len - 2] == 47 && tok[tok.len - 1] == 42) { return SPEC_PARTIAL; }
    return SPEC_EXACT;
}

/// Parse a comma-separated RFC 7231 Accept* header into `out` without heap allocation.
pub fn parseAcceptList(header: string, out: []NegSpec): usize {
    if (header.len == 0 || out.len == 0) { return 0; }
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < header.len && count < out.len) {
        let item_end: usize = pos;
        while (item_end < header.len && header[item_end] != 44) { item_end += 1; }
        const seg_lo: usize = negTrimStart(header, pos, item_end);
        const seg_hi: usize = negTrimEnd(header, seg_lo, item_end);
        if (seg_lo < seg_hi) {
            let semi: usize = seg_lo;
            while (semi < seg_hi && header[semi] != 59) { semi += 1; }
            const tok_lo: usize = negTrimStart(header, seg_lo, semi);
            const tok_hi: usize = negTrimEnd(header, tok_lo, semi);
            if (tok_lo < tok_hi) {
                let q_milli: u16 = 1000;
                let p_cur: usize = semi;
                while (p_cur < seg_hi) {
                    p_cur += 1;
                    let p_end: usize = p_cur;
                    while (p_end < seg_hi && header[p_end] != 59) { p_end += 1; }
                    const p_lo: usize = negTrimStart(header, p_cur, p_end);
                    const p_hi: usize = negTrimEnd(header, p_lo, p_end);
                    let eq_pos: usize = p_lo;
                    while (eq_pos < p_hi && header[eq_pos] != 61) { eq_pos += 1; }
                    if (eq_pos < p_hi) {
                        const k_lo: usize = negTrimStart(header, p_lo, eq_pos);
                        const k_hi: usize = negTrimEnd(header, k_lo, eq_pos);
                        if (k_hi - k_lo == 1 && (header[k_lo] == 113 || header[k_lo] == 81)) {
                            const v_lo: usize = negTrimStart(header, eq_pos + 1, p_hi);
                            const v_hi: usize = negTrimEnd(header, v_lo, p_hi);
                            q_milli = negParseQMilli(header[v_lo..v_hi]);
                        }
                    }
                    p_cur = p_end;
                }
                const tok_str: string = header[tok_lo..tok_hi];
                out[count] = NegSpec {
                    start: tok_lo,
                    len: tok_hi - tok_lo,
                    q_milli: q_milli,
                    specificity: negClassifyToken(tok_str),
                    order: count,
                };
                count += 1;
            }
        }
        pos = if (item_end < header.len) item_end + 1 else header.len;
    }
    return count;
}

fn negIsBetterCandidate(q: u16, spec: u8, order: usize, best_q: u16, best_spec: u8, best_order: usize, has_best: bool): bool {
    if (!has_best) { return true; }
    if (q != best_q) { return q > best_q; }
    if (spec != best_spec) { return spec > best_spec; }
    return order < best_order;
}

pub fn bestEncoding(header: string, available: []const string): i64 {
    if (available.len == 0) { return -1; }
    let specs: [32]NegSpec = undefined;
    const n: usize = parseAcceptList(header, specs[0..32]);
    if (n == 0) {
        let i: usize = 0;
        while (i < available.len) {
            if (eqIgnoreCase(available[i], "identity")) { return i as i64; }
            i += 1;
        }
        return 0;
    }
    let best_idx: i64 = -1;
    let best_q: u16 = 0;
    let best_spec: u8 = 0;
    let best_order: usize = 0;
    let i: usize = 0;
    while (i < available.len) {
        const cand: string = available[i];
        let matched: bool = false;
        let m_spec: u8 = 0;
        let m_q: u16 = 0;
        let m_order: usize = 0;
        let j: usize = 0;
        while (j < n) {
            const s: NegSpec = specs[j];
            const tok: string = header[s.start .. s.start + s.len];
            let cur_spec: u8 = 0;
            if (eqIgnoreCase(tok, cand)) { cur_spec = SPEC_EXACT; }
            else if (tok.len == 1 && tok[0] == 42) { cur_spec = SPEC_WILDCARD; }
            if (cur_spec > 0 && (!matched || cur_spec > m_spec || (cur_spec == m_spec && s.q_milli > m_q))) {
                matched = true; m_spec = cur_spec; m_q = s.q_milli; m_order = s.order;
            }
            j += 1;
        }
        if (matched && m_q > 0 && negIsBetterCandidate(m_q, m_spec, m_order, best_q, best_spec, best_order, best_idx >= 0)) {
            best_idx = i as i64; best_q = m_q; best_spec = m_spec; best_order = m_order;
        }
        i += 1;
    }
    return best_idx;
}

pub fn bestMediaType(header: string, available: []const string): i64 {
    if (available.len == 0) { return -1; }
    let specs: [32]NegSpec = undefined;
    const n: usize = parseAcceptList(header, specs[0..32]);
    if (n == 0) { return 0; }
    let best_idx: i64 = -1;
    let best_q: u16 = 0;
    let best_spec: u8 = 0;
    let best_order: usize = 0;
    let i: usize = 0;
    while (i < available.len) {
        const cand: string = available[i];
        const c_slash: i64 = findByte(cand, 47);
        const c_type: string = if (c_slash >= 0) cand[0..(c_slash as usize)] else cand;
        const c_sub: string = if (c_slash >= 0) cand[((c_slash as usize) + 1)..cand.len] else "";
        let matched: bool = false;
        let m_spec: u8 = 0;
        let m_q: u16 = 0;
        let m_order: usize = 0;
        let j: usize = 0;
        while (j < n) {
            const s: NegSpec = specs[j];
            const tok: string = header[s.start .. s.start + s.len];
            let cur_spec: u8 = 0;
            if ((tok.len == 1 && tok[0] == 42) || eqIgnoreCase(tok, "*/*")) {
                cur_spec = SPEC_WILDCARD;
            } else {
                const s_slash: i64 = findByte(tok, 47);
                if (s_slash >= 0) {
                    const s_type: string = tok[0..(s_slash as usize)];
                    const s_sub: string = tok[((s_slash as usize) + 1)..tok.len];
                    if (eqIgnoreCase(s_type, c_type)) {
                        if (s_sub.len == 1 && s_sub[0] == 42) { cur_spec = SPEC_PARTIAL; }
                        else if (eqIgnoreCase(s_sub, c_sub)) { cur_spec = SPEC_EXACT; }
                    }
                }
            }
            if (cur_spec > 0 && (!matched || cur_spec > m_spec || (cur_spec == m_spec && s.q_milli > m_q))) {
                matched = true; m_spec = cur_spec; m_q = s.q_milli; m_order = s.order;
            }
            j += 1;
        }
        if (matched && m_q > 0 && negIsBetterCandidate(m_q, m_spec, m_order, best_q, best_spec, best_order, best_idx >= 0)) {
            best_idx = i as i64; best_q = m_q; best_spec = m_spec; best_order = m_order;
        }
        i += 1;
    }
    return best_idx;
}

fn negLangPrefixMatch(longer: string, shorter: string): bool {
    if (shorter.len == 0 || longer.len <= shorter.len) { return false; }
    if (longer[shorter.len] != 45) { return false; }
    return eqIgnoreCase(longer[0..shorter.len], shorter);
}

pub fn bestLanguage(header: string, available: []const string): i64 {
    if (available.len == 0) { return -1; }
    let specs: [32]NegSpec = undefined;
    const n: usize = parseAcceptList(header, specs[0..32]);
    if (n == 0) { return 0; }
    let best_idx: i64 = -1;
    let best_q: u16 = 0;
    let best_spec: u8 = 0;
    let best_order: usize = 0;
    let i: usize = 0;
    while (i < available.len) {
        const cand: string = available[i];
        let matched: bool = false;
        let m_spec: u8 = 0;
        let m_q: u16 = 0;
        let m_order: usize = 0;
        let j: usize = 0;
        while (j < n) {
            const s: NegSpec = specs[j];
            const tok: string = header[s.start .. s.start + s.len];
            let cur_spec: u8 = 0;
            if (eqIgnoreCase(tok, cand)) { cur_spec = SPEC_EXACT; }
            else if (negLangPrefixMatch(tok, cand) || negLangPrefixMatch(cand, tok)) { cur_spec = SPEC_PARTIAL; }
            else if (tok.len == 1 && tok[0] == 42) { cur_spec = SPEC_WILDCARD; }
            if (cur_spec > 0 && (!matched || cur_spec > m_spec || (cur_spec == m_spec && s.q_milli > m_q))) {
                matched = true; m_spec = cur_spec; m_q = s.q_milli; m_order = s.order;
            }
            j += 1;
        }
        if (matched && m_q > 0 && negIsBetterCandidate(m_q, m_spec, m_order, best_q, best_spec, best_order, best_idx >= 0)) {
            best_idx = i as i64; best_q = m_q; best_spec = m_spec; best_order = m_order;
        }
        i += 1;
    }
    return best_idx;
}
