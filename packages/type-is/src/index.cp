// @cap/type-is — Zero-heap HTTP Content-Type matcher modeled on npm `type-is` v1.6.
// Clean-room MIT implementation; reuses `@cap/mime-types` and `@cap/unicase`.

import { trim, findByte } from "cap:ascii";
import { lookup, parseMimeType } from "../../mime-types/src/index.cp";
import { eqAscii, endsWithIgnoreCase } from "../../unicase/src/index.cp";

/// Minimal HTTP request header tuple for `typeis` and `requestHasBody`.
pub struct RequestHeaders {
    pub content_type: string,
    pub transfer_encoding: string,
    pub content_length: string,
}

/// Normalizes a shorthand type pattern (e.g., `"json"` -> `"application/json"`,
/// `"urlencoded"` -> `"application/x-www-form-urlencoded"`, `"multipart"` -> `"multipart/*"`).
pub fn normalizeTypePattern(pattern: string): string {
    let t = trim(pattern);
    if (t.len == 0) { return ""; }
    if (eqAscii(t, "urlencoded")) { return "application/x-www-form-urlencoded"; }
    if (eqAscii(t, "multipart")) { return "multipart/*"; }
    if (t.len == 1 && t[0] == 42) { return "*/*"; }
    if (t[0] == 43) { return t; } // "+json", "+xml" structured suffix
    if (findByte(t, 47, 0) != null) { return t; } // already "type/subtype"
    return lookup(t) ?? t;
}

/// Strict npm `type-is.normalize(type)` equivalent.
/// Returns `null` if the pattern is empty or an unknown extension without a slash.
pub fn normalize(pattern: string): string? {
    let t = trim(pattern);
    if (t.len == 0) { return null; }
    if (eqAscii(t, "urlencoded")) { return "application/x-www-form-urlencoded"; }
    if (eqAscii(t, "multipart")) { return "multipart/*"; }
    if (t.len == 1 && t[0] == 42) { return "*/*"; }
    if (t[0] == 43) {
        if (t.len == 1) { return null; }
        return t;
    }
    if (findByte(t, 47, 0) != null) { return t; }
    return lookup(t);
}

/// Extracts the `type/subtype` essence from a Content-Type header (stripping parameters).
/// Returns `null` if the header is empty or malformed.
pub fn extractEssence(content_type: string): string? {
    let parsed = parseMimeType(content_type);
    if (parsed.essence.len == 0 || parsed.type_part.len == 0 || parsed.subtype.len == 0) {
        return null;
    }
    return parsed.essence;
}

/// Returns `true` if `content_type` parses as a valid `type/subtype` MIME header.
pub fn isValidContentType(content_type: string): bool {
    return extractEssence(content_type) != null;
}

/// Matches a single `content_type` header (e.g., `"application/vnd.api+json; charset=utf-8"`)
/// against one pattern (`"json"`, `"text/*"`, `"*/*"`, `"+json"`, `"application/*+json"`, `"application/json"`).
pub fn typeMatches(content_type: string, pattern: string): bool {
    let parsed = parseMimeType(content_type);
    if (parsed.essence.len == 0 || parsed.type_part.len == 0 || parsed.subtype.len == 0) {
        return false;
    }
    let norm = normalizeTypePattern(pattern);
    if (norm.len == 0) { return false; }

    // Structured suffix shorthand: "+json", "+xml"
    if (norm[0] == 43) {
        if (norm.len == 1) { return false; }
        return endsWithIgnoreCase(parsed.subtype, norm) || eqAscii(parsed.subtype, norm[1..norm.len]);
    }

    let slash_opt = findByte(norm, 47, 0);
    if (slash_opt == null) {
        return eqAscii(parsed.essence, norm);
    }
    let slash = slash_opt.?;
    let pat_type = norm[0..slash];
    let pat_sub = norm[slash + 1..norm.len];
    if (pat_type.len == 0 || pat_sub.len == 0) { return false; }

    // Type part match (`*` or exact)
    let type_ok = (pat_type.len == 1 && pat_type[0] == 42) || eqAscii(parsed.type_part, pat_type);
    if (!type_ok) { return false; }

    // Subtype wildcard `*`
    if (pat_sub.len == 1 && pat_sub[0] == 42) { return true; }

    // Subtype suffix wildcard `*+json`
    if (pat_sub.len >= 2 && pat_sub[0] == 42 && pat_sub[1] == 43) {
        let suffix = pat_sub[1..pat_sub.len]; // "+json"
        return endsWithIgnoreCase(parsed.subtype, suffix) || eqAscii(parsed.subtype, pat_sub[2..pat_sub.len]);
    }

    return eqAscii(parsed.subtype, pat_sub);
}

/// npm `type-is.match(expected, actual)` equivalent.
/// Checks whether `actual` MIME type matches the `expected` pattern (supports wildcards & `+suffix`).
pub fn matchType(expected: string, actual: string): bool {
    let exp = trim(expected);
    if (exp.len == 0) { return false; }
    return typeMatches(actual, exp);
}

/// npm `type-is.is(mediaType, types)` equivalent accepting a slice of candidate patterns.
/// If `types` is empty, returns the normalized `type/subtype` essence of `content_type`.
/// Otherwise returns the first matching candidate string, or `null` if none matched.
pub fn is(content_type: string, types: []const string): string? {
    let essence = extractEssence(content_type);
    if (essence == null) { return null; }
    if (types.len == 0) { return essence; }

    let i: usize = 0;
    while (i < types.len) {
        let pat = trim(types[i]);
        if (pat.len > 0 && typeMatches(content_type, pat)) {
            return pat;
        }
        i = i + 1;
    }
    return null;
}

/// Matches `content_type` against a comma-separated list of candidate patterns,
/// returning the first matching pattern string or `null` if none matched.
pub fn typeIs(content_type: string, candidates_csv: string): string? {
    let pos: usize = 0;
    while (pos <= candidates_csv.len) {
        let sep = findByte(candidates_csv, 44, pos) ?? candidates_csv.len;
        let pat = trim(candidates_csv[pos..sep]);
        if (pat.len > 0 && typeMatches(content_type, pat)) {
            return pat;
        }
        if (sep >= candidates_csv.len) { break; }
        pos = sep + 1;
    }
    return null;
}

/// Checks if a request has a body based on `transfer_encoding` or `content_length` headers.
pub fn hasBody(transfer_encoding: string, content_length: string): bool {
    if (trim(transfer_encoding).len > 0) { return true; }
    let cl = trim(content_length);
    if (cl.len == 0) { return false; }
    // Any non-zero digit implies non-empty body
    let i: usize = 0;
    while (i < cl.len) {
        if (cl[i] >= 49 && cl[i] <= 57) { return true; }
        i = i + 1;
    }
    return false;
}

/// Checks if a `RequestHeaders` struct indicates a non-empty request body.
pub fn requestHasBody(req: RequestHeaders): bool {
    return hasBody(req.transfer_encoding, req.content_length);
}

/// npm `type-is(req, types)` top-level equivalent.
/// Returns `null` when the request has no body or when `content_type` does not match `types`.
/// If `types` is empty and the request has a body, returns the parsed `type/subtype` essence.
pub fn typeis(req: RequestHeaders, types: []const string): string? {
    if (!requestHasBody(req)) { return null; }
    return is(req.content_type, types);
}

/// Convenience `typeis` overload accepting a comma-separated candidate list.
pub fn typeisCsv(req: RequestHeaders, candidates_csv: string): string? {
    if (!requestHasBody(req)) { return null; }
    let trimmed = trim(candidates_csv);
    if (trimmed.len == 0) { return extractEssence(req.content_type); }
    return typeIs(req.content_type, trimmed);
}

/// Convenience predicates for common HTTP media types.
pub fn isJson(content_type: string): bool {
    return typeMatches(content_type, "+json");
}

pub fn isText(content_type: string): bool {
    return typeMatches(content_type, "text/*");
}

pub fn isForm(content_type: string): bool {
    return typeMatches(content_type, "urlencoded");
}

pub fn isMultipart(content_type: string): bool {
    return typeMatches(content_type, "multipart");
}
