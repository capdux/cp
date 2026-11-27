# keep-a-changelog@1.0.0 (Agent Quickref)

> Zero-allocation Keep a Changelog 1.1.0 parser, release lookup, section counter, and header formatter (keep-a-changelog / changelog-parser / Rust parse-changelog parity) for CP | License: MIT
> Import: `import { ... } from "keep-a-changelog";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum ChangeKind`

## Functions
- `export fn containsIgnoreCase(hay: string, needle_lower: string): bool;`
- `export fn isRefLinkDef(trimmed: string): bool;`
- `export fn classifySection(heading: string): ChangeKind;`
- `export fn parseReleaseHeading(rest_raw: string): ReleaseInfo;`
- `export fn isTopLevelBullet(line: string): bool;`
- `export fn parseChangelog(md: string): ParsedChangelog;`
- `export fn findRelease(log: ParsedChangelog, version: string): ReleaseInfo?;`
- `export fn latestRelease(log: ParsedChangelog): ReleaseInfo?;`
- `export fn formatReleaseHeader(version: string, date: string, yanked: bool, out: []u8): string;`
