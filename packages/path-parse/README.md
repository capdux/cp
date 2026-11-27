[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# path-parse

Zero-allocation POSIX and Win32 path.parse() splitter into { root, dir, base, ext, name } (clean-room MIT implementation of npm path-parse, #238 / 196M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:path-parse` ([https://github.com/jbgutierrez/path-parse](https://github.com/jbgutierrez/path-parse)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `path-parse` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add path-parse
```

```cp
import {
    ParsedPath,
    PprNameExt,
    pprIsWinSep,
    pprIsAlpha,
    pprSplitBaseExt,
    parsePosix,
    parseWin32,
} from "path-parse";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParsedPath` | `class` | Fields: `root: string, dir: string, base: string, ext: string, name: string` |
| `PprNameExt` | `class` | Fields: `name: string, ext: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pprIsWinSep` | `(b: u8): bool` | Invokes `pprIsWinSep` and returns `bool`. |
| `pprIsAlpha` | `(b: u8): bool` | Invokes `pprIsAlpha` and returns `bool`. |
| `pprSplitBaseExt` | `(base: string): PprNameExt` | Invokes `pprSplitBaseExt` and returns `PprNameExt`. |
| `parsePosix` | `(path: string): ParsedPath` | Parses input via `parsePosix` and returns `ParsedPath`. |
| `parseWin32` | `(path: string): ParsedPath` | Parses input via `parseWin32` and returns `ParsedPath`. |

## Minimal Example

```cp
import { parsePosix } from "path-parse";

pub fn main() {
    let result = parsePosix("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
