# cosmiconfig — AI Agent Quick Ref

Zero-heap pure CP configuration file search planner, candidate path builder, and loader detector (clean-room MIT implementation of npm cosmiconfig v9) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ExplorerOptions,
    SearchPlanner,
    CosmiConfigResult,
    cosmiEndsWithIgnoreCase,
    cosmiBasename,
    cosmiTrimTrailingSlash,
    detectLoader,
    formatSearchPlace,
} from "cosmiconfig";
```

## Core Types & Constants

```cp
class ExplorerOptions { module_name: string, stop_dir: string, package_prop: string, ignore_empty: bool } // methods: defaults(...)
class SearchPlanner { start_dir: string, stop_dir: string, current_end: usize, done: bool } // methods: new(...) +2
class CosmiConfigResult { filepath: string, is_empty: bool, loader_kind: u8, found: bool } // methods: notFound(...) +1
```

## API Signatures

```cp
fn cosmiEndsWithIgnoreCase(s: string, suffix: string): bool;
fn cosmiBasename(filepath: string): string;
fn cosmiTrimTrailingSlash(dir: string): string;
fn detectLoader(filepath: string): u8;
fn formatSearchPlace(module_name: string, place_idx: usize, out: []u8): usize;
fn buildCandidatePath(dir: string, place: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatSearchPlace } from "cosmiconfig";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatSearchPlace("example", 0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cosmiconfig"`; install via `cap add cosmiconfig`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
