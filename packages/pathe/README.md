[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pathe

Zero-allocation universal POSIX-normalized path utilities preserving Windows drive letters and URL protocols (clean-room MIT implementation of npm pathe, #219 / 207M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pathe` ([https://github.com/unjs/pathe](https://github.com/unjs/pathe)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `pathe` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pathe
```

```cp
import {
    pthIsSep,
    pthIsAlpha,
    pthIsSchemeChar,
    pthHasDrivePrefix,
    pthProtocolPrefixLen,
    pthPush,
    pthPushSlice,
    pthStrEq,
} from "pathe";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pthIsSep` | `(b: u8): bool` | Invokes `pthIsSep` and returns `bool`. |
| `pthIsAlpha` | `(b: u8): bool` | Invokes `pthIsAlpha` and returns `bool`. |
| `pthIsSchemeChar` | `(b: u8): bool` | Invokes `pthIsSchemeChar` and returns `bool`. |
| `pthHasDrivePrefix` | `(s: string): bool` | Invokes `pthHasDrivePrefix` and returns `bool`. |
| `pthProtocolPrefixLen` | `(s: string): usize` | Invokes `pthProtocolPrefixLen` and returns `usize`. |
| `pthPush` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `pthPush` and returns `usize`. |
| `pthPushSlice` | `(out: []u8, pos: usize, s: string): usize` | Invokes `pthPushSlice` and returns `usize`. |
| `pthStrEq` | `(a: string, b: string): bool` | Invokes `pthStrEq` and returns `bool`. |
| `isAbsolute` | `(path: string): bool` | Checks predicate `isAbsolute` and returns `bool`. |
| `normalize` | `(path: string, out: []u8): usize` | Invokes `normalize` and returns `usize`. |
| `join` | `(a: string, b: string, out: []u8): usize` | Invokes `join` and returns `usize`. |
| `joinPath` | `(a: string, b: string, out: []u8): usize` | Invokes `joinPath` and returns `usize`. |
| `pthStripTrailingSlash` | `(out: []u8, len: usize): usize` | Invokes `pthStripTrailingSlash` and returns `usize`. |
| `resolve` | `(base: string, rel: string, out: []u8): usize` | Invokes `resolve` and returns `usize`. |
| `dirname` | `(path: string): string` | Invokes `dirname` and returns `string`. |
| `basename` | `(path: string): string` | Invokes `basename` and returns `string`. |
| `extname` | `(path: string): string` | Invokes `extname` and returns `string`. |
| `pthSplitSegments` | `(s: string, starts: []usize, lens: []usize): usize` | Invokes `pthSplitSegments` and returns `usize`. |
| `relative` | `(from_path: string, to_path: string, out: []u8): usize` | Invokes `relative` and returns `usize`. |

## Minimal Example

```cp
import { isAbsolute } from "pathe";

pub fn main() {
    let result = isAbsolute("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
