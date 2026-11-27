# chokidar — AI Agent Quick Ref

Zero-heap pure CP file watcher state machine, snapshot differ, debounce engine, and glob ignore filter (clean-room MIT implementation of npm chokidar v4 core) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    WatchEvent,
    ChokidarOptions,
    TrackedEntry,
    FileWatcherCore,
    chkStrEq,
} from "chokidar";
```

## Core Types & Constants

```cp
class WatchEvent { kind: u8, path: string, mtime_ms: u64, size: u64 }
class ChokidarOptions { ignore_initial: bool, await_write_finish_ms: u64, follow_symlinks: bool, depth: usize, /* +1 fields */ } // methods: defaults(...)
class TrackedEntry { active: bool, path: string, is_dir: bool, mtime_ms: u64, /* +2 fields */ }
class FileWatcherCore { opts: ChokidarOptions, matcher: GlobPatternSet, tracked: [32]TrackedEntry, tracked_count: usize, /* +3 fields */ } // methods: new(...) +8
```

## API Signatures

```cp
fn chkStrEq(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { chkStrEq } from "chokidar";

pub fn main() {
    let result = chkStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"chokidar"`; install via `cap add chokidar`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
