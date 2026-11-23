// Source: cp/lib/mime/mime.cp (RFC 6838 MediaType Parser/Formatter & RFC 9110 ContentNegotiator)
// Reference-hardened against hyperium/mime (MIT/Apache-2.0)
// Pure CP zero-C-dependency MediaType parser, serializer, and HTTP Accept content negotiator
// with RFC 6838 restricted-name validation and 16-lane @Vector(16, u8) / cap:simd acceleration.

const std = @import("std");
import { Simd } from "cap:simd";

type MimeVec16 = @Vector(16, u8);

/// RFC 6838 §4.2 maximum length of a type or subtype restricted-name (127 bytes).
pub const MIME_MAX_NAME_LEN: usize = 127;

/// Maximum number of parameters accepted per MediaType to bound resource usage.
pub const MIME_MAX_PARAMS: usize = 64;

/// Errors returned during MediaType parsing, formatting, or Accept negotiation.
pub err MimeError {
    InvalidMediaType,
    InvalidParameter,
    InvalidQuality,
    OutOfMemory,
}

/// Key-value parameter attached to a MediaType (e.g., charset="utf-8").
pub struct MimeParam {
    name: []const u8;
    value: []const u8;
}

struct OwnedParam {
    name: []u8;
    value: []u8;
}

fn isTokenChar(c: u8): bool {
    if (c >= '0' && c <= '9') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    return switch (c) {
        '!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~' => true,
        else => false,
    };
}

fn isRestrictedNameFirst(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn isRestrictedNameChar(c: u8): bool {
    if (isRestrictedNameFirst(c)) return true;
    return switch (c) {
        '!', '#', '$', '&', '-', '^', '_', '.', '+' => true,
        else => false,
    };
}

/// 16-lane `@Vector(16, u8)` SIMD validation for RFC 2045/9110 MIME token characters.
pub fn simdIsValidMimeToken(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    const n = s.len;
    const v_0: MimeVec16 = @splat('0');
    const v_10: MimeVec16 = @splat(10);
    const v_a: MimeVec16 = @splat('a');
    const v_26: MimeVec16 = @splat(26);
    const v_case: MimeVec16 = @splat(0x20);
    const v_dash: MimeVec16 = @splat('-');
    const v_under: MimeVec16 = @splat('_');
    const v_dot: MimeVec16 = @splat('.');
    const v_plus: MimeVec16 = @splat('+');

    while (i + 16 <= n) {
        const chunk: MimeVec16 = s[i..][0..16].*;
        const m_dig: u16 = @bitCast((chunk -% v_0) < v_10);
        const m_alp: u16 = @bitCast(((chunk | v_case) -% v_a) < v_26);
        const m_dash: u16 = @bitCast(chunk == v_dash);
        const m_under: u16 = @bitCast(chunk == v_under);
        const m_dot: u16 = @bitCast(chunk == v_dot);
        const m_plus: u16 = @bitCast(chunk == v_plus);
        const fast_ok: u16 = m_dig | m_alp | m_dash | m_under | m_dot | m_plus;
        const non_fast: u16 = ~fast_ok;
        if (non_fast != 0) {
            i += @ctz(non_fast) as usize;
            if (!isTokenChar(s[i])) return false;
            i += 1;
        } else {
            i += 16;
        }
    }

    while (i < n) : (i += 1) {
        if (!isTokenChar(s[i])) return false;
    }
    return true;
}

/// 16-lane `@Vector(16, u8)` SIMD validation for RFC 6838 §4.2 restricted-name
/// (1..=127 bytes, starts with alphanumeric, or wildcard `*`).
pub fn simdIsValidRestrictedName(s: []const u8): bool {
    if (s.len == 0 || s.len > MIME_MAX_NAME_LEN) return false;
    if (s.len == 1 && s[0] == '*') return true;
    if (!isRestrictedNameFirst(s[0])) return false;

    let i: usize = 1;
    const n = s.len;
    const v_0: MimeVec16 = @splat('0');
    const v_10: MimeVec16 = @splat(10);
    const v_a: MimeVec16 = @splat('a');
    const v_26: MimeVec16 = @splat(26);
    const v_case: MimeVec16 = @splat(0x20);
    const v_dash: MimeVec16 = @splat('-');
    const v_under: MimeVec16 = @splat('_');
    const v_dot: MimeVec16 = @splat('.');
    const v_plus: MimeVec16 = @splat('+');

    while (i + 16 <= n) {
        const chunk: MimeVec16 = s[i..][0..16].*;
        const m_dig: u16 = @bitCast((chunk -% v_0) < v_10);
        const m_alp: u16 = @bitCast(((chunk | v_case) -% v_a) < v_26);
        const m_dash: u16 = @bitCast(chunk == v_dash);
        const m_under: u16 = @bitCast(chunk == v_under);
        const m_dot: u16 = @bitCast(chunk == v_dot);
        const m_plus: u16 = @bitCast(chunk == v_plus);
        const fast_ok: u16 = m_dig | m_alp | m_dash | m_under | m_dot | m_plus;
        const non_fast: u16 = ~fast_ok;
        if (non_fast != 0) {
            i += @ctz(non_fast) as usize;
            if (!isRestrictedNameChar(s[i])) return false;
            i += 1;
        } else {
            i += 16;
        }
    }

    while (i < n) : (i += 1) {
        if (!isRestrictedNameChar(s[i])) return false;
    }
    return true;
}

/// SIMD-accelerated delimiter finder for MIME syntax using `cap:simd` `Simd.indexOfAny4('/', ';', '=', '"')`.
pub fn simdFindMimeDelim(s: []const u8): usize? {
    return Simd.indexOfAny4(s, '/', ';', '=', '"');
}

/// SIMD-accelerated semicolon finder using `cap:simd` `Simd.indexOfByte(';')`.
pub fn simdIndexOfSemicolon(s: []const u8): usize? {
    return Simd.indexOfByte(s, ';');
}

fn isValidToken(s: []const u8): bool {
    return simdIsValidMimeToken(s);
}

fn isQuotedTextChar(c: u8): bool {
    return c == '\t' || (c >= 0x20 && c != 0x7F);
}

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isOws(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn toLowerChar(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerChar(a[i]) != toLowerChar(b[i])) return false;
    }
    return true;
}

fn allocLowerCopy(alloc: std.mem.Allocator, s: []const u8): ([]u8) ! MimeError {
    let buf = alloc.alloc(u8, s.len) catch return MimeError.OutOfMemory;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        buf[i] = toLowerChar(s[i]);
    }
    return buf;
}

struct EssenceSplit {
    slash_idx: usize;
    plus_idx: usize?;
}

fn validateTypeSubtype(raw_essence: []const u8): EssenceSplit ! MimeError {
    if (raw_essence.len == 0) return MimeError.InvalidMediaType;

    const first_delim = simdFindMimeDelim(raw_essence) ?? return MimeError.InvalidMediaType;
    if (raw_essence[first_delim] != '/') return MimeError.InvalidMediaType;
    const s_idx = first_delim;
    if (s_idx == 0 || s_idx + 1 >= raw_essence.len) {
        return MimeError.InvalidMediaType;
    }

    const t = raw_essence[0..s_idx];
    const st = raw_essence[(s_idx + 1)..];
    // Reject multiple '/', ';', '=', or '"' inside subtype
    if (simdFindMimeDelim(st) != null) {
        return MimeError.InvalidMediaType;
    }
    if (!simdIsValidRestrictedName(t) || !simdIsValidRestrictedName(st)) {
        return MimeError.InvalidMediaType;
    }
    // RFC 9110: "*/subtype" is invalid unless subtype is also "*"
    if (t.len == 1 && t[0] == '*' && !(st.len == 1 && st[0] == '*')) {
        return MimeError.InvalidMediaType;
    }

    // Extract optional structured syntax suffix (+json, +xml, etc.)
    let plus_idx: usize? = null;
    let j: usize = st.len;
    while (j > 0) {
        j -= 1;
        if (st[j] == '+') {
            plus_idx = s_idx + 1 + j;
            break;
        }
    }
    if (p_idx := plus_idx) {
        // Reject leading '+' (empty subtype prefix) or trailing '+' (empty suffix)
        if (p_idx == s_idx + 1 || p_idx + 1 >= raw_essence.len) {
            return MimeError.InvalidMediaType;
        }
    }

    return EssenceSplit{ slash_idx: s_idx, plus_idx: plus_idx };
}

fn parseParamsList(
    alloc: std.mem.Allocator,
    raw_params: []const u8,
): ([]OwnedParam) ! MimeError {
    let list: std.ArrayList(OwnedParam) = .empty;
    errdefer {
        for (p of list.items) {
            alloc.free(p.name);
            alloc.free(p.value);
        }
        list.deinit(alloc);
    }

    let pos: usize = 0;
    while (pos < raw_params.len) {
        // Skip OWS and ';'
        while (pos < raw_params.len && isOws(raw_params[pos])) {
            pos += 1;
        }
        if (pos >= raw_params.len) break;

        if (raw_params[pos] != ';') {
            return MimeError.InvalidParameter;
        }
        pos += 1;

        while (pos < raw_params.len && isOws(raw_params[pos])) {
            pos += 1;
        }
        if (pos >= raw_params.len) break;

        if (list.items.len >= MIME_MAX_PARAMS) {
            return MimeError.InvalidParameter;
        }

        // Parse parameter name up to '=' using SIMD delimiter scan
        const name_start = pos;
        const delim_rel = simdFindMimeDelim(raw_params[pos..]) ?? return MimeError.InvalidParameter;
        pos += delim_rel;
        if (raw_params[pos] != '=') {
            return MimeError.InvalidParameter;
        }
        const raw_name = trimOws(raw_params[name_start..pos]);
        if (raw_name.len > MIME_MAX_NAME_LEN || !simdIsValidMimeToken(raw_name)) {
            return MimeError.InvalidParameter;
        }
        pos += 1; // skip '='

        while (pos < raw_params.len && isOws(raw_params[pos])) {
            pos += 1;
        }
        if (pos >= raw_params.len) {
            return MimeError.InvalidParameter;
        }

        let owned_val: []u8 = [];
        if (raw_params[pos] == '"') {
            // Quoted-string parameter value (RFC 9110 §5.6.4)
            pos += 1;
            let val_buf: std.ArrayList(u8) = .empty;
            errdefer val_buf.deinit(alloc);
            let closed = false;
            while (pos < raw_params.len) {
                const ch = raw_params[pos];
                if (ch == '\\') {
                    pos += 1;
                    if (pos >= raw_params.len || !isQuotedTextChar(raw_params[pos])) {
                        return MimeError.InvalidParameter;
                    }
                    val_buf.append(alloc, raw_params[pos]) catch return MimeError.OutOfMemory;
                    pos += 1;
                } else if (ch == '"') {
                    pos += 1;
                    closed = true;
                    break;
                } else {
                    if (!isQuotedTextChar(ch)) {
                        return MimeError.InvalidParameter;
                    }
                    val_buf.append(alloc, ch) catch return MimeError.OutOfMemory;
                    pos += 1;
                }
            }
            if (!closed) {
                return MimeError.InvalidParameter;
            }
            // Only OWS or ';' may follow a closing quote
            while (pos < raw_params.len && isOws(raw_params[pos])) {
                pos += 1;
            }
            if (pos < raw_params.len && raw_params[pos] != ';') {
                return MimeError.InvalidParameter;
            }
            owned_val = val_buf.toOwnedSlice(alloc) catch return MimeError.OutOfMemory;
        } else {
            // Token parameter value
            const val_start = pos;
            if (semi_rel := simdIndexOfSemicolon(raw_params[pos..])) {
                pos += semi_rel;
            } else {
                pos = raw_params.len;
            }
            const raw_val = trimOws(raw_params[val_start..pos]);
            if (!simdIsValidMimeToken(raw_val)) {
                return MimeError.InvalidParameter;
            }
            owned_val = alloc.alloc(u8, raw_val.len) catch return MimeError.OutOfMemory;
            std.mem.copyForwards(u8, owned_val, raw_val);
        }

        errdefer alloc.free(owned_val);
        const owned_name = allocLowerCopy(alloc, raw_name)?;
        errdefer alloc.free(owned_name);

        list.append(alloc, OwnedParam{
            name: owned_name,
            value: owned_val,
        }) catch return MimeError.OutOfMemory;
    }

    return list.toOwnedSlice(alloc) catch return MimeError.OutOfMemory;
}

/// Parsed RFC 6838 / RFC 9110 MediaType with normalized lowercase type/subtype,
/// optional structured syntax suffix (e.g. "+json"), and unescaped parameters.
pub struct MediaType {
    alloc: std.mem.Allocator;
    raw_essence: []u8;
    type: []const u8;
    subtype: []const u8;
    suffix: []const u8;
    params: []MimeParam;
    owned_params: []OwnedParam;

    /// Parses a MediaType string such as `Text/HTML; charset="utf-8"; boundary=abc`.
    pub static fn parse(alloc: std.mem.Allocator, raw: []const u8): MediaType ! MimeError {
        const trimmed = trimOws(raw);
        if (trimmed.len == 0) return MimeError.InvalidMediaType;

        // Split essence from optional parameters at first ';' via SIMD
        const semi_idx = simdIndexOfSemicolon(trimmed) ?? trimmed.len;

        const essence_slice = trimOws(trimmed[0..semi_idx]);
        const meta = validateTypeSubtype(essence_slice)?;

        const raw_essence = allocLowerCopy(alloc, essence_slice)?;
        errdefer alloc.free(raw_essence);

        let param_slice: []const u8 = "";
        if (semi_idx < trimmed.len) {
            param_slice = trimmed[semi_idx..];
        }
        const owned_params = parseParamsList(alloc, param_slice)?;
        errdefer {
            for (p of owned_params) {
                alloc.free(p.name);
                alloc.free(p.value);
            }
            alloc.free(owned_params);
        }

        let params = alloc.alloc(MimeParam, owned_params.len) catch return MimeError.OutOfMemory;
        let p_i: usize = 0;
        while (p_i < owned_params.len) : (p_i += 1) {
            params[p_i] = MimeParam{
                name: owned_params[p_i].name,
                value: owned_params[p_i].value,
            };
        }

        const type_part = raw_essence[0..meta.slash_idx];
        const subtype_part = raw_essence[(meta.slash_idx + 1)..];
        let suffix_part: []const u8 = "";
        if (p_idx := meta.plus_idx) {
            suffix_part = raw_essence[p_idx..];
        }

        return MediaType{
            alloc: alloc,
            raw_essence: raw_essence,
            type: type_part,
            subtype: subtype_part,
            suffix: suffix_part,
            params: params,
            owned_params: owned_params,
        };
    }

    /// Releases all heap memory owned by this MediaType.
    pub fn deinit(*_) {
        if (_.raw_essence.len > 0) {
            _.alloc.free(_.raw_essence);
        }
        for (p of _.owned_params) {
            _.alloc.free(p.name);
            _.alloc.free(p.value);
        }
        if (_.owned_params.len > 0) {
            _.alloc.free(_.owned_params);
        }
        if (_.params.len > 0) {
            _.alloc.free(_.params);
        }
        _.raw_essence = [];
        _.owned_params = [];
        _.params = [];
    }

    /// Returns the `"type/subtype"` essence without parameters.
    pub fn essence(): []const u8 {
        return _.raw_essence;
    }

    /// Case-insensitive lookup for a parameter value by name.
    pub fn getParam(name: []const u8): ([]const u8)? {
        for (p of _.params) {
            if (asciiEqlIgnoreCase(p.name, name)) {
                return p.value;
            }
        }
        return null;
    }

    /// Convenience helper returning the `"charset"` parameter if present.
    pub fn charset(): ([]const u8)? {
        return _.getParam("charset");
    }

    /// Matches this MediaType against a pattern such as `"*/*"`, `"text/*"`,
    /// `"text/html"`, `"+json"`, `"*+json"`, `"*/*+json"`, or `"application/*+json"`.
    pub fn matches(pattern: []const u8): bool {
        let pat = trimOws(pattern);
        if (pat.len == 0) return false;

        // Strip any trailing parameters from pattern
        let s_idx: usize = 0;
        while (s_idx < pat.len) : (s_idx += 1) {
            if (pat[s_idx] == ';') {
                pat = trimOws(pat[0..s_idx]);
                break;
            }
        }
        if (pat.len == 0) return false;

        // Structured suffix shorthand: "+json" or "*+json"
        if (pat[0] == '+') {
            return _.suffix.len > 0 && asciiEqlIgnoreCase(_.suffix, pat);
        }
        if (pat.len >= 2 && pat[0] == '*' && pat[1] == '+') {
            return _.suffix.len > 0 && asciiEqlIgnoreCase(_.suffix, pat[1..]);
        }

        // Find '/'
        let slash: usize? = null;
        let i: usize = 0;
        while (i < pat.len) : (i += 1) {
            if (pat[i] == '/') {
                if (slash != null) return false;
                slash = i;
            }
        }
        const sl = slash ?? return false;
        const pat_type = pat[0..sl];
        const pat_sub = pat[(sl + 1)..];
        if (pat_type.len == 0 || pat_sub.len == 0) return false;

        const type_ok = asciiEqlIgnoreCase(pat_type, "*") || asciiEqlIgnoreCase(_.type, pat_type);
        if (!type_ok) return false;

        if (asciiEqlIgnoreCase(pat_sub, "*")) return true;
        if (pat_sub.len >= 2 && pat_sub[0] == '*' && pat_sub[1] == '+') {
            return _.suffix.len > 0 && asciiEqlIgnoreCase(_.suffix, pat_sub[1..]);
        }
        return asciiEqlIgnoreCase(_.subtype, pat_sub);
    }

    /// Formats the MediaType back into a canonical RFC 6838 string.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn toString(alloc: std.mem.Allocator): ([]u8) ! MimeError {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        buf.appendSlice(alloc, _.raw_essence) catch return MimeError.OutOfMemory;
        for (p of _.params) {
            buf.appendSlice(alloc, "; ") catch return MimeError.OutOfMemory;
            buf.appendSlice(alloc, p.name) catch return MimeError.OutOfMemory;
            buf.append(alloc, '=') catch return MimeError.OutOfMemory;
            if (isValidToken(p.value)) {
                buf.appendSlice(alloc, p.value) catch return MimeError.OutOfMemory;
            } else {
                buf.append(alloc, '"') catch return MimeError.OutOfMemory;
                for (ch of p.value) {
                    if (ch == '"' || ch == '\\') {
                        buf.append(alloc, '\\') catch return MimeError.OutOfMemory;
                    }
                    buf.append(alloc, ch) catch return MimeError.OutOfMemory;
                }
                buf.append(alloc, '"') catch return MimeError.OutOfMemory;
            }
        }

        return buf.toOwnedSlice(alloc) catch return MimeError.OutOfMemory;
    }
}

struct AcceptRange {
    mt: MediaType;
    q: f64;
    specificity: u8; // 3 = type/subtype, 2 = type/*, 1 = */*
    non_q_params: usize;
}

fn parseQualityValue(raw: []const u8): f64 ! MimeError {
    if (raw.len == 0) return MimeError.InvalidQuality;
    const val = std.fmt.parseFloat(f64, raw) catch return MimeError.InvalidQuality;
    if (std.math.isNan(val) || val < 0.0 || val > 1.0) {
        return MimeError.InvalidQuality;
    }
    return val;
}

struct PartSlice {
    s: []const u8;
}

fn splitAcceptRanges(alloc: std.mem.Allocator, header: []const u8): ([]PartSlice) ! MimeError {
    let parts: std.ArrayList(PartSlice) = .empty;
    errdefer parts.deinit(alloc);

    let in_quotes = false;
    let escaped = false;
    let start: usize = 0;
    let i: usize = 0;
    while (i < header.len) : (i += 1) {
        const ch = header[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (in_quotes) {
            if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                in_quotes = false;
            }
            continue;
        }
        if (ch == '"') {
            in_quotes = true;
        } else if (ch == ',') {
            parts.append(alloc, PartSlice{ s: trimOws(header[start..i]) }) catch return MimeError.OutOfMemory;
            start = i + 1;
        }
    }
    parts.append(alloc, PartSlice{ s: trimOws(header[start..]) }) catch return MimeError.OutOfMemory;
    return parts.toOwnedSlice(alloc) catch return MimeError.OutOfMemory;
}

fn rangeMatchesMediaType(range: AcceptRange, candidate: MediaType): bool {
    const r_type = range.mt.type;
    const r_sub = range.mt.subtype;
    if (!asciiEqlIgnoreCase(r_type, "*") && !asciiEqlIgnoreCase(r_type, candidate.type)) {
        return false;
    }
    if (!asciiEqlIgnoreCase(r_sub, "*") && !asciiEqlIgnoreCase(r_sub, candidate.subtype)) {
        return false;
    }
    // Check non-q parameters specified in the Accept range against candidate parameters
    for (rp of range.mt.params) {
        if (asciiEqlIgnoreCase(rp.name, "q")) continue;
        const c_val = candidate.getParam(rp.name) ?? return false;
        if (!asciiEqlIgnoreCase(c_val, rp.value)) return false;
    }
    return true;
}

/// RFC 9110 HTTP `Accept` content negotiation engine.
pub static class ContentNegotiator {
    /// Selects the best matching media type from `available_types` for the given `accept_header`.
    /// Returns `null` if no available type is acceptable (including explicit `q=0` exclusions).
    pub fn negotiateAccept(
        alloc: std.mem.Allocator,
        accept_header: []const u8,
        available_types: []const []const u8,
    ): string?! {
        if (available_types.len == 0) return null;

        const trimmed = trimOws(accept_header);
        if (trimmed.len == 0) {
            return available_types[0];
        }

        const raw_parts = splitAcceptRanges(alloc, trimmed)?;
        defer alloc.free(raw_parts);

        let ranges: std.ArrayList(AcceptRange) = .empty;
        defer {
            for (*r of ranges.items) {
                r.mt.deinit();
            }
            ranges.deinit(alloc);
        }

        for (part of raw_parts) {
            if (part.s.len == 0) continue;
            let mt = MediaType.parse(alloc, part.s)?;
            errdefer mt.deinit();

            let q: f64 = 1.0;
            let non_q: usize = 0;
            for (p of mt.params) {
                if (asciiEqlIgnoreCase(p.name, "q")) {
                    q = parseQualityValue(p.value)?;
                } else {
                    non_q += 1;
                }
            }

            let spec: u8 = 3;
            if (asciiEqlIgnoreCase(mt.type, "*") && asciiEqlIgnoreCase(mt.subtype, "*")) {
                spec = 1;
            } else if (asciiEqlIgnoreCase(mt.subtype, "*")) {
                spec = 2;
            }

            ranges.append(alloc, AcceptRange{
                mt: mt,
                q: q,
                specificity: spec,
                non_q_params: non_q,
            }) catch return MimeError.OutOfMemory;
        }

        if (ranges.items.len == 0) {
            return available_types[0];
        }

        let best_idx: usize? = null;
        let best_q: f64 = 0.0;
        let best_spec: u8 = 0;
        let best_params: usize = 0;

        let idx: usize = 0;
        while (idx < available_types.len) : (idx += 1) {
            let cand = MediaType.parse(alloc, available_types[idx])?;
            defer cand.deinit();

            // RFC 9110: the most specific matching media-range determines this candidate's q-value
            let matched_any = false;
            let win_q: f64 = 0.0;
            let win_spec: u8 = 0;
            let win_params: usize = 0;

            for (r of ranges.items) {
                if (!rangeMatchesMediaType(r, cand)) continue;
                if (!matched_any ||
                    r.specificity > win_spec ||
                    (r.specificity == win_spec && r.non_q_params > win_params) ||
                    (r.specificity == win_spec && r.non_q_params == win_params && r.q > win_q))
                {
                    matched_any = true;
                    win_q = r.q;
                    win_spec = r.specificity;
                    win_params = r.non_q_params;
                }
            }

            if (!matched_any || win_q <= 0.0) continue;

            if (best_idx == null ||
                win_q > best_q ||
                (win_q == best_q && win_spec > best_spec) ||
                (win_q == best_q && win_spec == best_spec && win_params > best_params))
            {
                best_idx = idx;
                best_q = win_q;
                best_spec = win_spec;
                best_params = win_params;
            }
        }

        if (winner := best_idx) {
            return available_types[winner];
        }
        return null;
    }
}
