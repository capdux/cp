// Origin: Pure-CP Stdlib (Zero-Alloc RFC 9110 HTTP Content Negotiation & Vary Header Engine) | Created: 2026-11-23
// Reference-hardened against kornelski/rusty-http-cache-semantics (BSD-2-Clause) & RFC 9111/9110
// Pure CP zero-heap-allocation RFC 9110 §12 HTTP Content Negotiation & Vary synthesis engine:
//   - Strict RFC 9110 §12.4.2 integer q-value parser (0..1000 milli-units, zero floating-point drift)
//   - Accept media-type negotiation (type/subtype; param=v; q=..., wildcard specificity, q=0 exclusion)
//   - Accept-Encoding negotiation (zstd/br/gzip/deflate/identity/*, identity;q=0 & *;q=0 406 handling)
//   - Accept-Language BCP 47 negotiation (case-insensitive exact & subtag prefix fallback, q=0 exclusion)
//   - Zero-alloc Vary response header deduplication & RFC 9110 §12.5.5 '*' collapse with 16-lane SIMD acceleration

const std = @import("std");
import {
    simdWebSemAsciiLowerEql,
    simdWebSemFindDelim,
} from "./cachePolicy.cp";

// ============================================================================
// 1. Constants & ASCII / Header Safety Helpers
// ============================================================================

/// Maximum q-value weight in milli-units (1000 == 1.000).
pub const NEG_Q_MAX: u16 = 1000;

/// Maximum comma-separated header items parsed per negotiation call (anti-DoS bound).
pub const NEG_MAX_ITEMS: usize = 32;

/// Maximum distinct header names tracked by `formatVaryHeader`.
pub const NEG_VARY_MAX_HEADERS: usize = 16;

fn negEqIgnoreCase(a: []const u8, b: []const u8): bool {
    return simdWebSemAsciiLowerEql(a, b);
}

fn negTrimOws(s: []const u8): []const u8 {
    let lo: usize = 0;
    let hi: usize = s.len;
    while (lo < hi && (s[lo] == 0x20 || s[lo] == 0x09)) {
        lo += 1;
    }
    while (hi > lo && (s[hi - 1] == 0x20 || s[hi - 1] == 0x09)) {
        hi -= 1;
    }
    return s[lo..hi];
}

fn negHasUnsafeCtl(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c == 0x00 || c == 0x0A || c == 0x0D || (c < 0x20 && c != 0x09) || c == 0x7F) {
            return true;
        }
    }
    return false;
}

fn negIsTchar(c: u8): bool {
    if ((c >= 0x30 && c <= 0x39) || (c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A)) {
        return true;
    }
    if (c == 0x21 || c == 0x23 || c == 0x24 || c == 0x25 || c == 0x26 ||
        c == 0x27 || c == 0x2A || c == 0x2B || c == 0x2D || c == 0x2E ||
        c == 0x5E || c == 0x5F || c == 0x60 || c == 0x7C || c == 0x7E)
    {
        return true;
    }
    return false;
}

fn negIsValidToken(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!negIsTchar(s[i])) return false;
    }
    return true;
}

/// Parses an RFC 9110 §12.4.2 q-value into integer milli-units `0..=1000`.
/// Grammar: `( "0" [ "." 0*3DIGIT ] ) / ( "1" [ "." 0*3("0") ] )`.
pub fn parseQValue(raw: []const u8): u16? {
    const s = negTrimOws(raw);
    if (s.len == 0 || s.len > 5) return null;

    if (s[0] == 0x31) { // '1'
        if (s.len == 1) return NEG_Q_MAX;
        if (s[1] != 0x2E) return null; // '.'
        let i: usize = 2;
        while (i < s.len) : (i += 1) {
            if (s[i] != 0x30) return null; // only '0' allowed after "1."
        }
        return NEG_Q_MAX;
    }

    if (s[0] == 0x30) { // '0'
        if (s.len == 1) return 0;
        if (s[1] != 0x2E) return null; // '.'
        let val: u16 = 0;
        let mul: u16 = 100;
        let i: usize = 2;
        while (i < s.len) : (i += 1) {
            const c: u8 = s[i];
            if (c < 0x30 || c > 0x39) return null;
            val += ((c - 0x30) as u16) * mul;
            mul = mul / 10;
        }
        return val;
    }

    return null;
}

// ============================================================================
// 2. Public Enums & Result Models
// ============================================================================

/// Standard HTTP Content-Encoding tokens supported by `negotiateEncoding`.
pub enum ContentEncoding {
    identity,
    gzip,
    deflate,
    br,
    zstd,

    /// Returns the canonical lowercase HTTP Content-Encoding header value.
    fn asHeaderValue(): []const u8 {
        if (_ == ContentEncoding.identity) return "identity";
        if (_ == ContentEncoding.gzip) return "gzip";
        if (_ == ContentEncoding.deflate) return "deflate";
        if (_ == ContentEncoding.br) return "br";
        return "zstd";
    }

    /// Parses a case-insensitive Content-Encoding token (maps `x-gzip` to `gzip`).
    static fn fromToken(tok: []const u8): ContentEncoding? {
        const s = negTrimOws(tok);
        if (negEqIgnoreCase(s, "identity")) return ContentEncoding.identity;
        if (negEqIgnoreCase(s, "gzip") || negEqIgnoreCase(s, "x-gzip")) return ContentEncoding.gzip;
        if (negEqIgnoreCase(s, "deflate")) return ContentEncoding.deflate;
        if (negEqIgnoreCase(s, "br")) return ContentEncoding.br;
        if (negEqIgnoreCase(s, "zstd")) return ContentEncoding.zstd;
        return null;
    }
}

/// Outcome status of a content negotiation call.
pub enum NegotiationMatchKind {
    matched,
    not_acceptable,
    invalid_header,
}

/// Result returned by `negotiateMediaType`.
pub struct MediaMatchResult {
    status: NegotiationMatchKind = NegotiationMatchKind.not_acceptable;
    offered_index: usize = 0;
    q_value: u16 = 0;
    specificity: u8 = 0;
}

/// Result returned by `negotiateEncoding`.
pub struct EncodingMatchResult {
    status: NegotiationMatchKind = NegotiationMatchKind.not_acceptable;
    encoding: ContentEncoding = ContentEncoding.identity;
    offered_index: usize = 0;
    q_value: u16 = 0;
}

/// Result returned by `negotiateLanguage`.
pub struct LanguageMatchResult {
    status: NegotiationMatchKind = NegotiationMatchKind.not_acceptable;
    offered_index: usize = 0;
    q_value: u16 = 0;
    matched_prefix_len: usize = 0;
}

// ============================================================================
// 3. RFC 9110 §12.5.1 Accept Media Type Negotiation
// ============================================================================

struct MediaParam {
    key: []const u8 = "",
    val: []const u8 = "",
}

struct ParsedMediaRange {
    media_type: []const u8 = "",
    subtype: []const u8 = "",
    q_value: u16 = NEG_Q_MAX,
    params: [4]MediaParam = [MediaParam{}; 4],
    param_count: usize = 0,
    valid: bool = false,
}

/// Splits `s` at the next unquoted `delim` starting at `start` using 16-lane SIMD delimiter skipping.
fn findNextUnquoted(s: []const u8, start: usize, delim: u8): usize {
    let i: usize = start;
    let in_quotes: bool = false;
    while (i < s.len) {
        if (!in_quotes) {
            const d: usize = simdWebSemFindDelim(s, i);
            if (d >= s.len) return s.len;
            const c: u8 = s[d];
            if (c == delim) return d;
            if (c == 0x22) { // '"'
                in_quotes = true;
            }
            i = d + 1;
        } else {
            const c: u8 = s[i];
            if (c == 0x22) {
                in_quotes = false;
                i += 1;
            } else if (c == 0x5C && i + 1 < s.len) {
                i += 2;
            } else {
                i += 1;
            }
        }
    }
    return s.len;
}

fn unquoteParamVal(raw: []const u8): []const u8 {
    const s = negTrimOws(raw);
    if (s.len >= 2 && s[0] == 0x22 && s[s.len - 1] == 0x22) {
        return s[1 .. s.len - 1];
    }
    return s;
}

fn parseMediaRangeItem(raw_item: []const u8): ParsedMediaRange {
    const item = negTrimOws(raw_item);
    if (item.len == 0) return ParsedMediaRange{};

    const first_semi: usize = findNextUnquoted(item, 0, 0x3B); // ';'
    const type_part = negTrimOws(item[0..first_semi]);

    let slash_idx: usize = type_part.len;
    let i: usize = 0;
    while (i < type_part.len) : (i += 1) {
        if (type_part[i] == 0x2F) { // '/'
            slash_idx = i;
            break;
        }
    }
    if (slash_idx == 0 || slash_idx >= type_part.len - 1) return ParsedMediaRange{};

    const mtype = negTrimOws(type_part[0..slash_idx]);
    const subtype = negTrimOws(type_part[slash_idx + 1 ..]);
    if (!negIsValidToken(mtype) || !negIsValidToken(subtype)) return ParsedMediaRange{};
    // Wildcard type with concrete subtype (`*/html`) is invalid per RFC 9110.
    if (negEqIgnoreCase(mtype, "*") && !negEqIgnoreCase(subtype, "*")) return ParsedMediaRange{};

    let res = ParsedMediaRange{
        media_type: mtype,
        subtype: subtype,
        q_value: NEG_Q_MAX,
        valid: true,
    };

    let pos: usize = first_semi;
    let seen_q: bool = false;
    while (pos < item.len) {
        const seg_start: usize = pos + 1;
        const seg_end: usize = findNextUnquoted(item, seg_start, 0x3B);
        const seg = negTrimOws(item[seg_start..seg_end]);
        pos = seg_end;
        if (seg.len == 0) continue;

        let eq_idx: usize = seg.len;
        let k: usize = 0;
        while (k < seg.len) : (k += 1) {
            if (seg[k] == 0x3D) { // '='
                eq_idx = k;
                break;
            }
        }
        if (eq_idx == 0 || eq_idx >= seg.len - 1) return ParsedMediaRange{};

        const pkey = negTrimOws(seg[0..eq_idx]);
        const pval = unquoteParamVal(seg[eq_idx + 1 ..]);
        if (negEqIgnoreCase(pkey, "q")) {
            const q = parseQValue(pval) orelse return ParsedMediaRange{};
            res.q_value = q;
            seen_q = true;
        } else if (!seen_q) {
            if (res.param_count < 4) {
                res.params[res.param_count] = MediaParam{
                    key: pkey,
                    val: pval,
                };
                res.param_count += 1;
            }
        }
    }

    return res;
}

/// Evaluates how specifically `client` range matches `offered` media type.
/// Returns `null` if non-matching, else specificity `0` (`*/*`), `1` (`type/*`),
/// `2` (`type/subtype`), or `2 + client.param_count` when all client media params match.
fn mediaMatchSpecificity(client: ParsedMediaRange, offered: ParsedMediaRange): u8? {
    if (negEqIgnoreCase(client.media_type, "*") && negEqIgnoreCase(client.subtype, "*")) {
        return 0;
    }
    if (!negEqIgnoreCase(client.media_type, offered.media_type)) {
        return null;
    }
    if (negEqIgnoreCase(client.subtype, "*")) {
        return 1;
    }
    if (!negEqIgnoreCase(client.subtype, offered.subtype)) {
        return null;
    }
    // Check that all client media parameters match offered parameters.
    let i: usize = 0;
    while (i < client.param_count) : (i += 1) {
        let found: bool = false;
        let j: usize = 0;
        while (j < offered.param_count) : (j += 1) {
            if (negEqIgnoreCase(client.params[i].key, offered.params[j].key) &&
                negEqIgnoreCase(client.params[i].val, offered.params[j].val))
            {
                found = true;
                break;
            }
        }
        if (!found) return null;
    }
    return (2 + client.param_count) as u8;
}

/// Negotiates the best media type from `offered` against an HTTP `Accept` header (RFC 9110 §12.5.1).
/// Ranks by `(q_value desc, specificity desc, offered_index asc)` and respects `q=0` explicit exclusion.
pub fn negotiateMediaType(
    accept_header: []const u8,
    offered: []const []const u8,
): MediaMatchResult {
    if (negHasUnsafeCtl(accept_header)) {
        return MediaMatchResult{ status: NegotiationMatchKind.invalid_header };
    }
    if (offered.len == 0) {
        return MediaMatchResult{ status: NegotiationMatchKind.not_acceptable };
    }

    const trimmed = negTrimOws(accept_header);
    if (trimmed.len == 0) {
        // Missing/empty Accept header defaults to `*/*;q=1.0` -> server preference #0.
        return MediaMatchResult{
            status: NegotiationMatchKind.matched,
            offered_index: 0,
            q_value: NEG_Q_MAX,
            specificity: 0,
        };
    }

    // Parse up to NEG_MAX_ITEMS client media ranges on stack.
    let client_ranges: [32]ParsedMediaRange = [ParsedMediaRange{}; 32];
    let client_count: usize = 0;

    let pos: usize = 0;
    while (pos <= trimmed.len && client_count < NEG_MAX_ITEMS) {
        const next_comma: usize = findNextUnquoted(trimmed, pos, 0x2C); // ','
        const seg = negTrimOws(trimmed[pos..next_comma]);
        if (seg.len > 0) {
            const parsed = parseMediaRangeItem(seg);
            if (!parsed.valid) {
                return MediaMatchResult{ status: NegotiationMatchKind.invalid_header };
            }
            client_ranges[client_count] = parsed;
            client_count += 1;
        }
        if (next_comma >= trimmed.len) break;
        pos = next_comma + 1;
    }

    if (client_count == 0) {
        return MediaMatchResult{
            status: NegotiationMatchKind.matched,
            offered_index: 0,
            q_value: NEG_Q_MAX,
            specificity: 0,
        };
    }

    let best_found: bool = false;
    let best_idx: usize = 0;
    let best_q: u16 = 0;
    let best_spec: u8 = 0;

    let o_idx: usize = 0;
    while (o_idx < offered.len) : (o_idx += 1) {
        const off_parsed = parseMediaRangeItem(offered[o_idx]);
        if (!off_parsed.valid) continue;

        // Find the most specific matching client range for this offered media type.
        let matched_any: bool = false;
        let top_spec: u8 = 0;
        let top_q: u16 = 0;

        let c_idx: usize = 0;
        while (c_idx < client_count) : (c_idx += 1) {
            if (spec := mediaMatchSpecificity(client_ranges[c_idx], off_parsed)) {
                if (!matched_any || spec > top_spec ||
                    (spec == top_spec && client_ranges[c_idx].q_value > top_q))
                {
                    matched_any = true;
                    top_spec = spec;
                    top_q = client_ranges[c_idx].q_value;
                }
            }
        }

        if (!matched_any || top_q == 0) continue; // Excluded by q=0 or unmatched

        if (!best_found || top_q > best_q || (top_q == best_q && top_spec > best_spec)) {
            best_found = true;
            best_idx = o_idx;
            best_q = top_q;
            best_spec = top_spec;
        }
    }

    if (!best_found) {
        return MediaMatchResult{ status: NegotiationMatchKind.not_acceptable };
    }
    return MediaMatchResult{
        status: NegotiationMatchKind.matched,
        offered_index: best_idx,
        q_value: best_q,
        specificity: best_spec,
    };
}

// ============================================================================
// 4. RFC 9110 §12.5.3 Accept-Encoding Negotiation
// ============================================================================

struct ParsedCodingItem {
    is_wildcard: bool = false;
    has_known: bool = false;
    encoding: ContentEncoding = ContentEncoding.identity;
    q_value: u16 = NEG_Q_MAX;
    valid: bool = false;
}

fn parseCodingItem(raw_item: []const u8): ParsedCodingItem {
    const item = negTrimOws(raw_item);
    if (item.len == 0) return ParsedCodingItem{};

    const first_semi: usize = findNextUnquoted(item, 0, 0x3B);
    const tok = negTrimOws(item[0..first_semi]);
    if (!negIsValidToken(tok)) return ParsedCodingItem{};

    let res = ParsedCodingItem{
        q_value: NEG_Q_MAX,
        valid: true,
    };
    if (negEqIgnoreCase(tok, "*")) {
        res.is_wildcard = true;
    } else if (enc := ContentEncoding.fromToken(tok)) {
        res.has_known = true;
        res.encoding = enc;
    }

    let pos: usize = first_semi;
    while (pos < item.len) {
        const seg_start: usize = pos + 1;
        const seg_end: usize = findNextUnquoted(item, seg_start, 0x3B);
        const seg = negTrimOws(item[seg_start..seg_end]);
        pos = seg_end;
        if (seg.len == 0) continue;

        let eq_idx: usize = seg.len;
        let k: usize = 0;
        while (k < seg.len) : (k += 1) {
            if (seg[k] == 0x3D) {
                eq_idx = k;
                break;
            }
        }
        if (eq_idx == 0 || eq_idx >= seg.len - 1) return ParsedCodingItem{};
        const pkey = negTrimOws(seg[0..eq_idx]);
        const pval = unquoteParamVal(seg[eq_idx + 1 ..]);
        if (negEqIgnoreCase(pkey, "q")) {
            const q = parseQValue(pval) orelse return ParsedCodingItem{};
            res.q_value = q;
        }
    }
    return res;
}

/// Negotiates the best Content-Encoding from `offered` against `Accept-Encoding` (RFC 9110 §12.5.3).
/// Handles `zstd`/`br`/`gzip`/`deflate`/`identity`/`*`, implicit `identity` fallback, and
/// explicit `identity;q=0` or `*;q=0` returning `not_acceptable` (406).
pub fn negotiateEncoding(
    accept_encoding: []const u8,
    offered: []const ContentEncoding,
): EncodingMatchResult {
    if (negHasUnsafeCtl(accept_encoding)) {
        return EncodingMatchResult{ status: NegotiationMatchKind.invalid_header };
    }
    if (offered.len == 0) {
        return EncodingMatchResult{ status: NegotiationMatchKind.not_acceptable };
    }

    const trimmed = negTrimOws(accept_encoding);
    if (trimmed.len == 0) {
        // Empty Accept-Encoding means client wants no content-coding (only identity if offered).
        let i: usize = 0;
        while (i < offered.len) : (i += 1) {
            if (offered[i] == ContentEncoding.identity) {
                return EncodingMatchResult{
                    status: NegotiationMatchKind.matched,
                    encoding: ContentEncoding.identity,
                    offered_index: i,
                    q_value: NEG_Q_MAX,
                };
            }
        }
        return EncodingMatchResult{ status: NegotiationMatchKind.not_acceptable };
    }

    let items: [32]ParsedCodingItem = [ParsedCodingItem{}; 32];
    let item_count: usize = 0;
    let has_wildcard: bool = false;
    let wildcard_q: u16 = 0;

    let pos: usize = 0;
    while (pos <= trimmed.len && item_count < NEG_MAX_ITEMS) {
        const next_comma: usize = findNextUnquoted(trimmed, pos, 0x2C);
        const seg = negTrimOws(trimmed[pos..next_comma]);
        if (seg.len > 0) {
            const parsed = parseCodingItem(seg);
            if (!parsed.valid) {
                return EncodingMatchResult{ status: NegotiationMatchKind.invalid_header };
            }
            if (parsed.is_wildcard) {
                has_wildcard = true;
                wildcard_q = parsed.q_value;
            }
            items[item_count] = parsed;
            item_count += 1;
        }
        if (next_comma >= trimmed.len) break;
        pos = next_comma + 1;
    }

    let best_found: bool = false;
    let best_idx: usize = 0;
    let best_enc: ContentEncoding = ContentEncoding.identity;
    let best_q: u16 = 0;

    let o_idx: usize = 0;
    while (o_idx < offered.len) : (o_idx += 1) {
        const enc = offered[o_idx];
        let explicit_found: bool = false;
        let eff_q: u16 = 0;

        let c_idx: usize = 0;
        while (c_idx < item_count) : (c_idx += 1) {
            if (items[c_idx].has_known && items[c_idx].encoding == enc) {
                if (!explicit_found || items[c_idx].q_value > eff_q) {
                    explicit_found = true;
                    eff_q = items[c_idx].q_value;
                }
            }
        }

        if (!explicit_found) {
            if (has_wildcard) {
                eff_q = wildcard_q;
            } else if (enc == ContentEncoding.identity) {
                // RFC 9110 §12.5.3: identity is acceptable by default unless identity;q=0 or *;q=0.
                eff_q = 1;
            } else {
                eff_q = 0;
            }
        }

        if (eff_q == 0) continue;
        if (!best_found || eff_q > best_q) {
            best_found = true;
            best_idx = o_idx;
            best_enc = enc;
            best_q = eff_q;
        }
    }

    if (!best_found) {
        return EncodingMatchResult{ status: NegotiationMatchKind.not_acceptable };
    }
    return EncodingMatchResult{
        status: NegotiationMatchKind.matched,
        encoding: best_enc,
        offered_index: best_idx,
        q_value: best_q,
    };
}

// ============================================================================
// 5. RFC 9110 §12.5.4 Accept-Language BCP 47 Negotiation
// ============================================================================

struct ParsedLangItem {
    tag: []const u8 = "",
    q_value: u16 = NEG_Q_MAX,
    valid: bool = false,
}

fn isValidLangTag(s: []const u8): bool {
    if (s.len == 0) return false;
    if (s.len == 1 && s[0] == 0x2A) return true; // '*'
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        const ok: bool = (c >= 0x30 && c <= 0x39) ||
            (c >= 0x41 && c <= 0x5A) ||
            (c >= 0x61 && c <= 0x7A) ||
            (c == 0x2D); // '-'
        if (!ok) return false;
    }
    return s[0] != 0x2D && s[s.len - 1] != 0x2D;
}

fn parseLangItem(raw_item: []const u8): ParsedLangItem {
    const item = negTrimOws(raw_item);
    if (item.len == 0) return ParsedLangItem{};

    const first_semi: usize = findNextUnquoted(item, 0, 0x3B);
    const tag = negTrimOws(item[0..first_semi]);
    if (!isValidLangTag(tag)) return ParsedLangItem{};

    let res = ParsedLangItem{
        tag: tag,
        q_value: NEG_Q_MAX,
        valid: true,
    };

    let pos: usize = first_semi;
    while (pos < item.len) {
        const seg_start: usize = pos + 1;
        const seg_end: usize = findNextUnquoted(item, seg_start, 0x3B);
        const seg = negTrimOws(item[seg_start..seg_end]);
        pos = seg_end;
        if (seg.len == 0) continue;

        let eq_idx: usize = seg.len;
        let k: usize = 0;
        while (k < seg.len) : (k += 1) {
            if (seg[k] == 0x3D) {
                eq_idx = k;
                break;
            }
        }
        if (eq_idx == 0 || eq_idx >= seg.len - 1) return ParsedLangItem{};
        const pkey = negTrimOws(seg[0..eq_idx]);
        const pval = unquoteParamVal(seg[eq_idx + 1 ..]);
        if (negEqIgnoreCase(pkey, "q")) {
            const q = parseQValue(pval) orelse return ParsedLangItem{};
            res.q_value = q;
        }
    }
    return res;
}

/// Computes BCP 47 subtag match score between client tag `client_tag` and server `offered_tag`.
/// Returns `null` if non-matching, `0` for wildcard `*`, or matched prefix byte length
/// (exact match scores `offered_tag.len + 100` to beat partial prefix fallback).
fn langMatchScore(client_tag: []const u8, offered_tag: []const u8): usize? {
    if (client_tag.len == 1 && client_tag[0] == 0x2A) return 0;
    if (negEqIgnoreCase(client_tag, offered_tag)) {
        return offered_tag.len + 100;
    }
    // Client is more specific than server (e.g. client `zh-Hans-CN` falls back to server `zh-Hans` or `zh`)
    if (client_tag.len > offered_tag.len && client_tag[offered_tag.len] == 0x2D) {
        if (negEqIgnoreCase(client_tag[0..offered_tag.len], offered_tag)) {
            return offered_tag.len;
        }
    }
    // Server is more specific than client (e.g. client `zh` matches server `zh-CN`)
    if (offered_tag.len > client_tag.len && offered_tag[client_tag.len] == 0x2D) {
        if (negEqIgnoreCase(offered_tag[0..client_tag.len], client_tag)) {
            return client_tag.len;
        }
    }
    return null;
}

/// Negotiates the best BCP 47 language tag from `offered` against `Accept-Language` (RFC 9110 §12.5.4).
/// Supports case-insensitive exact match, subtag prefix fallback (`zh-Hans-CN` -> `zh-Hans` -> `zh`),
/// wildcard `*`, and `q=0` exclusion.
pub fn negotiateLanguage(
    accept_lang: []const u8,
    offered: []const []const u8,
): LanguageMatchResult {
    if (negHasUnsafeCtl(accept_lang)) {
        return LanguageMatchResult{ status: NegotiationMatchKind.invalid_header };
    }
    if (offered.len == 0) {
        return LanguageMatchResult{ status: NegotiationMatchKind.not_acceptable };
    }

    const trimmed = negTrimOws(accept_lang);
    if (trimmed.len == 0) {
        return LanguageMatchResult{
            status: NegotiationMatchKind.matched,
            offered_index: 0,
            q_value: NEG_Q_MAX,
            matched_prefix_len: 0,
        };
    }

    let items: [32]ParsedLangItem = [ParsedLangItem{}; 32];
    let item_count: usize = 0;

    let pos: usize = 0;
    while (pos <= trimmed.len && item_count < NEG_MAX_ITEMS) {
        const next_comma: usize = findNextUnquoted(trimmed, pos, 0x2C);
        const seg = negTrimOws(trimmed[pos..next_comma]);
        if (seg.len > 0) {
            const parsed = parseLangItem(seg);
            if (!parsed.valid) {
                return LanguageMatchResult{ status: NegotiationMatchKind.invalid_header };
            }
            items[item_count] = parsed;
            item_count += 1;
        }
        if (next_comma >= trimmed.len) break;
        pos = next_comma + 1;
    }

    if (item_count == 0) {
        return LanguageMatchResult{
            status: NegotiationMatchKind.matched,
            offered_index: 0,
            q_value: NEG_Q_MAX,
            matched_prefix_len: 0,
        };
    }

    let best_found: bool = false;
    let best_idx: usize = 0;
    let best_q: u16 = 0;
    let best_score: usize = 0;

    let o_idx: usize = 0;
    while (o_idx < offered.len) : (o_idx += 1) {
        const off_tag = negTrimOws(offered[o_idx]);
        if (!isValidLangTag(off_tag)) continue;

        let matched_any: bool = false;
        let top_score: usize = 0;
        let top_q: u16 = 0;

        let c_idx: usize = 0;
        while (c_idx < item_count) : (c_idx += 1) {
            if (sc := langMatchScore(items[c_idx].tag, off_tag)) {
                if (!matched_any || sc > top_score ||
                    (sc == top_score && items[c_idx].q_value > top_q))
                {
                    matched_any = true;
                    top_score = sc;
                    top_q = items[c_idx].q_value;
                }
            }
        }

        if (!matched_any || top_q == 0) continue;

        if (!best_found || top_q > best_q || (top_q == best_q && top_score > best_score)) {
            best_found = true;
            best_idx = o_idx;
            best_q = top_q;
            best_score = top_score;
        }
    }

    if (!best_found) {
        return LanguageMatchResult{ status: NegotiationMatchKind.not_acceptable };
    }
    const prefix_len: usize = if (best_score >= 100) (best_score - 100) else best_score;
    return LanguageMatchResult{
        status: NegotiationMatchKind.matched,
        offered_index: best_idx,
        q_value: best_q,
        matched_prefix_len: prefix_len,
    };
}

// ============================================================================
// 6. Zero-Alloc Vary Response Header Deduplication & Synthesis
// ============================================================================

/// Merges `existing_vary` (comma-separated header names) with `add_headers` into `out_buf`:
/// - Deduplicates case-insensitively while preserving first-seen canonical casing.
/// - Collapses to `"*"` if any input header is `"*"` (RFC 9110 §12.5.5).
/// - Rejects control/CRLF injection or invalid tokens by returning `null`.
/// Returns total bytes written to `out_buf`, or `null` if invalid or `out_buf` is too small.
pub fn formatVaryHeader(
    existing_vary: []const u8,
    add_headers: []const []const u8,
    out_buf: *[]u8,
): usize? {
    if (negHasUnsafeCtl(existing_vary)) return null;

    let unique: [16][]const u8 = [""; 16];
    let unique_count: usize = 0;
    let is_star: bool = false;

    // 1. Parse existing comma-separated Vary header
    const trimmed = negTrimOws(existing_vary);
    if (trimmed.len > 0) {
        let pos: usize = 0;
        while (pos <= trimmed.len) {
            const next_comma: usize = findNextUnquoted(trimmed, pos, 0x2C);
            const tok = negTrimOws(trimmed[pos..next_comma]);
            if (tok.len > 0) {
                if (tok.len == 1 && tok[0] == 0x2A) {
                    is_star = true;
                } else {
                    if (!negIsValidToken(tok)) return null;
                    let dup: bool = false;
                    let u: usize = 0;
                    while (u < unique_count) : (u += 1) {
                        if (negEqIgnoreCase(unique[u], tok)) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup) {
                        if (unique_count >= NEG_VARY_MAX_HEADERS) return null;
                        unique[unique_count] = tok;
                        unique_count += 1;
                    }
                }
            }
            if (next_comma >= trimmed.len) break;
            pos = next_comma + 1;
        }
    }

    // 2. Merge additional header names
    let a: usize = 0;
    while (a < add_headers.len) : (a += 1) {
        const raw_h = add_headers[a];
        if (negHasUnsafeCtl(raw_h)) return null;
        const tok = negTrimOws(raw_h);
        if (tok.len == 0) continue;
        if (tok.len == 1 && tok[0] == 0x2A) {
            is_star = true;
            continue;
        }
        if (!negIsValidToken(tok)) return null;
        let dup: bool = false;
        let u: usize = 0;
        while (u < unique_count) : (u += 1) {
            if (negEqIgnoreCase(unique[u], tok)) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            if (unique_count >= NEG_VARY_MAX_HEADERS) return null;
            unique[unique_count] = tok;
            unique_count += 1;
        }
    }

    if (is_star) {
        if (out_buf.len < 1) return null;
        out_buf[0] = 0x2A; // '*'
        return 1;
    }
    if (unique_count == 0) return 0;

    let out_pos: usize = 0;
    let i: usize = 0;
    while (i < unique_count) : (i += 1) {
        if (i > 0) {
            if (out_pos + 2 > out_buf.len) return null;
            out_buf[out_pos] = 0x2C; // ','
            out_buf[out_pos + 1] = 0x20; // ' '
            out_pos += 2;
        }
        const h = unique[i];
        if (out_pos + h.len > out_buf.len) return null;
        let k: usize = 0;
        while (k < h.len) : (k += 1) {
            out_buf[out_pos + k] = h[k];
        }
        out_pos += h.len;
    }

    return out_pos;
}
