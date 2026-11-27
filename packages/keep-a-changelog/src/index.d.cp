// Type definitions for CP

export enum ChangeKind {
    Added,
    Changed,
    Deprecated,
    Removed,
    Fixed,
    Security,
    Other
}

export class ReleaseInfo {
    version: string,
    date: string,
    is_unreleased: bool,
    is_yanked: bool,
    body: string,
    added_count: usize,
    changed_count: usize,
    fixed_count: usize,
    security_count: usize,
    fn empty(): ReleaseInfo;
}

export class ParsedChangelog {
    releases: [MAX_RELEASES]ReleaseInfo,
    count: usize,
    fn empty(): ParsedChangelog;
    fn releaseAt(i: usize): ReleaseInfo;
}

export fn isSpace(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn trim(s: string): string;
export fn eqIgnoreCase(a: string, b: string): bool;
export fn containsIgnoreCase(hay: string, needle_lower: string): bool;
export fn isRefLinkDef(trimmed: string): bool;
export fn classifySection(heading: string): ChangeKind;
export fn parseReleaseHeading(rest_raw: string): ReleaseInfo;
export fn isTopLevelBullet(line: string): bool;
export fn parseChangelog(md: string): ParsedChangelog;
export fn findRelease(log: ParsedChangelog, version: string): ReleaseInfo?;
export fn latestRelease(log: ParsedChangelog): ReleaseInfo?;
export fn pushStr(out: []u8, pos: usize, s: string): usize;
export fn formatReleaseHeader(version: string, date: string, yanked: bool, out: []u8): string;
