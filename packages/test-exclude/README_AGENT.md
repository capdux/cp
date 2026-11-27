# test-exclude — AI Agent Quick Ref

Zero-heap test and coverage file path glob filter engine (`v7.0.1`, `ISC`)

## Import

```cp
import {
    ExcludeConfig,
    texcHasPrefix,
    texcHasSuffix,
    texcContainsSegment,
    texcMatchAt,
    texcMatchRaw,
    matchSimpleGlob,
    texcMatchesExt,
} from "test-exclude";
```

## Core Types & Constants

```cp
class ExcludeConfig { includes: &[]string, excludes: &[]string, extensions: &[]string }
```

## API Signatures

```cp
fn texcHasPrefix(s: string, prefix: string): bool;
fn texcHasSuffix(s: string, suffix: string): bool;
fn texcContainsSegment(path: string, seg: string): bool;
fn texcMatchAt(pat: string, pi: usize, text: string, ti: usize): bool;
fn texcMatchRaw(pat: string, path: string): bool;
fn matchSimpleGlob(pattern: string, path: string): bool;
fn texcMatchesExt(path: string, ext: string): bool;
fn shouldInstrument(rel_path: string, cfg: ExcludeConfig): bool;
```

## Minimal Usage

```cp
import { texcHasPrefix } from "test-exclude";

pub fn main() {
    let result = texcHasPrefix("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"test-exclude"`; install via `cap add test-exclude`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
