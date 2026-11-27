# resolve-pkg-maps — AI Agent Quick Ref

Zero-heap package.json exports subpath wildcard and condition resolver (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ExportRule,
    rpmEq,
    rpmFindStar,
    rpmHasParentTraversal,
    rpmConditionRank,
    matchSubpathPattern,
    resolveExportTarget,
} from "resolve-pkg-maps";
```

## Core Types & Constants

```cp
class ExportRule { pattern: string, condition: string, target: string }
```

## API Signatures

```cp
fn rpmEq(a: string, b: string): bool;
fn rpmFindStar(s: string): usize;
fn rpmHasParentTraversal(path: string): bool;
fn rpmConditionRank(cond: string, conditions: &[]string): usize;
fn matchSubpathPattern(pattern: string, subpath: string, matched_star: []u8): i64;
fn resolveExportTarget(rules: &[]ExportRule, subpath: string, conditions: &[]string, out: []u8): usize;
```

## Minimal Usage

```cp
import { rpmEq } from "resolve-pkg-maps";

pub fn main() {
    let result = rpmEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"resolve-pkg-maps"`; install via `cap add resolve-pkg-maps`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
