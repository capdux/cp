[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# shell-escape

Zero-allocation POSIX sh and Windows CommandLineToArgvW argument escaper, command joiner, and env var name validator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shell-escape` ([https://github.com/xxorax/node-shell-escape](https://github.com/xxorax/node-shell-escape)) / `crates.io:shell-escape` ([https://github.com/sfackler/shell-escape](https://github.com/sfackler/shell-escape))

| Metadata | Value |
| --- | --- |
| Package | `shell-escape` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add shell-escape
```

```cp
import {
    isSafePosixByte,
    isSafePosixWord,
    escapePosix,
    needsWindowsQuotes,
    escapeWindows,
    joinPosixCommand,
    isValidEnvName,
} from "shell-escape";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isSafePosixByte` | `(b: u8): bool` | Checks predicate `isSafePosixByte` and returns `bool`. |
| `isSafePosixWord` | `(arg: string): bool` | Checks predicate `isSafePosixWord` and returns `bool`. |
| `escapePosix` | `(arg: string, out: []u8): usize` | Invokes `escapePosix` and returns `usize`. |
| `needsWindowsQuotes` | `(arg: string): bool` | Invokes `needsWindowsQuotes` and returns `bool`. |
| `escapeWindows` | `(arg: string, out: []u8): usize` | Invokes `escapeWindows` and returns `usize`. |
| `joinPosixCommand` | `(args: &[]string, out: []u8): usize` | Invokes `joinPosixCommand` and returns `usize`. |
| `isValidEnvName` | `(name: string): bool` | Checks predicate `isValidEnvName` and returns `bool`. |

## Minimal Example

```cp
import { isSafePosixWord } from "shell-escape";

pub fn main() {
    let result = isSafePosixWord("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
