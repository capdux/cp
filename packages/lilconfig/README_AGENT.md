# lilconfig — AI Agent Quick Ref

Zero-heap pure CP lightweight configuration explorer with JSON/JS defaults and custom loader registry (clean-room MIT implementation of npm lilconfig v3) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    LilconfigOptions,
    CustomLoaderEntry,
    LilExplorer,
    lilEndsWithIgnoreCase,
    lilBasename,
    lilIsNoExt,
    formatLilSearchPlace,
    isSupportedByDefault,
} from "lilconfig";
```

## Core Types & Constants

```cp
class LilconfigOptions { module_name: string, stop_dir: string, package_prop: string, ignore_empty_search_places: bool } // methods: defaults(...)
class CustomLoaderEntry { ext: string, loader_id: u8 }
class LilExplorer { opts: LilconfigOptions, planner: SearchPlanner, custom_loaders: [8]CustomLoaderEntry, custom_count: usize } // methods: new(...) +5
```

## API Signatures

```cp
fn lilEndsWithIgnoreCase(s: string, suffix: string): bool;
fn lilBasename(filepath: string): string;
fn lilIsNoExt(filepath: string): bool;
fn formatLilSearchPlace(module_name: string, idx: usize, out: []u8): usize;
fn isSupportedByDefault(filepath: string): bool;
```

## Minimal Usage

```cp
import { formatLilSearchPlace } from "lilconfig";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatLilSearchPlace("example", 0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"lilconfig"`; install via `cap add lilconfig`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
