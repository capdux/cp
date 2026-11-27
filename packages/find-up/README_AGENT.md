# find-up — AI Agent Quick Ref

Zero-heap upward ancestor directory file/directory search walker with stopAt boundary and multi-name candidate generation (npm find-up equivalent, reuses npm-run-path) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    FindUpOptions,
    FindUpWalker,
    fupStrEq,
    fupTrimTrailingSlash,
    fupPush,
    joinDirAndName,
    findUpInList,
    findUpMultipleInList,
} from "find-up";
```

## Core Types & Constants

```cp
class FindUpOptions { cwd: string, stop_at: string, allow_symlinks: bool } // methods: defaults(...)
class FindUpWalker { current: string, stop_at: string, done: bool, steps: usize } // methods: new(...) +2
```

## API Signatures

```cp
fn fupStrEq(a: string, b: string): bool;
fn fupTrimTrailingSlash(s: string): string;
fn fupPush(out: []u8, pos: usize, s: string): usize;
fn joinDirAndName(dir: string, name: string, out: []u8): usize;
fn findUpInList(opts: FindUpOptions, filename: string, existing_paths: []string, out: []u8): usize?;
fn findUpMultipleInList(opts: FindUpOptions, filenames: []string, existing_paths: []string, out: []u8): usize?;
```

## Minimal Usage

```cp
import { fupStrEq } from "find-up";

pub fn main() {
    let result = fupStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"find-up"`; install via `cap add find-up`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
