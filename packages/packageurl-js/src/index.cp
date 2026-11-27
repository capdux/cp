// @cap/purl — Zero-allocation Package URL (PURL) Parser, Validator & Canonical Formatter
// Clean-room MIT implementation in pure CP (pkg:type/namespace/name@version?qualifiers#subpath).

import { isAlpha, isDigit, toLowerByte, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub struct Purl {
    pub pkg_type: string,
    pub namespace: string,
    pub name: string,
    pub version: string,
    pub qualifiers: string,
    pub subpath: string,
    pub valid: bool,
}

fn invalidPurl() -> Purl {
    return Purl {
        pkg_type: "",
        namespace: "",
        name: "",
        version: "",
        qualifiers: "",
        subpath: "",
        valid: false,
    };
}

fn purlIsSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

/// Validates a PURL `type`: must start with an ASCII letter followed by letters, digits, `.`, `+`, or `-`.
pub fn isValidType(pkg_type: string) -> bool {
    if (pkg_type.len == 0 || !isAlpha(pkg_type[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < pkg_type.len) {
        const ch = pkg_type[i];
        if (!isAlpha(ch) && !isDigit(ch) && ch != 46 && ch != 43 && ch != 45) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimSlashes(s: string) -> string {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && s[a] == 47) {
        a += 1;
    }
    while (b > a && s[b - 1] == 47) {
        b -= 1;
    }
    return s[a..b];
}

/// Parses a Package URL (`pkg:type/namespace/name@version?qualifiers#subpath`) with zero heap allocation.
pub fn parsePurl(input: string) -> Purl {
    let a: usize = 0;
    let b: usize = input.len;
    while (a < b && purlIsSpace(input[a])) {
        a += 1;
    }
    while (b > a && purlIsSpace(input[b - 1])) {
        b -= 1;
    }
    if (b - a < 5) {
        return invalidPurl();
    }

    // Must start with "pkg:" (case-insensitive)
    if (!eqIgnoreCase(input[a..(a + 4)], "pkg:")) {
        return invalidPurl();
    }

    let cur_start = a + 4;
    let cur_end = b;
    // Skip optional leading '/' after "pkg:"
    while (cur_start < cur_end && input[cur_start] == 47) {
        cur_start += 1;
    }

    // 1. Strip optional "#subpath"
    let subpath: string = "";
    let hash_idx = cur_start;
    while (hash_idx < cur_end && input[hash_idx] != 35) {
        hash_idx += 1;
    }
    if (hash_idx < cur_end) {
        subpath = trimSlashes(input[hash_idx + 1..cur_end]);
        cur_end = hash_idx;
    }

    // 2. Strip optional "?qualifiers"
    let qualifiers: string = "";
    let q_idx = cur_start;
    while (q_idx < cur_end && input[q_idx] != 63) {
        q_idx += 1;
    }
    if (q_idx < cur_end) {
        qualifiers = input[q_idx + 1..cur_end];
        cur_end = q_idx;
    }

    // 3. Extract "type" up to first '/'
    let slash_idx = cur_start;
    while (slash_idx < cur_end && input[slash_idx] != 47) {
        slash_idx += 1;
    }
    if (slash_idx >= cur_end) {
        return invalidPurl();
    }
    const pkg_type = input[cur_start..slash_idx];
    if (!isValidType(pkg_type)) {
        return invalidPurl();
    }

    let rest_start = slash_idx + 1;
    let rest_end = cur_end;

    // 4. Strip optional "@version" from right
    let version: string = "";
    let at_idx = rest_end;
    let scan = rest_end;
    while (scan > rest_start) {
        scan -= 1;
        if (input[scan] == 64) {
            at_idx = scan;
            break;
        }
    }
    if (at_idx < rest_end) {
        version = input[at_idx + 1..rest_end];
        rest_end = at_idx;
    }

    // 5. Split optional "namespace/" and final "name"
    const path_part = trimSlashes(input[rest_start..rest_end]);
    if (path_part.len == 0) {
        return invalidPurl();
    }

    let last_slash = path_part.len;
    let ps = path_part.len;
    while (ps > 0) {
        ps -= 1;
        if (path_part[ps] == 47) {
            last_slash = ps;
            break;
        }
    }

    let namespace: string = "";
    let name: string = path_part;
    if (last_slash < path_part.len) {
        namespace = trimSlashes(path_part[0..last_slash]);
        name = path_part[last_slash + 1..path_part.len];
    }

    if (name.len == 0) {
        return invalidPurl();
    }

    return Purl {
        pkg_type: pkg_type,
        namespace: namespace,
        name: name,
        version: version,
        qualifiers: qualifiers,
        subpath: subpath,
        valid: true,
    };
}

/// Zero-copy lookup of a qualifier value by `key` (case-insensitive) in `purl.qualifiers`.
/// Returns `""` if not found.
pub fn getQualifier(purl: Purl, key: string) -> string {
    if (purl.qualifiers.len == 0 || key.len == 0) {
        return "";
    }
    const q = purl.qualifiers;
    let pos: usize = 0;
    const n = q.len;
    while (pos < n) {
        const start = pos;
        while (pos < n && q[pos] != 38) {
            pos += 1; // '&'
        }
        const pair = q[start..pos];
        if (pos < n && q[pos] == 38) {
            pos += 1;
        }
        let eq: usize = 0;
        while (eq < pair.len && pair[eq] != 61) {
            eq += 1;
        }
        if (eq < pair.len && eqIgnoreCase(pair[0..eq], key)) {
            return pair[eq + 1..pair.len];
        }
    }
    return "";
}

fn pushSlice(dst: []u8, pos: usize, src: string) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    return writeStr(src, dst, pos) as i64;
}

fn pushLowerSlice(dst: []u8, pos: usize, src: string) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = toLowerByte(src[i]);
        i += 1;
    }
    return (pos + src.len) as i64;
}

/// Serializes a `Purl` into canonical lowercase-type form:
/// `pkg:<type>/[namespace/]<name>[@version][?qualifiers][#subpath]`.
/// Returns bytes written, or `0` if invalid or `out` is too small.
pub fn formatPurl(p: Purl, out: []u8) -> usize {
    if (!isValidType(p.pkg_type) || p.name.len == 0) {
        return 0;
    }
    let w = pushSlice(out, 0, "pkg:");
    if (w < 0) { return 0; }
    w = pushLowerSlice(out, w as usize, p.pkg_type);
    if (w < 0) { return 0; }
    w = pushSlice(out, w as usize, "/");
    if (w < 0) { return 0; }

    if (p.namespace.len > 0) {
        w = pushSlice(out, w as usize, p.namespace);
        if (w < 0) { return 0; }
        w = pushSlice(out, w as usize, "/");
        if (w < 0) { return 0; }
    }

    w = pushSlice(out, w as usize, p.name);
    if (w < 0) { return 0; }

    if (p.version.len > 0) {
        w = pushSlice(out, w as usize, "@");
        if (w < 0) { return 0; }
        w = pushSlice(out, w as usize, p.version);
        if (w < 0) { return 0; }
    }
    if (p.qualifiers.len > 0) {
        w = pushSlice(out, w as usize, "?");
        if (w < 0) { return 0; }
        w = pushSlice(out, w as usize, p.qualifiers);
        if (w < 0) { return 0; }
    }
    if (p.subpath.len > 0) {
        w = pushSlice(out, w as usize, "#");
        if (w < 0) { return 0; }
        w = pushSlice(out, w as usize, p.subpath);
        if (w < 0) { return 0; }
    }

    return w as usize;
}
