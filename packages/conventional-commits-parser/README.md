[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# conventional-commits-parser

Zero-allocation Conventional Commits 1.0.0 specification parser, header formatter, and Semver bump recommender for CP — clean-room MIT parity with npm `conventional-commits-parser` and Rust `git-conventional`.

Parses `<type>[(<scope>)][!]: <description>`, multiline body, up to 16 `Token: value` / `Token #value` footers, and `BREAKING CHANGE` / `BREAKING-CHANGE` markers into zero-copy slices, with automatic Semver bump recommendation and multi-commit release aggregation.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `conventional-commits-parser` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/conventional-changelog/conventional-changelog` |
| License | MIT |

Keep the exact package name when migrating from TypeScript/Node.js to CP:

```ts
// TypeScript / Node.js (npm: conventional-commits-parser)
import { CommitParser } from "conventional-commits-parser";
const parser = new CommitParser();
const commit = parser.parse("feat(parser)!: drop legacy v1 syntax");
```

```cp
// CP (cap: conventional-commits-parser) — zero-copy parse & Semver bump
import { parseCommit, recommendedBump } from "conventional-commits-parser";

const commit = parseCommit("feat(parser)!: drop legacy v1 syntax");
const bump = recommendedBump(commit); // SemverBump.Major
```

## Installation

```bash
cap add conventional-commits-parser
```

## Usage

### Basic Usage: Parse Commit, Inspect Footers & Recommend Semver Bump

```cp
import {
    SemverBump,
    parseCommit,
    recommendedBump,
    formatCommitHeader,
} from "conventional-commits-parser";

fn demoCommit() {
    const msg = "feat(parser)!: drop legacy v1 syntax\n\nMigration details here.\n\nBREAKING CHANGE: remove old macro mode\nRefs #42";
    const c = parseCommit(msg);
    if (c.valid) {
        // c.commit_type == "feat", c.scope == "parser", c.is_breaking == true
        // c.breaking_note == "remove old macro mode"
        const bump = recommendedBump(c); // SemverBump.Major
        const refs = c.getFooter("Refs"); // Some("42")

        let hdr_buf: [128]u8 = undefined;
        const hdr = formatCommitHeader(c, hdr_buf[0..128]);
        // => "feat(parser)!: drop legacy v1 syntax"
        _ = bump;
        _ = refs;
        _ = hdr;
    }
}
```

### Edge Cases: Release Aggregation, Header Validation & Malformed Messages

```cp
import {
    SemverBump,
    parseCommit,
    isValidHeader,
    recommendedBump,
    highestBump,
} from "conventional-commits-parser";

fn demoRelease() {
    // 1. Aggregate Semver bumps across a batch of commits
    let release_bump = SemverBump.None;
    const c1 = parseCommit("chore: update docs");
    const c2 = parseCommit("fix(net): guard empty packet");
    const c3 = parseCommit("feat(http): add streaming response");
    release_bump = highestBump(release_bump, recommendedBump(c1));
    release_bump = highestBump(release_bump, recommendedBump(c2));
    release_bump = highestBump(release_bump, recommendedBump(c3));
    // release_bump == SemverBump.Minor

    // 2. Fast single-line header validation & graceful rejection
    const ok1 = isValidHeader("fix(core): handle EOF"); // true
    const ok2 = isValidHeader("WIP: missing space?");   // false (invalid or non-conventional)
    const bad = parseCommit("not a conventional commit");
    // bad.valid == false; recommendedBump(bad) == SemverBump.None
    _ = release_bump;
    _ = ok1;
    _ = ok2;
    _ = bad;
}
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `SemverBump` | `enum { None = 0, Patch = 1, Minor = 2, Major = 3 }` | Recommended Semver bump level. |
| `CommitFooter` | `class { token: string, value: string, is_breaking: bool }` | Single parsed footer entry. |
| `ParsedCommit` | `class { valid, commit_type, scope, description, body, is_breaking, breaking_note, footers: [16]CommitFooter, footer_count }` | Zero-copy parsed commit with `footerAt(i): CommitFooter?` and `getFooter(token): string?`. |
| `parseCommit` | `fn(msg: string): ParsedCommit` | Parses a full Conventional Commits 1.0.0 message. |
| `isValidHeader` | `fn(header: string): bool` | Validates a single-line commit header. |
| `recommendedBump` | `fn(commit: ParsedCommit): SemverBump` | Maps breaking -> `Major`, `feat` -> `Minor`, `fix` -> `Patch`, otherwise `None`. |
| `highestBump` / `bumpRank` | `fn(a, b): SemverBump` / `fn(b): u8` | Aggregates the highest bump across multiple commits. |
| `formatCommitHeader` | `fn(commit: ParsedCommit, out: []u8): string` | Formats a `ParsedCommit` back into `<type>[(<scope>)][!]: <description>`. |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`eqIgnoreCase`, `isAlpha`, `isAlnum`) and `cap:fmt` (`writeStr`) for fast header/footer tokenization and formatting.
- **Zero Heap Allocation & Fixed Footer Pool**: Stores up to `MAX_FOOTERS = 16` footers on the stack (`[16]CommitFooter`) with zero-copy string slices into `msg`.
- **Zero-Clear Stack Arrays (`undefined`)**: Internal footer pool uses `undefined` initialization to skip unnecessary zeroing of unused slots.
- **Bounds-Safe Formatting**: `formatCommitHeader` bounds-checks every write to `out: []u8` and never panics on invalid commits.

## License

MIT — Clean-room CP implementation inspired by npm `conventional-commits-parser`.
