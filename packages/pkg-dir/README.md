[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pkg-dir

Zero-heap Node.js/CP package root directory finder locating nearest ancestor package.json or cap.json (npm pkg-dir equivalent, reuses find-up & npm-run-path)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pkg-dir` ([https://github.com/sindresorhus/pkg-dir](https://github.com/sindresorhus/pkg-dir))

| Metadata | Value |
| --- | --- |
| Package | `pkg-dir` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pkg-dir
```

```cp
import { pkdPush, packageDirectoryInList, capPackageDirectoryInList, packageDirectoryBounded } from "pkg-dir";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pkdPush` | `(out: []u8, pos: usize, s: string): usize` | Invokes `pkdPush` and returns `usize`. |
| `packageDirectoryInList` | `(cwd: string, existing_files: []string, out: []u8): usize?` | Invokes `packageDirectoryInList` and returns `usize?`. |
| `capPackageDirectoryInList` | `(cwd: string, existing_files: []string, out: []u8): usize?` | Invokes `capPackageDirectoryInList` and returns `usize?`. |
| `packageDirectoryBounded` | `(cwd: string, stop_at: string, existing_files: []string, out: []u8): usize?` | Invokes `packageDirectoryBounded` and returns `usize?`. |

## Minimal Example

```cp
import { pkdPush } from "pkg-dir";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pkdPush(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
