// @cap/media-typer — Zero-heap RFC 6838 Media Type parser, validator, and formatter.
// Clean-room MIT port of npm `media-typer` v1.1 (`parse`, `format`, `test`); reuses `@cap/unicase`.

import { foldAsciiByte, eqAscii } from "../../unicase/src/index.cp";

pub struct MediaType {
    pub valid: bool,
    pub main_type: string,
    pub subtype: string,
    pub suffix: string,

    pub static fn validate(raw: string): bool {
        return testMediaType(raw);
    }

    pub fn isValid(): bool {
        return _.valid;
    }

    pub fn hasSuffix(): bool {
        return _.valid && _.suffix.len > 0;
    }

    pub fn formatTo(out: []u8): usize {
        if (!_.valid) { return 0; }
        return formatMediaType(_.main_type, _.subtype, _.suffix, out);
    }
}

fn mtIsAlphaNum(b: u8): bool {
    return (b >= 48 && b <= 57) || (b >= 65 && b <= 90) || (b >= 97 && b <= 122);
}

fn mtIsTypeChar(b: u8): bool {
    // RFC 6838 / media-typer type-name: ALPHA / DIGIT / "!" / "#" / "$" / "&" / "^" / "_" / "-"
    if (mtIsAlphaNum(b)) { return true; }
    return b == 33 || b == 35 || b == 36 || b == 38 || b == 45 || b == 94 || b == 95;
}

fn mtIsSuffixChar(b: u8): bool {
    // RFC 6838 suffix: ALPHA / DIGIT / "!" / "#" / "$" / "&" / "-" / "^" / "_" / "." (no '+')
    if (mtIsTypeChar(b)) { return true; }
    return b == 46;
}

fn mtIsSubtypeChar(b: u8): bool {
    // RFC 6838 subtype: restricted-name-chars including '.' and '+'
    if (mtIsSuffixChar(b)) { return true; }
    return b == 43;
}

/// Validates an RFC 6838 top-level `type` (`[A-Za-z0-9][A-Za-z0-9!#$&^_-]{0,126}`).
pub fn isValidType(s: string): bool {
    if (s.len == 0 || s.len > 127) { return false; }
    if (!mtIsAlphaNum(s[0])) { return false; }
    let i: usize = 1;
    while (i < s.len) : (i += 1) {
        if (!mtIsTypeChar(s[i])) { return false; }
    }
    return true;
}

/// Validates an RFC 6838 `subtype` (`[A-Za-z0-9][A-Za-z0-9!#$&^_.+-]{0,126}`).
pub fn isValidSubtype(s: string): bool {
    if (s.len == 0 || s.len > 127) { return false; }
    if (!mtIsAlphaNum(s[0])) { return false; }
    let i: usize = 1;
    while (i < s.len) : (i += 1) {
        if (!mtIsSubtypeChar(s[i])) { return false; }
    }
    return true;
}

/// Validates an RFC 6838 structured syntax `suffix` (`[A-Za-z0-9][A-Za-z0-9!#$&^_.-]{0,126}`).
pub fn isValidSuffix(s: string): bool {
    if (s.len == 0 || s.len > 127) { return false; }
    if (!mtIsAlphaNum(s[0])) { return false; }
    let i: usize = 1;
    while (i < s.len) : (i += 1) {
        if (!mtIsSuffixChar(s[i])) { return false; }
    }
    return true;
}

fn mtPushLower(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = foldAsciiByte(s[i]);
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn mtStartsWithAscii(s: string, prefix: string): bool {
    if (s.len < prefix.len) { return false; }
    return eqAscii(s[0..prefix.len], prefix);
}

/// Validates an RFC 6838 media type string (`type/subtype[+suffix]`).
pub fn testMediaType(raw: string): bool {
    return parseMediaType(raw).valid;
}

/// Validates an RFC 6838 media type string (concise alias for `testMediaType`; avoids collision with `cap:test`'s `test`).
pub fn testType(raw: string): bool {
    return testMediaType(raw);
}

/// Parses an RFC 6838 media type into `(main_type, subtype, suffix)`.
pub fn parseMediaType(raw: string): MediaType {
    let invalid = MediaType { valid: false, main_type: "", subtype: "", suffix: "" };
    if (raw.len == 0) { return invalid; }

    // RFC 6838 / media-typer allows optional leading/trailing ASCII spaces
    let start: usize = 0;
    while (start < raw.len && raw[start] == 32) : (start += 1) {}
    let end: usize = raw.len;
    while (end > start && raw[end - 1] == 32) : (end -= 1) {}
    const trimmed = raw[start..end];
    if (trimmed.len == 0) { return invalid; }

    let slash: usize = trimmed.len;
    let i: usize = 0;
    while (i < trimmed.len) {
        if (trimmed[i] == 47) {
            if (slash < trimmed.len) { return invalid; } // duplicate '/'
            slash = i;
        }
        i = i + 1;
    }
    if (slash == 0 || slash >= trimmed.len - 1) { return invalid; }
    let t = trimmed[0..slash];
    let rest = trimmed[slash + 1..trimmed.len];
    if (!isValidType(t)) { return invalid; }
    if (rest.len > 127) { return invalid; }

    // Find last '+' in subtype for structured syntax suffix
    let plus: usize = rest.len;
    let j: usize = 0;
    while (j < rest.len) {
        if (rest[j] == 43) { plus = j; }
        j = j + 1;
    }

    if (plus < rest.len) {
        let sub = rest[0..plus];
        let suf = rest[plus + 1..rest.len];
        if (!isValidSubtype(sub) || !isValidSuffix(suf)) { return invalid; }
        return MediaType { valid: true, main_type: t, subtype: sub, suffix: suf };
    }

    if (!isValidSubtype(rest)) { return invalid; }
    return MediaType { valid: true, main_type: t, subtype: rest, suffix: "" };
}

/// npm `media-typer` `parse`: parses an RFC 6838 media type string.
pub fn parse(raw: string): MediaType {
    return parseMediaType(raw);
}

/// Formats `(main_type, subtype, suffix)` as lowercase RFC 6838 media type into `out`.
/// Returns 0 if any component is invalid.
pub fn formatMediaType(main_type: string, subtype: string, suffix: string, out: []u8): usize {
    if (!isValidType(main_type) || !isValidSubtype(subtype)) { return 0; }
    if (suffix.len > 0) {
        if (!isValidSuffix(suffix)) { return 0; }
        if (subtype.len + 1 + suffix.len > 127) { return 0; }
    }
    let p: usize = 0;
    p = mtPushLower(out, p, main_type);
    if (p < out.len) { out[p] = 47; p = p + 1; }
    p = mtPushLower(out, p, subtype);
    if (suffix.len > 0) {
        if (p < out.len) { out[p] = 43; p = p + 1; }
        p = mtPushLower(out, p, suffix);
    }
    return p;
}

/// npm `media-typer` `format`: formats a `MediaType` object into `out` as lowercase RFC 6838 string.
pub fn format(mt: MediaType, out: []u8): usize {
    return formatMediaType(mt.main_type, mt.subtype, mt.suffix, out);
}

/// Normalizes a media type string into canonical lowercase `type/subtype[+suffix]` in `out`.
/// Returns 0 if `raw` is invalid.
pub fn essence(raw: string, out: []u8): usize {
    const parsed = parseMediaType(raw);
    if (!parsed.valid) { return 0; }
    return formatMediaType(parsed.main_type, parsed.subtype, parsed.suffix, out);
}

/// Checks whether two media types are equivalent (case-insensitive per RFC 6838).
pub fn mediaTypeEq(a: string, b: string): bool {
    let pa = parseMediaType(a);
    let pb = parseMediaType(b);
    if (!pa.valid || !pb.valid) { return false; }
    return eqAscii(pa.main_type, pb.main_type) && eqAscii(pa.subtype, pb.subtype) && eqAscii(pa.suffix, pb.suffix);
}

/// Returns `true` if `raw` is a valid media type with a structured syntax suffix matching `suffix` (case-insensitive).
pub fn hasSuffix(raw: string, suffix: string): bool {
    const p = parseMediaType(raw);
    if (!p.valid || p.suffix.len == 0) { return false; }
    return eqAscii(p.suffix, suffix);
}

/// Returns `true` if `t` is one of the IANA registered top-level media types.
pub fn isStandardType(t: string): bool {
    return eqAscii(t, "application")
        || eqAscii(t, "audio")
        || eqAscii(t, "example")
        || eqAscii(t, "font")
        || eqAscii(t, "haptics")
        || eqAscii(t, "image")
        || eqAscii(t, "message")
        || eqAscii(t, "model")
        || eqAscii(t, "multipart")
        || eqAscii(t, "text")
        || eqAscii(t, "video");
}

/// Returns `true` if `subtype` belongs to the RFC 6838 vendor tree (`vnd.*`).
pub fn isVendorSubtype(subtype: string): bool {
    return mtStartsWithAscii(subtype, "vnd.");
}

/// Returns `true` if `subtype` belongs to the RFC 6838 personal/vanity tree (`prs.*`).
pub fn isPersonalSubtype(subtype: string): bool {
    return mtStartsWithAscii(subtype, "prs.");
}

/// Returns `true` if `subtype` belongs to the unregistered tree (`x.*` or `x-*`).
pub fn isUnregisteredSubtype(subtype: string): bool {
    return mtStartsWithAscii(subtype, "x.") || mtStartsWithAscii(subtype, "x-");
}
