# keep-a-changelog — AI Agent Quick Ref

Zero-allocation Keep a Changelog 1.1.0 parser, release lookup, section counter, and header formatter (keep-a-changelog / changelog-parser / Rust parse-changelog parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ChangeKind,
    ReleaseInfo,
    ParsedChangelog,
    containsIgnoreCase,
    isRefLinkDef,
    classifySection,
    parseReleaseHeading,
    isTopLevelBullet,
} from "keep-a-changelog";
```

## Core Types & Constants

```cp
enum ChangeKind { Added, Changed, Deprecated, Removed, Fixed, Security, ... }
class ReleaseInfo { version: string, date: string, is_unreleased: bool, is_yanked: bool, /* +5 fields */ } // methods: empty(...)
class ParsedChangelog { releases: [MAX_RELEASES]ReleaseInfo, count: usize } // methods: empty(...) +1
```

## API Signatures

```cp
fn containsIgnoreCase(hay: string, needle_lower: string): bool;
fn isRefLinkDef(trimmed: string): bool;
fn classifySection(heading: string): ChangeKind;
fn parseReleaseHeading(rest_raw: string): ReleaseInfo;
fn isTopLevelBullet(line: string): bool;
fn parseChangelog(md: string): ParsedChangelog;
fn findRelease(log: ParsedChangelog, version: string): ReleaseInfo?;
fn latestRelease(log: ParsedChangelog): ReleaseInfo?;
fn formatReleaseHeader(version: string, date: string, yanked: bool, out: []u8): string;
```

## Minimal Usage

```cp
import { isRefLinkDef } from "keep-a-changelog";

pub fn main() {
    let result = isRefLinkDef("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"keep-a-changelog"`; install via `cap add keep-a-changelog`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
