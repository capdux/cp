// @cap/media-typer — Zero-heap RFC 6838 Media Type parser, validator, and formatter.
// Clean-room MIT port of npm `media-typer` v1.1; reuses `@cap/unicase`.

import { foldAsciiByte, eqAscii } from "../../unicase/src/index.cp";

pub struct MediaType {
    pub valid: bool,
    pub main_type: string,
    pub subtype: string,
    pub suffix: string,
}

fn mtIsAlphaNum(b: u8): bool {
    return (b >= 48 && b <= 57) || (b >= 65 && b <= 90) || (b >= 97 && b <= 122);
}

fn mtIsRestrictedChar(b: u8): bool {
    // RFC 6838 restricted-name-chars: ALPHA / DIGIT / "!" / "#" / "$" / "&" / "-" / "^" / "_" / "." / "+"
    if (mtIsAlphaNum(b)) { return true; }
    return b == 33 || b == 35 || b == 36 || b == 38 || b == 45 || b == 94 || b == 95 || b == 46 || b == 43;
}

fn mtIsValidName(s: string): bool {
    if (s.len == 0 || s.len > 127) { return false; }
    if (!mtIsAlphaNum(s[0])) { return false; }
    let i: usize = 1;
    while (i < s.len) {
        if (!mtIsRestrictedChar(s[i])) { return false; }
        i = i + 1;
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

/// Validates an RFC 6838 media type string (`type/subtype[+suffix]`).
pub fn testMediaType(raw: string): bool {
    return parseMediaType(raw).valid;
}

/// Parses an RFC 6838 media type into `(main_type, subtype, suffix)`.
pub fn parseMediaType(raw: string): MediaType {
    let invalid = MediaType { valid: false, main_type: "", subtype: "", suffix: "" };
    if (raw.len == 0) { return invalid; }
    let slash: usize = raw.len;
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == 47) {
            if (slash < raw.len) { return invalid; } // duplicate '/'
            slash = i;
        }
        i = i + 1;
    }
    if (slash == 0 || slash >= raw.len - 1) { return invalid; }
    let t = raw[0..slash];
    let rest = raw[slash + 1..raw.len];
    if (!mtIsValidName(t)) { return invalid; }

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
        if (!mtIsValidName(sub) || !mtIsValidName(suf)) { return invalid; }
        // Reject '+' inside suffix (already guaranteed by last '+') or inside main_type
        return MediaType { valid: true, main_type: t, subtype: sub, suffix: suf };
    }

    if (!mtIsValidName(rest)) { return invalid; }
    return MediaType { valid: true, main_type: t, subtype: rest, suffix: "" };
}

/// Formats `(main_type, subtype, suffix)` as lowercase RFC 6838 media type into `out`.
/// Returns 0 if any component is invalid.
pub fn formatMediaType(main_type: string, subtype: string, suffix: string, out: []u8): usize {
    if (!mtIsValidName(main_type) || !mtIsValidName(subtype)) { return 0; }
    if (suffix.len > 0 && !mtIsValidName(suffix)) { return 0; }
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

/// Checks whether two media types are equivalent (case-insensitive per RFC 6838).
pub fn mediaTypeEq(a: string, b: string): bool {
    let pa = parseMediaType(a);
    let pb = parseMediaType(b);
    if (!pa.valid || !pb.valid) { return false; }
    return eqAscii(pa.main_type, pb.main_type) && eqAscii(pa.subtype, pb.subtype) && eqAscii(pa.suffix, pb.suffix);
}
