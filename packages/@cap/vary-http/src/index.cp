// @cap/vary-http — Zero-heap RFC 7231 HTTP `Vary` header builder & matcher.
// Clean-room MIT port of npm `vary` v1.1; reuses `@cap/unicase` for case-insensitive field matching.

import { eqAscii } from "../../unicase/src/index.cp";

fn vhIsWs(b: u8): bool {
    return b == 32 || b == 9;
}

fn vhTrim(s: string): string {
    let start: usize = 0;
    while (start < s.len && vhIsWs(s[start])) { start = start + 1; }
    let end: usize = s.len;
    while (end > start && vhIsWs(s[end - 1])) { end = end - 1; }
    return s[start..end];
}

fn vhPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

/// Validates an RFC 7230 HTTP header field-name token (`tchar+`) or `"*"`.
pub fn isValidFieldName(name: string): bool {
    if (name.len == 0) { return false; }
    if (name.len == 1 && name[0] == 42) { return true; } // '*'
    let i: usize = 0;
    while (i < name.len) {
        let b = name[i];
        let ok = (b >= 48 && b <= 57) || (b >= 65 && b <= 90) || (b >= 97 && b <= 122) ||
            b == 33 || b == 35 || b == 36 || b == 37 || b == 38 || b == 39 ||
            b == 42 || b == 43 || b == 45 || b == 46 || b == 94 || b == 95 ||
            b == 96 || b == 124 || b == 126;
        if (!ok) { return false; }
        i = i + 1;
    }
    return true;
}

/// Checks whether `header` already contains `field` (case-insensitive) or `"*"`.
pub fn hasVaryField(header: string, field: string): bool {
    let target = vhTrim(field);
    if (target.len == 0) { return false; }
    let pos: usize = 0;
    while (pos <= header.len) {
        let sep = pos;
        while (sep < header.len && header[sep] != 44) { sep = sep + 1; }
        let tok = vhTrim(header[pos..sep]);
        if (tok.len > 0) {
            if (tok.len == 1 && tok[0] == 42) { return true; }
            if (eqAscii(tok, target)) { return true; }
        }
        if (sep >= header.len) { break; }
        pos = sep + 1;
    }
    return false;
}

/// Counts distinct header field tokens in a `Vary` header value.
pub fn countVaryFields(header: string): usize {
    let count: usize = 0;
    let pos: usize = 0;
    while (pos <= header.len) {
        let sep = pos;
        while (sep < header.len && header[sep] != 44) { sep = sep + 1; }
        let tok = vhTrim(header[pos..sep]);
        if (tok.len > 0) {
            if (tok.len == 1 && tok[0] == 42) { return 1; }
            count = count + 1;
        }
        if (sep >= header.len) { break; }
        pos = sep + 1;
    }
    return count;
}

/// Appends one or more comma-separated `fields` to an existing `Vary` `header`,
/// deduplicating case-insensitively and collapsing to `"*"` if either contains `"*"`.
/// Writes result into `out` and returns bytes written.
pub fn appendVary(header: string, fields: string, out: []u8): usize {
    // Fast path: if either header or fields contains "*", result is always "*"
    if (hasVaryField(header, "*") || hasVaryField(fields, "*")) {
        return vhPush(out, 0, "*");
    }

    let p: usize = 0;
    let added: [32]string = undefined;
    let added_cnt: usize = 0;

    // 1. Copy existing valid tokens from `header` (deduplicated)
    let pos: usize = 0;
    while (pos <= header.len) {
        let sep = pos;
        while (sep < header.len && header[sep] != 44) { sep = sep + 1; }
        let tok = vhTrim(header[pos..sep]);
        if (tok.len > 0 && isValidFieldName(tok)) {
            let dup = false;
            let k: usize = 0;
            while (k < added_cnt) {
                if (eqAscii(added[k], tok)) { dup = true; break; }
                k = k + 1;
            }
            if (!dup) {
                if (p > 0) { p = vhPush(out, p, ", "); }
                p = vhPush(out, p, tok);
                if (added_cnt < 32) {
                    added[added_cnt] = tok;
                    added_cnt = added_cnt + 1;
                }
            }
        }
        if (sep >= header.len) { break; }
        pos = sep + 1;
    }

    // 2. Append new tokens from `fields` if not already present
    let fpos: usize = 0;
    while (fpos <= fields.len) {
        let sep = fpos;
        while (sep < fields.len && fields[sep] != 44) { sep = sep + 1; }
        let tok = vhTrim(fields[fpos..sep]);
        if (tok.len > 0 && isValidFieldName(tok)) {
            let dup = false;
            let k: usize = 0;
            while (k < added_cnt) {
                if (eqAscii(added[k], tok)) { dup = true; break; }
                k = k + 1;
            }
            if (!dup) {
                if (p > 0) { p = vhPush(out, p, ", "); }
                p = vhPush(out, p, tok);
                if (added_cnt < 32) {
                    added[added_cnt] = tok;
                    added_cnt = added_cnt + 1;
                }
            }
        }
        if (sep >= fields.len) { break; }
        fpos = sep + 1;
    }

    return p;
}
