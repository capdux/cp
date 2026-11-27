# conventional-commits-parser@1.0.0 — AI Agent Quick Ref

> Zero-alloc Conventional Commits 1.0.0 parser, header formatter & Semver bump recommender | Source: `npm:conventional-commits-parser` | License: MIT
> Import: `import { SemverBump, ParsedCommit, parseCommit, isValidHeader, recommendedBump, highestBump, formatCommitHeader } from "conventional-commits-parser";` | Types: `src/index.d.cp`

## Core Types & Constants

```cp
export enum SemverBump { None = 0, Patch = 1, Minor = 2, Major = 3 }

export class CommitFooter {
    token: string, value: string, is_breaking: bool,
    fn empty(): CommitFooter;
}

export class ParsedCommit {
    valid: bool,
    commit_type: string, scope: string, description: string, body: string,
    is_breaking: bool, breaking_note: string,
    footers: [16]CommitFooter, footer_count: usize,
    fn invalid(): ParsedCommit;
    fn footerAt(i: usize): CommitFooter?;
    fn getFooter(token: string): string?;
}
// Constant: MAX_FOOTERS = 16
```

## Core Functions

- `parseCommit(msg: string): ParsedCommit` — Parses `<type>[(<scope>)][!]: <description>`, multiline body, and up to 16 footers.
- `isValidHeader(header: string): bool` — Fast validation for single-line commit header.
- `recommendedBump(commit: ParsedCommit): SemverBump` — `is_breaking` -> `Major`, `feat` -> `Minor`, `fix` -> `Patch`, else `None`.
- `highestBump(a: SemverBump, b: SemverBump): SemverBump` | `bumpRank(b: SemverBump): u8`
- `formatCommitHeader(commit: ParsedCommit, out: []u8): string` — Reconstructs canonical header into `out`.

## Minimal Usage

```cp
import { SemverBump, parseCommit, recommendedBump, highestBump } from "conventional-commits-parser";

const c = parseCommit("feat(api)!: change auth header\n\nBREAKING CHANGE: use Bearer");
const bump = recommendedBump(c); // SemverBump.Major
const merged = highestBump(SemverBump.Patch, bump); // SemverBump.Major
```

## Pitfalls & Rules

1. **Zero-copy slices**: All `string` fields on `ParsedCommit` and `CommitFooter` borrow `msg`; keep `msg` alive while inspecting `c`.
2. **Footer cap**: At most `16` footers are stored (`footer_count <= 16`). Use `c.footerAt(i)` or `c.getFooter("Refs")` (case-insensitive token lookup).
3. **Invalid commits**: Check `c.valid`; `recommendedBump` returns `SemverBump.None` for invalid commits.
4. **Breaking detection**: Either `!` before `:` in header OR a `BREAKING CHANGE:` / `BREAKING-CHANGE:` footer sets `c.is_breaking == true`.
5. **Header formatter buffer**: `formatCommitHeader` returns a slice of `out` (`""` if `!commit.valid`).
