# browserslist@4.24.4 (Agent Quickref)

> Zero-heap Browserslist query clause parser and browser version matcher | License: MIT
> Import: `import { ... } from "browserslist";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum BrowserOp`

## Functions
- `export fn blqStartsWithWord(s: string, word: string): bool;`
- `export fn blqParseU32(s: string): u32;`
- `export fn blqIsDeadBrowser(browser: string): bool;`
- `export fn blqCmpVersion(maj_a: u32, min_a: u32, maj_b: u32, min_b: u32): i32;`
- `export fn parseBrowserClause(clause: string): BrowserClause;`
- `export fn matchBrowserVersion(clause: BrowserClause, browser: string, major: u32, minor: u32, latest_major: u32): bool;`
