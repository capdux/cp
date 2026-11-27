# minimatch — AI Agent Quick Ref

Minimal matching utility converting glob expressions (clean-room CP implementation of npm minimatch) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MinimatchOptions,
    miniToPico,
    isCommentPattern,
    isNegatedPattern,
    minimatchWithOptions,
    minimatch,
    minimatchList,
    minimatchFilterAll,
} from "minimatch";
```

## Core Types & Constants

```cp
class MinimatchOptions { dot: bool, nocase: bool, match_base: bool, noglobstar: bool, /* +3 fields */ } // methods: defaults(...)
```

## API Signatures

```cp
fn miniToPico(opts: MinimatchOptions): PicoOptions;
fn isCommentPattern(pattern: string, opts: MinimatchOptions): bool;
fn isNegatedPattern(pattern: string, opts: MinimatchOptions): bool;
fn minimatchWithOptions(path: string, pattern: string, opts: MinimatchOptions): bool;
fn minimatch(path: string, pattern: string): bool;
fn minimatchList(paths: &[]string, pattern: string, opts: MinimatchOptions, out_indices: []usize): usize;
fn minimatchFilterAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize;
fn minimatchRejectAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize;
```

## Minimal Usage

```cp
import { isCommentPattern } from "minimatch";

pub fn main() {
    let result = isCommentPattern("example", /* opts: MinimatchOptions */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"minimatch"`; install via `cap add minimatch`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
