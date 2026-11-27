# rimraf — AI Agent Quick Ref

Zero-allocation recursive deletion safety guard, root-path validator, and bottom-up delete planner for CP, reusing fast-glob. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RimrafOptions,
    RimrafPlan,
    rmfIsAsciiAlpha,
    rmfIsSep,
    rmfHasNul,
    rmfPathDepth,
    isRootPath,
    validateRimrafTarget,
} from "rimraf";
```

## Core Types & Constants

```cp
class RimrafOptions { preserve_root: bool, max_retries: u8, retry_delay_ms: u32, is_windows: bool } // methods: defaults(...)
class RimrafPlan { valid: bool, paths: [32]string, count: usize }
```

## API Signatures

```cp
fn rmfIsAsciiAlpha(b: u8): bool;
fn rmfIsSep(b: u8, is_windows: bool): bool;
fn rmfHasNul(s: string): bool;
fn rmfPathDepth(path: string, is_windows: bool): usize;
fn isRootPath(path: string, is_windows: bool): bool;
fn validateRimrafTarget(target: string, opts: RimrafOptions): bool;
fn rmfSortByDepthDesc(paths: []string, count: usize, is_windows: bool);
fn planBottomUpDelete(entries: []string, opts: RimrafOptions): RimrafPlan;
fn planFilteredDelete(entries: []string, pattern: string, opts: RimrafOptions): RimrafPlan;
```

## Minimal Usage

```cp
import { isRootPath } from "rimraf";

pub fn main() {
    let result = isRootPath("example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rimraf"`; install via `cap add rimraf`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
