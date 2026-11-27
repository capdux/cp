# conventional-commits-parser@1.0.0 (Agent Quickref)

> Zero-allocation Conventional Commits 1.0.0 parser, formatter, and Semver bump recommender for CP | License: MIT
> Import: `import { ... } from "conventional-commits-parser";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum SemverBump`

## Functions
- `export fn isTypeChar(c: u8): bool;`
- `export fn isFooterTokenChar(c: u8): bool;`
- `export fn ccIsHspace(c: u8): bool;`
- `export fn isLineBlank(s: string, start: usize, end: usize): bool;`
- `export fn trimLineEnd(s: string, start: usize, end: usize): usize;`
- `export fn trimBlockEdges(s: string, start: usize, end: usize): string;`
- `export fn matchFooterLine(msg: string, line_start: usize, line_end: usize): FooterLineMatch;`
- `export fn parseCommit(msg: string): ParsedCommit;`
- `export fn isValidHeader(header: string): bool;`
- `export fn recommendedBump(commit: ParsedCommit): SemverBump;`
- `export fn bumpRank(b: SemverBump): u8;`
- `export fn highestBump(a: SemverBump, b: SemverBump): SemverBump;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn formatCommitHeader(commit: ParsedCommit, out: []u8): string;`
