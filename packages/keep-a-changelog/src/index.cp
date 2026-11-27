// @cap/keep-changelog — Zero-allocation Keep a Changelog 1.1.0 parser & generator for CP
// Clean-room MIT implementation inspired by npm `keep-a-changelog`, `changelog-parser`, and Rust `parse-changelog`.
//
// Features:
// - Parses `## [Unreleased]`, `## [1.2.0] - 2026-11-26`, `## [1.1.0] - 2026-10-01 [YANKED]`,
//   and linked headings `## [1.2.0](https://...) - 2026-11-26`.
// - Classifies `### Added`, `### Changed`, `### Deprecated`, `### Removed`, `### Fixed`, `### Security`
//   and counts top-level `- ` / `* ` bullet entries per release.
// - Zero heap allocation: all `ReleaseInfo` strings slice directly into the input markdown.
// - Provides `parseChangelog`, `findRelease`, `latestRelease`, `classifySection`, and `formatReleaseHeader`.

import { toLowerByte, trim, eqIgnoreCase, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";

const MAX_RELEASES: usize = 32;

pub enum ChangeKind: u8 {
    Added,
    Changed,
    Deprecated,
    Removed,
    Fixed,
    Security,
    Other,
}

pub struct ReleaseInfo {
    pub version: string,
    pub date: string,
    pub is_unreleased: bool,
    pub is_yanked: bool,
    pub body: string,
    pub added_count: usize,
    pub changed_count: usize,
    pub fixed_count: usize,
    pub security_count: usize,

    pub static fn empty() -> ReleaseInfo {
        return ReleaseInfo{
            version: "",
            date: "",
            is_unreleased: false,
            is_yanked: false,
            body: "",
            added_count: 0,
            changed_count: 0,
            fixed_count: 0,
            security_count: 0,
        };
    }
}

pub struct ParsedChangelog {
    pub releases: [MAX_RELEASES]ReleaseInfo,
    pub count: usize,

    pub static fn empty() -> ParsedChangelog {
        let arr: [MAX_RELEASES]ReleaseInfo = undefined;
        return ParsedChangelog{ releases: arr, count: 0 };
    }

    pub fn releaseAt(i: usize) -> ReleaseInfo {
        if (i >= _.count) {
            return ReleaseInfo.empty();
        }
        return _.releases[i];
    }
}

fn containsIgnoreCase(hay: string, needle_lower: string) -> bool {
    if (needle_lower.len == 0 || needle_lower.len > hay.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle_lower.len <= hay.len) {
        let j: usize = 0;
        let ok = true;
        while (j < needle_lower.len) {
            if (toLowerByte(hay[i + j]) != needle_lower[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn isRefLinkDef(trimmed: string) -> bool {
    if (trimmed.len < 5 || trimmed[0] != '[') {
        return false;
    }
    let i: usize = 1;
    while (i < trimmed.len && trimmed[i] != ']') {
        if (trimmed[i] == '[') {
            return false;
        }
        i += 1;
    }
    if (i <= 1 || i + 1 >= trimmed.len) {
        return false;
    }
    return trimmed[i] == ']' && trimmed[i + 1] == ':';
}

/// Classifies a Keep a Changelog `### <Section>` heading into a `ChangeKind`.
pub fn classifySection(heading: string) -> ChangeKind {
    const s = trim(heading);
    if (eqIgnoreCase(s, "added")) { return ChangeKind.Added; }
    if (eqIgnoreCase(s, "changed")) { return ChangeKind.Changed; }
    if (eqIgnoreCase(s, "deprecated")) { return ChangeKind.Deprecated; }
    if (eqIgnoreCase(s, "removed")) { return ChangeKind.Removed; }
    if (eqIgnoreCase(s, "fixed")) { return ChangeKind.Fixed; }
    if (eqIgnoreCase(s, "security")) { return ChangeKind.Security; }
    return ChangeKind.Other;
}

fn parseReleaseHeading(rest_raw: string) -> ReleaseInfo {
    let info = ReleaseInfo.empty();
    const rest = trim(rest_raw);
    if (rest.len == 0) {
        return info;
    }

    let after: string = rest[rest.len..rest.len];
    if (rest[0] == '[') {
        let rb: usize = 1;
        while (rb < rest.len && rest[rb] != ']') {
            rb += 1;
        }
        if (rb < rest.len) {
            info.version = trim(rest[1..rb]);
            after = trim(rest[(rb + 1)..rest.len]);
            // Skip optional inline link target `(https://...)`
            if (after.len > 0 && after[0] == '(') {
                let rp: usize = 1;
                while (rp < after.len && after[rp] != ')') {
                    rp += 1;
                }
                if (rp < after.len) {
                    after = trim(after[(rp + 1)..after.len]);
                }
            }
        } else {
            info.version = rest;
        }
    } else {
        let sp: usize = 0;
        while (sp < rest.len && rest[sp] != ' ' && rest[sp] != '\t') {
            sp += 1;
        }
        info.version = rest[0..sp];
        after = trim(rest[sp..rest.len]);
    }

    info.is_unreleased = eqIgnoreCase(info.version, "unreleased");
    info.is_yanked = containsIgnoreCase(after, "[yanked]");

    // Extract optional `- YYYY-MM-DD` date
    if (after.len > 0 && after[0] == '-') {
        const dpart = trim(after[1..after.len]);
        let dend: usize = 0;
        while (dend < dpart.len && dpart[dend] != ' ' && dpart[dend] != '\t' && dpart[dend] != '[') {
            dend += 1;
        }
        if (dend > 0) {
            info.date = dpart[0..dend];
        }
    }

    return info;
}

fn isTopLevelBullet(line: string) -> bool {
    let i: usize = 0;
    while (i < line.len && i < 2 && line[i] == ' ') {
        i += 1;
    }
    if (i + 1 < line.len && (line[i] == '-' || line[i] == '*') && (line[i + 1] == ' ' || line[i + 1] == '\t')) {
        return true;
    }
    return false;
}

/// Parses a Keep a Changelog 1.1.0 markdown document into a zero-allocation `ParsedChangelog`.
pub fn parseChangelog(md: string) -> ParsedChangelog {
    let parsed = ParsedChangelog.empty();
    let in_release = false;
    let cur_idx: usize = 0;
    let body_start: usize = 0;
    let body_end: usize = 0;
    let cur_kind = ChangeKind.Other;

    let cursor: usize = 0;
    while (cursor < md.len) {
        const ls = cursor;
        cursor = findByte(md, '\n', cursor) ?? md.len;
        let le = cursor;
        if (le > ls && md[le - 1] == '\r') {
            le -= 1;
        }
        const next_cursor = if (cursor < md.len && md[cursor] == '\n') { cursor + 1 } else { cursor };
        const line = md[ls..le];
        const trimmed = trim(line);

        // New release heading: `## ` (but not `### `)
        if (trimmed.len >= 3 && trimmed[0] == '#' && trimmed[1] == '#' && (trimmed[2] == ' ' || trimmed[2] == '\t')) {
            if (in_release) {
                parsed.releases[cur_idx].body = trim(md[body_start..body_end]);
            }
            if (parsed.count < MAX_RELEASES) {
                cur_idx = parsed.count;
                parsed.releases[cur_idx] = parseReleaseHeading(trimmed[3..trimmed.len]);
                parsed.count += 1;
                in_release = true;
                body_start = next_cursor;
                body_end = next_cursor;
                cur_kind = ChangeKind.Other;
            } else {
                in_release = false;
            }
            cursor = next_cursor;
            continue;
        }

        if (in_release) {
            // Stop release body at bottom reference link definitions `[1.0.0]: https://...`
            if (isRefLinkDef(trimmed)) {
                parsed.releases[cur_idx].body = trim(md[body_start..body_end]);
                in_release = false;
                cursor = next_cursor;
                continue;
            }

            body_end = le;

            // Section heading: `### Added`, `### Changed`, `### Fixed`, `### Security`, ...
            if (trimmed.len >= 4 && trimmed[0] == '#' && trimmed[1] == '#' && trimmed[2] == '#' &&
                (trimmed[3] == ' ' || trimmed[3] == '\t')) {
                cur_kind = classifySection(trimmed[4..trimmed.len]);
            } else if (isTopLevelBullet(line)) {
                if (cur_kind == ChangeKind.Added) {
                    parsed.releases[cur_idx].added_count += 1;
                } else if (cur_kind == ChangeKind.Changed) {
                    parsed.releases[cur_idx].changed_count += 1;
                } else if (cur_kind == ChangeKind.Fixed) {
                    parsed.releases[cur_idx].fixed_count += 1;
                } else if (cur_kind == ChangeKind.Security) {
                    parsed.releases[cur_idx].security_count += 1;
                }
            }
        }

        cursor = next_cursor;
    }

    if (in_release) {
        parsed.releases[cur_idx].body = trim(md[body_start..body_end]);
    }

    return parsed;
}

/// Finds a release by exact version string (or `"Unreleased"`, case-insensitive).
pub fn findRelease(log: ParsedChangelog, version: string) -> ReleaseInfo? {
    let i: usize = 0;
    while (i < log.count) {
        if (eqIgnoreCase(log.releases[i].version, version)) {
            return log.releases[i];
        }
        i += 1;
    }
    return null;
}

/// Returns the latest non-Unreleased release entry, or `null` if none exist.
pub fn latestRelease(log: ParsedChangelog) -> ReleaseInfo? {
    let i: usize = 0;
    while (i < log.count) {
        if (!log.releases[i].is_unreleased) {
            return log.releases[i];
        }
        i += 1;
    }
    return null;
}

/// Formats a standard Keep a Changelog release heading into `out`
/// (e.g. `## [1.2.0] - 2026-11-26`, `## [Unreleased]`, or `## [1.1.0] - 2026-10-01 [YANKED]`).
pub fn formatReleaseHeader(version: string, date: string, yanked: bool, out: []u8) -> string {
    let pos: usize = 0;
    pos = writeStr("## [", out, pos);
    pos = writeStr(version, out, pos);
    pos = writeStr("]", out, pos);
    if (!eqIgnoreCase(version, "unreleased")) {
        if (date.len > 0) {
            pos = writeStr(" - ", out, pos);
            pos = writeStr(date, out, pos);
        }
        if (yanked) {
            pos = writeStr(" [YANKED]", out, pos);
        }
    }
    return out[0..pos];
}
