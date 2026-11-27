// packages/@cap/normalize-package-data/src/index.cp
// Zero-allocation package metadata normalizer and validator for CP.
// Clean-room MIT pure CP implementation reusing `@cap/validate-npm-package-name` and `@cap/validate-npm-package-license`.

import { PackageNameValidation, validatePackageName } from "../../validate-npm-package-name/src/index.cp";
import { LicenseValidation, validateLicense } from "../../validate-npm-package-license/src/index.cp";

/// Raw package metadata fields to normalize and validate.
pub struct PackageMetadata {
    pub name: string,
    pub version: string,
    pub license: string,
    pub repository: string,
    pub main_entry: string,
    pub private_pkg: bool,
}

/// Aggregated validation and warning report returned by `normalizePackageData`.
pub struct NormalizedPackageResult {
    pub valid: bool,
    pub name_valid: bool,
    pub license_valid: bool,
    pub has_warnings: bool,
    pub warning_count: usize,
}

fn npdIsSpace(ch: u8): bool {
    return ch == (' ' as u8)
        || ch == ('\t' as u8)
        || ch == ('\r' as u8)
        || ch == ('\n' as u8);
}

fn npdTrim(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && npdIsSpace(s[start])) {
        start += 1;
    }
    while (end > start && npdIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn npdStartsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn npdEndsWith(s: string, suffix: string): bool {
    if (s.len < suffix.len) {
        return false;
    }
    let offset: usize = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (s[offset + i] != suffix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn npdPush(out: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn npdIsOwnerRepoShorthand(s: string): bool {
    let slash_count: usize = 0;
    let slash_pos: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        let ch: u8 = s[i];
        if (npdIsSpace(ch) || ch == (':' as u8)) {
            return false;
        }
        if (ch == ('/' as u8)) {
            slash_count += 1;
            slash_pos = i;
        }
        i += 1;
    }
    return slash_count == 1 && slash_pos > 0 && slash_pos + 1 < s.len;
}

fn npdWriteHostedUrl(base_url: string, slug: string, out: []u8): usize {
    let p: usize = 0;
    p = npdPush(out, p, base_url);
    p = npdPush(out, p, slug);
    if (!npdEndsWith(slug, ".git")) {
        p = npdPush(out, p, ".git");
    }
    return p;
}

/// Expand repository shorthands (`owner/repo`, `github:owner/repo`, `gitlab:owner/repo`, `bitbucket:owner/repo`)
/// into canonical HTTPS Git URLs. Existing `http://` or `https://` URLs are copied as-is.
pub fn expandRepoShorthand(repo: string, out: []u8): usize {
    let s: string = npdTrim(repo);
    if (s.len == 0) {
        return 0;
    }

    if (npdStartsWith(s, "https://") || npdStartsWith(s, "http://")) {
        return npdPush(out, 0, s);
    }

    if (npdStartsWith(s, "github:")) {
        return npdWriteHostedUrl("https://github.com/", s[7..s.len], out);
    }
    if (npdStartsWith(s, "gitlab:")) {
        return npdWriteHostedUrl("https://gitlab.com/", s[7..s.len], out);
    }
    if (npdStartsWith(s, "bitbucket:")) {
        return npdWriteHostedUrl("https://bitbucket.org/", s[10..s.len], out);
    }

    if (npdIsOwnerRepoShorthand(s)) {
        return npdWriteHostedUrl("https://github.com/", s, out);
    }

    return npdPush(out, 0, s);
}

/// Normalize a package entry path by stripping leading `./`, leading `/`, and trailing `/`.
pub fn normalizeEntryPath(entry: string): string {
    let s: string = npdTrim(entry);
    while (s.len > 0) {
        if (s.len >= 2 && s[0] == ('.' as u8) && s[1] == ('/' as u8)) {
            s = s[2..s.len];
            continue;
        }
        if (s[0] == ('/' as u8)) {
            s = s[1..s.len];
            continue;
        }
        break;
    }
    let end: usize = s.len;
    while (end > 0 && s[end - 1] == ('/' as u8)) {
        end -= 1;
    }
    return s[0..end];
}

/// Clean a version string by trimming whitespace and leading `v`/`V`/`=` prefixes.
pub fn cleanVersionString(ver: string): string {
    let s: string = npdTrim(ver);
    while (s.len > 0) {
        let ch: u8 = s[0];
        if (ch == ('v' as u8) || ch == ('V' as u8) || ch == ('=' as u8) || npdIsSpace(ch)) {
            s = s[1..s.len];
            continue;
        }
        break;
    }
    return npdTrim(s);
}

/// Validate and normalize package metadata using `@cap/validate-npm-package-name` and `@cap/validate-npm-package-license`.
pub fn normalizePackageData(meta: PackageMetadata): NormalizedPackageResult {
    let name_res: PackageNameValidation = validatePackageName(meta.name);
    let lic_res: LicenseValidation = validateLicense(meta.license);
    let cleaned_ver: string = cleanVersionString(meta.version);

    let warnings: usize = 0;

    let name_valid: bool = name_res.valid_for_new;
    if (name_res.warning_code != 0) {
        warnings += 1;
    }

    let license_valid: bool = false;
    if (meta.license.len == 0) {
        warnings += 1;
        if (meta.private_pkg) {
            license_valid = true;
        }
    } else {
        license_valid = lic_res.valid_for_new_packages;
        if (!lic_res.valid_for_new_packages && lic_res.valid_for_old_packages) {
            warnings += 1;
        }
    }

    if (cleaned_ver.len == 0) {
        warnings += 1;
    }

    if (!meta.private_pkg && npdTrim(meta.repository).len == 0) {
        warnings += 1;
    }

    let ver_ok: bool = cleaned_ver.len > 0 || meta.private_pkg;
    let valid: bool = name_valid && license_valid && ver_ok;

    return NormalizedPackageResult {
        valid: valid,
        name_valid: name_valid,
        license_valid: license_valid,
        has_warnings: warnings > 0,
        warning_count: warnings,
    };
}
