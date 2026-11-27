# jest-diff@1.0.0 (Agent Quickref)

> Zero-heap pure CP Jest-style unified diff engine with ANSI coloring (clean-room MIT implementation of npm jest-diff) | License: MIT
> Import: `import { ... } from "jest-diff";` | Full types: `src/index.d.cp`
> Deps: `picocolors`, `similar`

## Functions
- `export fn diffStats(expected: string, received: string): DiffSummary;`
- `export fn diffLinesUnified(expected: string, received: string, opts: DiffOptions, out: []u8): usize;`
