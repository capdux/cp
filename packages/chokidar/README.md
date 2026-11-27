[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# chokidar

Zero-heap pure CP file watcher state machine, snapshot differ, debounce engine, and glob ignore filter (clean-room MIT implementation of npm chokidar v4 core)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:chokidar` ([https://github.com/paulmillr/chokidar](https://github.com/paulmillr/chokidar))（原库名 `chokidar` → CP 包名 `chokidar`）

| Metadata | Value |
| --- | --- |
| Package | `chokidar` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add chokidar
```

```cp
import {
    WatchEvent,
    ChokidarOptions,
    TrackedEntry,
    FileWatcherCore,
    chkStrEq,
} from "chokidar";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `WatchEvent` | `class` | Fields: `kind: u8, path: string, mtime_ms: u64, size: u64` |
| `ChokidarOptions` | `class` | Fields: `ignore_initial: bool, await_write_finish_ms: u64, follow_symlinks: bool, depth: usize, ignore_dotfiles: bool` · Methods: `defaults` |
| `TrackedEntry` | `class` | Fields: `active: bool, path: string, is_dir: bool, mtime_ms: u64, size: u64, last_emit_ms: u64` |
| `FileWatcherCore` | `class` | Fields: `opts: ChokidarOptions, matcher: GlobPatternSet, tracked: [32]TrackedEntry, tracked_count: usize, events: [32]WatchEvent, event_head: usize, +1 more` · Methods: `new, addIgnore, isIgnored, chkEnqueue, recordSnapshot, recordUnlink, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `chkStrEq` | `(a: string, b: string): bool` | Invokes `chkStrEq` and returns `bool`. |

## Minimal Example

```cp
import { chkStrEq } from "chokidar";

pub fn main() {
    let result = chkStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
