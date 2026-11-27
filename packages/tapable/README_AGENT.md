# tapable — AI Agent Quick Ref

Zero-allocation plugin hook engine (SyncHook, SyncBailHook, SyncWaterfallHook, SyncLoopHook) with stage ordering and before insertion (clean-room MIT implementation of npm tapable v2, #242 / 195M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TapEntry,
    SyncHook,
    SyncBailHook,
    SyncWaterfallHook,
    SyncLoopHook,
    tapEmptyEntry,
    tapStrEq,
    tapInsertSorted,
} from "tapable";
```

## Core Types & Constants

```cp
class TapEntry { name: string, stage: i32, handler_id: u32, enabled: bool }
class SyncHook { entries: [32]TapEntry, count: usize } // methods: init(...) +8
class SyncBailHook { entries: [32]TapEntry, count: usize } // methods: init(...) +8
class SyncWaterfallHook { entries: [32]TapEntry, count: usize } // methods: init(...) +8
class SyncLoopHook { entries: [32]TapEntry, count: usize } // methods: init(...) +8
```

## API Signatures

```cp
fn tapEmptyEntry(): TapEntry;
fn tapStrEq(a: string, b: string): bool;
fn tapInsertSorted(entries: []TapEntry, count: usize, entry: TapEntry): usize;
fn tapInsertBefore(entries: []TapEntry, count: usize, name: string, before_name: string, handler_id: u32): usize;
fn tapSetEnabled(entries: []TapEntry, count: usize, name: string, enabled: bool): bool;
fn tapCollectOrdered(entries: &[]TapEntry, count: usize, out: []u32): usize;
```

## Minimal Usage

```cp
import { tapStrEq } from "tapable";

pub fn main() {
    let result = tapStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tapable"`; install via `cap add tapable`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
