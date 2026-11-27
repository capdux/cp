// @cap/type-is — Zero-heap HTTP Content-Type matcher modeled on npm `type-is` v1.6.
// Clean-room MIT implementation; reuses `@cap/mime-types` and `@cap/unicase`.

import { trim, findByte } from "cap:ascii";
import { lookup, parseMimeType } from "../../mime-types/src/index.cp";
import { eqAscii, endsWithIgnoreCase } from "../../unicase/src/index.cp";

/// Normalizes a shorthand type pattern (e.g., `"json"` -> `"application/json"`,
/// `"urlencoded"` -> `"application/x-www-form-urlencoded"`, `"multipart"` -> `"multipart/*"`).
pub fn normalizeTypePattern(pattern: string): string {
    let t = trim(pattern);
    if (t.len == 0) { return ""; }
    if (eqAscii(t, "urlencoded")) { return "application/x-www-form-urlencoded"; }
    if (eqAscii(t, "multipart")) { return "multipart/*"; }
    if (t[0] == 43) { return t; } // "+json", "+xml" structured suffix
    if (findByte(t, 47, 0) != null) { return t; } // already "type/subtype"
    return lookup(t) ?? t;
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
        return endsWithIgnoreCase(parsed.subtype, norm) || eqAscii(parsed.subtype, norm[1..norm.len]);
    }

    let slash_opt = findByte(norm, 47, 0);
    if (slash_opt == null) {
        return eqAscii(parsed.essence, norm);
    }
    let slash = slash_opt.?;
    let pat_type = norm[0..slash];
    let pat_sub = norm[slash + 1..norm.len];

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
