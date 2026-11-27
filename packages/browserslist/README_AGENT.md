# browserslist — AI Agent Quick Ref

Zero-heap Browserslist query clause parser and browser version matcher (`v4.24.4`, `MIT`)

## Import

```cp
import {
    BrowserOp,
    BrowserClause,
    blqStartsWithWord,
    blqParseU32,
    blqIsDeadBrowser,
    blqCmpVersion,
    parseBrowserClause,
    matchBrowserVersion,
} from "browserslist";
```

## Core Types & Constants

```cp
enum BrowserOp { Ge, Gt, Le, Lt, Eq, LastN, ... }
class BrowserClause { negated: bool, is_and: bool, browser: string, op: BrowserOp, /* +3 fields */ }
```

## API Signatures

```cp
fn blqStartsWithWord(s: string, word: string): bool;
fn blqParseU32(s: string): u32;
fn blqIsDeadBrowser(browser: string): bool;
fn blqCmpVersion(maj_a: u32, min_a: u32, maj_b: u32, min_b: u32): i32;
fn parseBrowserClause(clause: string): BrowserClause;
fn matchBrowserVersion(clause: BrowserClause, browser: string, major: u32, minor: u32, latest_major: u32): bool;
```

## Minimal Usage

```cp
import { parseBrowserClause } from "browserslist";

pub fn main() {
    let result = parseBrowserClause("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"browserslist"`; install via `cap add browserslist`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
