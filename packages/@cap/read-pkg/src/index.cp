// packages/@cap/read-pkg/src/index.cp
// Zero-allocation package.json parser and normalizer for CP.
// Clean-room MIT pure CP implementation reusing `@cap/normalize-package-data`.

import {
    PackageMetadata,
    NormalizedPackageResult,
    normalizePackageData,
    normalizeEntryPath,
    cleanVersionString,
} from "../../normalize-package-data/src/index.cp";

/// Parsed package metadata paired with optional normalization/validation result.
pub struct ReadPkgResult {
    pub meta: PackageMetadata,
    pub normalized: NormalizedPackageResult,
    pub parsed: bool,
}

fn rpkgIsSpace(ch: u8): bool {
    return ch == (' ' as u8)
        || ch == ('\t' as u8)
        || ch == ('\r' as u8)
        || ch == ('\n' as u8);
}

fn rpkgStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn rpkgStartsWithAt(s: string, pos: usize, kw: string): bool {
    if (pos + kw.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < kw.len) {
        if (s[pos + i] != kw[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn rpkgIsValueBoundary(s: string, pos: usize): bool {
    if (pos >= s.len) {
        return true;
    }
    let ch: u8 = s[pos];
    return rpkgIsSpace(ch)
        || ch == (',' as u8)
        || ch == ('}' as u8)
        || ch == (']' as u8);
}

fn rpkgSkipWs(s: string, pos: usize): usize {
    let i: usize = pos;
    while (i < s.len && rpkgIsSpace(s[i])) {
        i += 1;
    }
    return i;
}

/// Given `s[open_quote] == '"'`, returns the index of the closing `"` or `null` if unclosed.
fn rpkgFindStringEnd(s: string, open_quote: usize): usize? {
    if (open_quote >= s.len || s[open_quote] != ('"' as u8)) {
        return null;
    }
    let i: usize = open_quote + 1;
    while (i < s.len) {
        let ch: u8 = s[i];
        if (ch == ('\\' as u8)) {
            if (i + 1 >= s.len) {
                return null;
            }
            i += 2;
            continue;
        }
        if (ch == ('"' as u8)) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// Skip a JSON value starting at `pos`. Returns the index immediately after the value, or `null` on syntax error.
fn rpkgSkipValue(s: string, pos: usize): usize? {
    let p: usize = rpkgSkipWs(s, pos);
    if (p >= s.len) {
        return null;
    }
    let ch: u8 = s[p];

    if (ch == ('"' as u8)) {
        let end_opt: usize? = rpkgFindStringEnd(s, p);
        if (end_opt == null) {
            return null;
        }
        return (end_opt ?? 0) + 1;
    }

    if (ch == ('{' as u8) || ch == ('[' as u8)) {
        let depth: usize = 1;
        let i: usize = p + 1;
        while (i < s.len && depth > 0) {
            let c: u8 = s[i];
            if (c == ('"' as u8)) {
                let str_end: usize? = rpkgFindStringEnd(s, i);
                if (str_end == null) {
                    return null;
                }
                i = (str_end ?? 0) + 1;
                continue;
            }
            if (c == ('{' as u8) || c == ('[' as u8)) {
                depth += 1;
            } else if (c == ('}' as u8) || c == (']' as u8)) {
                depth -= 1;
            }
            i += 1;
        }
        if (depth != 0) {
            return null;
        }
        return i;
    }

    // Primitive literal: number, true, false, null.
    let i: usize = p;
    while (i < s.len && !rpkgIsValueBoundary(s, i)) {
        i += 1;
    }
    if (i == p) {
        return null;
    }
    return i;
}

/// Validate that `json_text` is a syntactically balanced top-level JSON object `{ ... }`.
fn rpkgValidateTopObject(json_text: string): bool {
    let p: usize = rpkgSkipWs(json_text, 0);
    if (p >= json_text.len || json_text[p] != ('{' as u8)) {
        return false;
    }
    p = rpkgSkipWs(json_text, p + 1);
    if (p < json_text.len && json_text[p] == ('}' as u8)) {
        let tail: usize = rpkgSkipWs(json_text, p + 1);
        return tail == json_text.len;
    }

    while (p < json_text.len) {
        let key_end_opt: usize? = rpkgFindStringEnd(json_text, p);
        if (key_end_opt == null) {
            return false;
        }
        p = rpkgSkipWs(json_text, (key_end_opt ?? 0) + 1);
        if (p >= json_text.len || json_text[p] != (':' as u8)) {
            return false;
        }
        let val_end_opt: usize? = rpkgSkipValue(json_text, p + 1);
        if (val_end_opt == null) {
            return false;
        }
        p = rpkgSkipWs(json_text, val_end_opt ?? 0);
        if (p >= json_text.len) {
            return false;
        }
        if (json_text[p] == (',' as u8)) {
            p = rpkgSkipWs(json_text, p + 1);
            continue;
        }
        if (json_text[p] == ('}' as u8)) {
            let tail: usize = rpkgSkipWs(json_text, p + 1);
            return tail == json_text.len;
        }
        return false;
    }
    return false;
}

/// Find the byte offset of the value for top-level `key` in a JSON object.
fn rpkgFindTopLevelValue(json_text: string, key: string): usize? {
    let p: usize = rpkgSkipWs(json_text, 0);
    if (p >= json_text.len || json_text[p] != ('{' as u8)) {
        return null;
    }
    p = rpkgSkipWs(json_text, p + 1);
    if (p < json_text.len && json_text[p] == ('}' as u8)) {
        return null;
    }

    while (p < json_text.len) {
        let key_end_opt: usize? = rpkgFindStringEnd(json_text, p);
        if (key_end_opt == null) {
            return null;
        }
        let key_end: usize = key_end_opt ?? 0;
        let cur_key: string = json_text[p + 1 .. key_end];

        p = rpkgSkipWs(json_text, key_end + 1);
        if (p >= json_text.len || json_text[p] != (':' as u8)) {
            return null;
        }
        let val_pos: usize = rpkgSkipWs(json_text, p + 1);
        if (rpkgStrEq(cur_key, key)) {
            return val_pos;
        }

        let val_end_opt: usize? = rpkgSkipValue(json_text, val_pos);
        if (val_end_opt == null) {
            return null;
        }
        p = rpkgSkipWs(json_text, val_end_opt ?? 0);
        if (p < json_text.len && json_text[p] == (',' as u8)) {
            p = rpkgSkipWs(json_text, p + 1);
            continue;
        }
        return null;
    }
    return null;
}

/// Extract a top-level JSON string field by `key`, returning `null` if absent or non-string.
pub fn extractJsonStringField(json_text: string, key: string): string? {
    let val_opt: usize? = rpkgFindTopLevelValue(json_text, key);
    if (val_opt == null) {
        return null;
    }
    let val_pos: usize = val_opt ?? 0;
    if (val_pos >= json_text.len || json_text[val_pos] != ('"' as u8)) {
        return null;
    }
    let end_opt: usize? = rpkgFindStringEnd(json_text, val_pos);
    if (end_opt == null) {
        return null;
    }
    let end_pos: usize = end_opt ?? 0;
    return json_text[val_pos + 1 .. end_pos];
}

/// Extract a top-level JSON boolean field by `key`, returning `null` if absent or non-boolean.
pub fn extractJsonBoolField(json_text: string, key: string): bool? {
    let val_opt: usize? = rpkgFindTopLevelValue(json_text, key);
    if (val_opt == null) {
        return null;
    }
    let val_pos: usize = val_opt ?? 0;
    if (rpkgStartsWithAt(json_text, val_pos, "true") && rpkgIsValueBoundary(json_text, val_pos + 4)) {
        return true;
    }
    if (rpkgStartsWithAt(json_text, val_pos, "false") && rpkgIsValueBoundary(json_text, val_pos + 5)) {
        return false;
    }
    return null;
}

/// Extract `"repository"` as either a top-level string or `{ "url": "..." }` object.
fn rpkgExtractRepository(json_text: string): string {
    let direct: string? = extractJsonStringField(json_text, "repository");
    if (direct != null) {
        return direct ?? "";
    }
    let val_opt: usize? = rpkgFindTopLevelValue(json_text, "repository");
    if (val_opt == null) {
        return "";
    }
    let val_pos: usize = val_opt ?? 0;
    if (val_pos < json_text.len && json_text[val_pos] == ('{' as u8)) {
        let end_opt: usize? = rpkgSkipValue(json_text, val_pos);
        if (end_opt != null) {
            let repo_obj: string = json_text[val_pos .. (end_opt ?? val_pos)];
            let url_opt: string? = extractJsonStringField(repo_obj, "url");
            if (url_opt != null) {
                return url_opt ?? "";
            }
        }
    }
    return "";
}

/// Parse top-level metadata from `package.json` / `cap.json` text with zero heap allocations.
/// When `normalize` is true, cleans `version` and `main_entry` and runs `normalizePackageData`.
pub fn parsePackageJson(json_text: string, normalize: bool): ReadPkgResult {
    let empty_meta: PackageMetadata = PackageMetadata {
        name: "",
        version: "",
        license: "",
        repository: "",
        main_entry: "",
        private_pkg: false,
    };
    let empty_norm: NormalizedPackageResult = NormalizedPackageResult {
        valid: false,
        name_valid: false,
        license_valid: false,
        has_warnings: false,
        warning_count: 0,
    };

    if (!rpkgValidateTopObject(json_text)) {
        return ReadPkgResult {
            meta: empty_meta,
            normalized: empty_norm,
            parsed: false,
        };
    }

    let name_val: string = extractJsonStringField(json_text, "name") ?? "";
    let ver_val: string = extractJsonStringField(json_text, "version") ?? "";
    let lic_val: string = extractJsonStringField(json_text, "license") ?? "";
    let repo_val: string = rpkgExtractRepository(json_text);
    let main_val: string = extractJsonStringField(json_text, "main") ?? "";
    let priv_val: bool = extractJsonBoolField(json_text, "private") ?? false;

    if (normalize) {
        ver_val = cleanVersionString(ver_val);
        main_val = normalizeEntryPath(main_val);
    }

    let meta: PackageMetadata = PackageMetadata {
        name: name_val,
        version: ver_val,
        license: lic_val,
        repository: repo_val,
        main_entry: main_val,
        private_pkg: priv_val,
    };

    let norm_res: NormalizedPackageResult = empty_norm;
    if (normalize) {
        norm_res = normalizePackageData(meta);
    }

    return ReadPkgResult {
        meta: meta,
        normalized: norm_res,
        parsed: true,
    };
}
