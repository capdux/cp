// @cap/semver-crate — Zero-heap Rust Cargo/crates.io `semver` v1 API.
// Clean-room MIT port of crates.io #42 `semver` (1.55B+ downloads);
// reuses `@cap/semver-range` with zero duplicated SemVer parsing logic.

import {
    SemVer, parseSemver, compareSemver, satisfies,
    incMajor, incMinor, incPatch, formatSemver
} from "../../semver-range/src/index.cp";

fn smcIsWs(b: u8): bool {
    return b == 32 || b == 9 || b == 10 || b == 13;
}

fn smcTrim(s: string): string {
    let start: usize = 0;
    while (start < s.len && smcIsWs(s[start])) { start = start + 1; }
    let end: usize = s.len;
    while (end > start && smcIsWs(s[end - 1])) { end = end - 1; }
    return s[start..end];
}

/// Parses a strict SemVer 2.0 version string (`semver::Version::parse`).
pub fn parseVersion(s: string): SemVer {
    return parseSemver(s);
}

/// Compares two `SemVer` values (-1, 0, 1) per SemVer 2.0 precedence rules.
pub fn compareVersions(a: SemVer, b: SemVer): i32 {
    return compareSemver(a, b);
}

/// Returns true if the version has a non-empty pre-release tag (e.g. `1.0.0-alpha.1`).
pub fn isPrerelease(v: SemVer): bool {
    return v.valid && v.pre_len > 0;
}

/// Evaluates a Cargo/Rust comma-separated `VersionReq` (e.g. `">=1.2.3, <1.8.0"`, `"^1.2"`, `"~1.2.3"`, `"1.2.*"`, `"*"`).
/// In Rust's `semver` crate, bare versions like `"1.2.3"` default to caret (`"^1.2.3"`).
pub fn matchesVersionReq(version_str: string, req: string): bool {
    let v = parseSemver(version_str);
    if (!v.valid) { return false; }
    let trimmed = smcTrim(req);
    if (trimmed.len == 0) { return false; }
    if (trimmed.len == 1 && trimmed[0] == 42) { return v.pre_len == 0; } // "*"

    let pos: usize = 0;
    let clause_count: usize = 0;
    while (pos <= trimmed.len) {
        let sep = pos;
        while (sep < trimmed.len && trimmed[sep] != 44) { sep = sep + 1; }
        let part = smcTrim(trimmed[pos..sep]);
        if (part.len > 0) {
            clause_count = clause_count + 1;
            // Cargo default: bare version starting with digit implies caret '^' unless wildcard '*'
            let has_star = false;
            let k: usize = 0;
            while (k < part.len) {
                if (part[k] == 42 || part[k] == 120 || part[k] == 88) { has_star = true; break; }
                k = k + 1;
            }
            if (!has_star && part[0] >= 48 && part[0] <= 57) {
                let buf: [64]u8 = undefined;
                if (part.len + 1 <= 64) {
                    buf[0] = 94; // '^'
                    let i: usize = 0;
                    while (i < part.len) { buf[i + 1] = part[i]; i = i + 1; }
                    if (!satisfies(version_str, buf[0..part.len + 1] as string)) {
                        return false;
                    }
                } else {
                    return false;
                }
            } else {
                if (!satisfies(version_str, part)) {
                    return false;
                }
            }
        }
        if (sep >= trimmed.len) { break; }
        pos = sep + 1;
    }
    return clause_count > 0;
}

/// Checks if `ver` is strictly compatible with `target` under Cargo caret semantics (`^target`).
pub fn isCaretCompatible(ver: string, target: string): bool {
    return matchesVersionReq(ver, target);
}

/// Formats `SemVer` into `out`, returning bytes written.
pub fn formatVersion(v: SemVer, out: []u8): usize {
    return formatSemver(v, out);
}

/// Increments major, minor, or patch version cleanly.
pub fn bumpMajor(v: SemVer): SemVer { return incMajor(v); }
pub fn bumpMinor(v: SemVer): SemVer { return incMinor(v); }
pub fn bumpPatch(v: SemVer): SemVer { return incPatch(v); }
