// @cap/negotiator — Zero-heap RFC 7231 HTTP Content Negotiation engine.
// Clean-room MIT port of npm `negotiator` v0.6; reuses `cap:ascii` & `unicase`.

import { trim, eqIgnoreCase, findByte } from "cap:ascii";
import { eqAscii, startsWithIgnoreCase } from "../../unicase/src/index.cp";
import {
    NEG_MAX_SPECS,
    SPEC_WILDCARD,
    SPEC_PARTIAL,
    SPEC_EXACT,
    NegSpec,
    parseAcceptList,
    bestEncoding,
    bestMediaType,
    bestLanguage,
} from "./core.cp";

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

export {
    NEG_MAX_SPECS,
    SPEC_WILDCARD,
    SPEC_PARTIAL,
    SPEC_EXACT,
    NegSpec,
    QualitySpec,
    parseQValue,
    parseWeightedList,
    negotiateEncoding,
    negotiateLanguage,
    negotiateCharset,
    parseAcceptList,
    bestEncoding,
    bestMediaType,
    bestLanguage,
};
