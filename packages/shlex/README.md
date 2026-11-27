[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# shlex

Zero-allocation POSIX shell quoting, command-line lexical splitting & injection guard (shell-quote / Rust shlex parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shell-quote` ([https://github.com/ljharb/shell-quote](https://github.com/ljharb/shell-quote)) / `npm:shell-quote` ([https://github.com/inspect-js/node-shell-quote](https://github.com/inspect-js/node-shell-quote)) / `crates.io:shlex` ([https://github.com/comex/rust-shlex](https://github.com/comex/rust-shlex))（原库名 `shell-quote` → CP 包名 `shlex`）

| Metadata | Value |
| --- | --- |
| Package | `shlex` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add shlex
```

```cp
import {
    ShlexError,
    SplitResult,
    isSafeUnquotedChar,
    needsQuote,
    hasShellMetachars,
    appendByte,
    quoteArg,
    joinArgs,
} from "shlex";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `ShlexError` | `None, UnclosedSingleQuote, UnclosedDoubleQuote, TrailingBackslash, TooManyTokens, BufferOverflow` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SplitResult` | `class` | Fields: `pool: [MAX_POOL_BYTES]u8, starts: [MAX_TOKENS]usize, lens: [MAX_TOKENS]usize, count: usize, ok: bool, err: ShlexError` · Methods: `empty, tokenAt` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isSafeUnquotedChar` | `(c: u8): bool` | Checks predicate `isSafeUnquotedChar` and returns `bool`. |
| `needsQuote` | `(arg: string): bool` | Invokes `needsQuote` and returns `bool`. |
| `hasShellMetachars` | `(s: string): bool` | Checks predicate `hasShellMetachars` and returns `bool`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `quoteArg` | `(arg: string, out: []u8): string` | Invokes `quoteArg` and returns `string`. |
| `joinArgs` | `(args: []string, out: []u8): string` | Invokes `joinArgs` and returns `string`. |
| `splitShell` | `(cmd: string): SplitResult` | Invokes `splitShell` and returns `SplitResult`. |

## Minimal Example

```cp
import { needsQuote } from "shlex";

pub fn main() {
    let result = needsQuote("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
