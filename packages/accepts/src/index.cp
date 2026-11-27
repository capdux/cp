// @cap/accepts — Zero-allocation RFC 9110 HTTP content negotiation engine for CP
// Clean-room MIT implementation composing `@cap/negotiator` directly (parity with npm `accepts`).

import { eqIgnoreCase, findByte, isSpace } from "cap:ascii";
import {
    NegSpec,
    SPEC_WILDCARD,
    SPEC_PARTIAL,
    SPEC_EXACT,
    bestEncoding,
    bestMediaType,
    bestLanguage,
    parseAcceptList,
} from "../../negotiator/src/core.cp";

fn acceptsTrimSlice(s: []const u8) -> []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isSpace(s[end - (1 as usize)])) {
        end -= 1;
    }
    return s[start..end];
}

// Parse RFC 9110 q-value into integer milli-units 0..1000 (-1 on invalid)
pub fn parseQValue(s_in: []const u8) -> i32 {
    const s = acceptsTrimSlice(s_in);
    if (s.len == (0 as usize) || s.len > (5 as usize)) {
        return -1 as i32;
    }
    const first = s[0];
    if (first != '0' && first != '1') {
        return -1 as i32;
    }
    if (s.len == (1 as usize)) {
        if (first == '1') {
            return 1000 as i32;
        }
        return 0 as i32;
    }
    if (s[1] != '.') {
        return -1 as i32;
    }
    if (first == '1') {
        let i: usize = 2;
        while (i < s.len) {
            if (s[i] != '0') {
                return -1 as i32;
            }
            i += 1;
        }
        return 1000 as i32;
    }

    // first == '0', s[1] == '.'
    let val: u32 = 0;
    let weight: u32 = 100;
    let i: usize = 2;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return -1 as i32;
        }
        val += ((c - '0') as u32) * weight;
        weight = weight / (10 as u32);
        i += 1;
    }
    return val as i32;
}

fn accIsBetter(q: u16, spec: u8, order: usize, bq: u16, bspec: u8, border: usize): bool {
    if (q != bq) { return q > bq; }
    if (spec != bspec) { return spec > bspec; }
    return order < border;
}

/// Negotiate best `Accept-Encoding` candidate via `@cap/negotiator`.
pub fn negotiateEncoding(accept_header: []const u8, available: []const []const u8) -> i64 {
    return bestEncoding(accept_header, available);
}

/// Negotiate best `Accept-Language` candidate via `@cap/negotiator`.
pub fn negotiateLanguage(accept_header: []const u8, available: []const []const u8) -> i64 {
    return bestLanguage(accept_header, available);
}

/// Negotiate best `Accept` media type candidate via `@cap/negotiator`.
pub fn negotiateMediaType(accept_header: []const u8, available: []const []const u8) -> i64 {
    return bestMediaType(accept_header, available);
}

/// Negotiate best `Accept-Charset` candidate (RFC 9110: exact match > `*` wildcard, `q=0` rejected).
/// Returns index in `available`, or `-1` if no candidate is acceptable.
pub fn negotiateCharset(accept_charset: []const u8, available: []const []const u8) -> i64 {
    if (available.len == 0) { return -1 as i64; }
    let specs: [32]NegSpec = undefined;
    const n: usize = parseAcceptList(accept_charset, specs[0..32]);
    if (n == 0) {
        return 0 as i64;
    }
    let best_idx: i64 = -1;
    let best_q: u16 = 0;
    let best_spec: u8 = 0;
    let best_order: usize = 0;
    let i: usize = 0;
    while (i < available.len) : (i += 1) {
        const cand = available[i];
        let matched: bool = false;
        let m_spec: u8 = 0;
        let m_q: u16 = 0;
        let m_order: usize = 0;
        let j: usize = 0;
        while (j < n) : (j += 1) {
            const s = specs[j];
            const tok = accept_charset[s.start .. s.start + s.len];
            let cur_spec: u8 = 0;
            if (eqIgnoreCase(tok, cand)) {
                cur_spec = SPEC_EXACT;
            } else if (tok.len == 1 && tok[0] == '*') {
                cur_spec = SPEC_WILDCARD;
            }
            if (cur_spec > 0 && (!matched || cur_spec > m_spec || (cur_spec == m_spec && s.q_milli > m_q))) {
                matched = true;
                m_spec = cur_spec;
                m_q = s.q_milli;
                m_order = s.order;
            }
        }
        if (matched && m_q > 0 && (best_idx < 0 || accIsBetter(m_q, m_spec, m_order, best_q, best_spec, best_order))) {
            best_idx = i as i64;
            best_q = m_q;
            best_spec = m_spec;
            best_order = m_order;
        }
    }
    return best_idx;
}

/// Rank all acceptable media type candidates from `available` in descending preference order
/// (by q-value, specificity, then header order) into `out_indices`. Returns count written.
pub fn rankMediaTypes(accept_header: []const u8, available: []const []const u8, out_indices: []usize) -> usize {
    if (available.len == 0 || out_indices.len == 0) {
        return 0;
    }
    let specs: [32]NegSpec = undefined;
    const n: usize = parseAcceptList(accept_header, specs[0..32]);
    if (n == 0) {
        const lim: usize = if (available.len < out_indices.len) available.len else out_indices.len;
        let k: usize = 0;
        while (k < lim) : (k += 1) {
            out_indices[k] = k;
        }
        return lim;
    }

    const max_cands: usize = 32;
    let cand_idx: [32]usize = undefined;
    let cand_q: [32]u16 = undefined;
    let cand_spec: [32]u8 = undefined;
    let cand_order: [32]usize = undefined;
    let count: usize = 0;

    let i: usize = 0;
    while (i < available.len && count < max_cands) : (i += 1) {
        const cand = available[i];
        const c_slash_opt = findByte(cand, 47, 0);
        const c_type = if (c_slash_opt != null) cand[0..(c_slash_opt ?? 0)] else cand;
        const c_sub = if (c_slash_opt != null) cand[((c_slash_opt ?? 0) + 1)..cand.len] else "";
        let matched: bool = false;
        let m_spec: u8 = 0;
        let m_q: u16 = 0;
        let m_order: usize = 0;
        let j: usize = 0;
        while (j < n) : (j += 1) {
            const s = specs[j];
            const tok = accept_header[s.start .. s.start + s.len];
            let cur_spec: u8 = 0;
            if ((tok.len == 1 && tok[0] == '*') || eqIgnoreCase(tok, "*/*")) {
                cur_spec = SPEC_WILDCARD;
            } else {
                const s_slash_opt = findByte(tok, 47, 0);
                if (s_slash_opt != null) {
                    const s_slash = s_slash_opt ?? 0;
                    const s_type = tok[0..s_slash];
                    const s_sub = tok[(s_slash + 1)..tok.len];
                    if (eqIgnoreCase(s_type, c_type)) {
                        if (s_sub.len == 1 && s_sub[0] == '*') {
                            cur_spec = SPEC_PARTIAL;
                        } else if (eqIgnoreCase(s_sub, c_sub)) {
                            cur_spec = SPEC_EXACT;
                        }
                    }
                }
            }
            if (cur_spec > 0 && (!matched || cur_spec > m_spec || (cur_spec == m_spec && s.q_milli > m_q))) {
                matched = true;
                m_spec = cur_spec;
                m_q = s.q_milli;
                m_order = s.order;
            }
        }
        if (matched && m_q > 0) {
            cand_idx[count] = i;
            cand_q[count] = m_q;
            cand_spec[count] = m_spec;
            cand_order[count] = m_order;
            count += 1;
        }
    }

    // Insertion sort descending by (q, specificity, -order)
    let a: usize = 1;
    while (a < count) : (a += 1) {
        const ki = cand_idx[a];
        const kq = cand_q[a];
        const ks = cand_spec[a];
        const ko = cand_order[a];
        let b: usize = a;
        while (b > 0 && accIsBetter(kq, ks, ko, cand_q[b - 1], cand_spec[b - 1], cand_order[b - 1])) {
            cand_idx[b] = cand_idx[b - 1];
            cand_q[b] = cand_q[b - 1];
            cand_spec[b] = cand_spec[b - 1];
            cand_order[b] = cand_order[b - 1];
            b -= 1;
        }
        cand_idx[b] = ki;
        cand_q[b] = kq;
        cand_spec[b] = ks;
        cand_order[b] = ko;
    }

    const written: usize = if (count < out_indices.len) count else out_indices.len;
    let w: usize = 0;
    while (w < written) : (w += 1) {
        out_indices[w] = cand_idx[w];
    }
    return written;
}

/// Request Accept-* header bundle for constructing an `Accepts` negotiator.
pub struct AcceptHeaders {
    pub accept: []const u8,
    pub accept_encoding: []const u8,
    pub accept_charset: []const u8,
    pub accept_language: []const u8,

    pub static fn empty(): AcceptHeaders {
        return AcceptHeaders {
            accept: "",
            accept_encoding: "",
            accept_charset: "",
            accept_language: "",
        };
    }
}

/// Node `accepts(req)` negotiator object supporting `types`, `encodings`, `charsets`, and `languages`.
pub struct Accepts {
    pub headers: AcceptHeaders,

    pub static fn new(headers: AcceptHeaders): Accepts {
        return Accepts { headers: headers };
    }

    pub fn typeIndex(available: []const []const u8): i64 {
        return negotiateMediaType(_.headers.accept, available);
    }

    pub fn types(available: []const []const u8): ([]const u8)? {
        const idx = negotiateMediaType(_.headers.accept, available);
        if (idx < 0) { return null; }
        return available[idx as usize];
    }

    pub fn encodingIndex(available: []const []const u8): i64 {
        return negotiateEncoding(_.headers.accept_encoding, available);
    }

    pub fn encodings(available: []const []const u8): ([]const u8)? {
        const idx = negotiateEncoding(_.headers.accept_encoding, available);
        if (idx < 0) { return null; }
        return available[idx as usize];
    }

    pub fn charsetIndex(available: []const []const u8): i64 {
        return negotiateCharset(_.headers.accept_charset, available);
    }

    pub fn charsets(available: []const []const u8): ([]const u8)? {
        const idx = negotiateCharset(_.headers.accept_charset, available);
        if (idx < 0) { return null; }
        return available[idx as usize];
    }

    pub fn languageIndex(available: []const []const u8): i64 {
        return negotiateLanguage(_.headers.accept_language, available);
    }

    pub fn languages(available: []const []const u8): ([]const u8)? {
        const idx = negotiateLanguage(_.headers.accept_language, available);
        if (idx < 0) { return null; }
        return available[idx as usize];
    }
}

export {
    AcceptHeaders,
    Accepts,
    parseQValue,
    parseAcceptList,
    negotiateEncoding,
    negotiateLanguage,
    negotiateMediaType,
    negotiateCharset,
    rankMediaTypes,
};
