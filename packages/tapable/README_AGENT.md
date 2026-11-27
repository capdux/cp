# tapable@1.0.0 (Agent Quickref)

> Zero-allocation plugin hook engine (SyncHook, SyncBailHook, SyncWaterfallHook, SyncLoopHook) with stage ordering and before insertion (clean-room MIT implementation of npm tapable v2, #242 / 195M wk) | License: MIT
> Import: `import { ... } from "tapable";` | Full types: `src/index.d.cp`

## Functions
- `export fn tapEmptyEntry(): TapEntry;`
- `export fn tapStrEq(a: string, b: string): bool;`
- `export fn tapInsertSorted(entries: []TapEntry, count: usize, entry: TapEntry): usize;`
- `export fn tapInsertBefore(entries: []TapEntry, count: usize, name: string, before_name: string, handler_id: u32): usize;`
- `export fn tapSetEnabled(entries: []TapEntry, count: usize, name: string, enabled: bool): bool;`
- `export fn tapCollectOrdered(entries: &[]TapEntry, count: usize, out: []u32): usize;`
