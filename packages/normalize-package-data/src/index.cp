// packages/@cap/normalize-package-data/src/index.cp
// Zero-allocation package metadata normalizer and validator for CP.
// Clean-room MIT pure CP implementation reusing `@cap/validate-npm-package-name` and `@cap/validate-npm-package-license`.

import { PackageNameValidation, validatePackageName } from "../../validate-npm-package-name/src/index.cp";
import { LicenseValidation, validateLicense } from "../../validate-npm-package-license/src/index.cp";
import { isSpace, trim, startsWith, endsWith } from "cap:ascii";
import { writeStr } from "cap:fmt";

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

fn npdIsOwnerRepoShorthand(s: string): bool {
    let slash_count: usize = 0;
    let slash_pos: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        let ch: u8 = s[i];
        if (isSpace(ch) || ch == (':' as u8)) {
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
    let p: usize = writeStr(base_url, out, 0);
    p = writeStr(slug, out, p);
    if (!endsWith(slug, ".git")) {
        p = writeStr(".git", out, p);
    }
    return p;
}

/// Expand repository shorthands (`owner/repo`, `github:owner/repo`, `gitlab:owner/repo`, `bitbucket:owner/repo`)
/// into canonical HTTPS Git URLs. Existing `http://` or `https://` URLs are copied as-is.
pub fn expandRepoShorthand(repo: string, out: []u8): usize {
    let s: string = trim(repo);
    if (s.len == 0) {
        return 0;
    }

    if (startsWith(s, "https://") || startsWith(s, "http://")) {
        return writeStr(s, out, 0);
    }

    if (startsWith(s, "github:")) {
        return npdWriteHostedUrl("https://github.com/", s[7..s.len], out);
    }
    if (startsWith(s, "gitlab:")) {
        return npdWriteHostedUrl("https://gitlab.com/", s[7..s.len], out);
    }
    if (startsWith(s, "bitbucket:")) {
        return npdWriteHostedUrl("https://bitbucket.org/", s[10..s.len], out);
    }

    if (npdIsOwnerRepoShorthand(s)) {
        return npdWriteHostedUrl("https://github.com/", s, out);
    }

    return writeStr(s, out, 0);
}

/// Normalize a package entry path by stripping leading `./`, leading `/`, and trailing `/`.
pub fn normalizeEntryPath(entry: string): string {
    let s: string = trim(entry);
    while (s.len > 0) {
        if (startsWith(s, "./")) {
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
    let s: string = trim(ver);
    while (s.len > 0) {
        let ch: u8 = s[0];
        if (ch == ('v' as u8) || ch == ('V' as u8) || ch == ('=' as u8) || isSpace(ch)) {
            s = s[1..s.len];
            continue;
        }
        break;
    }
    return trim(s);
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

    if (!meta.private_pkg && trim(meta.repository).len == 0) {
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
