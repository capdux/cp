[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# keep-a-changelog

Zero-allocation Keep a Changelog 1.1.0 parser, release lookup, section counter, and header formatter (keep-a-changelog / changelog-parser / Rust parse-changelog parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:keep-a-changelog` ([https://github.com/oscarotero/keep-a-changelog](https://github.com/oscarotero/keep-a-changelog))（原库名 `keep-a-changelog` → CP 包名 `keep-a-changelog`）

| Metadata | Value |
| --- | --- |
| Package | `keep-a-changelog` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add keep-a-changelog
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `ChangeKind` | `Added, Changed, Deprecated, Removed, Fixed, Security, Other` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ReleaseInfo` | `class` | Fields: `version: string, date: string, is_unreleased: bool, is_yanked: bool, body: string, added_count: usize, +3 more` · Methods: `empty` |
| `ParsedChangelog` | `class` | Fields: `releases: [MAX_RELEASES]ReleaseInfo, count: usize` · Methods: `empty, releaseAt` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `containsIgnoreCase` | `(hay: string, needle_lower: string): bool` | Invokes `containsIgnoreCase` and returns `bool`. |
| `isRefLinkDef` | `(trimmed: string): bool` | Checks predicate `isRefLinkDef` and returns `bool`. |
| `classifySection` | `(heading: string): ChangeKind` | Invokes `classifySection` and returns `ChangeKind`. |
| `parseReleaseHeading` | `(rest_raw: string): ReleaseInfo` | Parses input via `parseReleaseHeading` and returns `ReleaseInfo`. |
| `isTopLevelBullet` | `(line: string): bool` | Checks predicate `isTopLevelBullet` and returns `bool`. |
| `parseChangelog` | `(md: string): ParsedChangelog` | Parses input via `parseChangelog` and returns `ParsedChangelog`. |
| `findRelease` | `(log: ParsedChangelog, version: string): ReleaseInfo?` | Queries/retrieves data via `findRelease` returning `ReleaseInfo?`. |
| `latestRelease` | `(log: ParsedChangelog): ReleaseInfo?` | Invokes `latestRelease` and returns `ReleaseInfo?`. |
| `formatReleaseHeader` | `(version: string, date: string, yanked: bool, out: []u8): string` | Serializes/formats data via `formatReleaseHeader` returning `string`. |

## Minimal Example

```cp
import { isRefLinkDef } from "keep-a-changelog";

pub fn main() {
    let result = isRefLinkDef("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
