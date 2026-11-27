[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# execa

Zero-heap subprocess command builder, POSIX/Windows argument escaper, preferLocal PATH resolver, and ExecaResult inspector (npm execa equivalent, reuses npm-run-path)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:execa` ([https://github.com/sindresorhus/execa](https://github.com/sindresorhus/execa))

| Metadata | Value |
| --- | --- |
| Package | `execa` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add execa
```

```cp
import {
    ExecaOptions,
    ExecaResult,
    exaPush,
    exaPushByte,
    exaNeedsQuote,
    escapePosixArg,
    formatCommand,
    stripFinalNewline,
} from "execa";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ExecaOptions` | `class` | Fields: `cwd: string, path_env: string, prefer_local: bool, reject: bool, strip_final_newline: bool, timeout_ms: u64, +1 more` · Methods: `defaults` |
| `ExecaResult` | `class` | Fields: `exit_code: i32, signal_code: i32, timed_out: bool, is_canceled: bool, failed: bool, stdout: string, +1 more` · Methods: `success, failure, timeout, isOk` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `exaPush` | `(out: []u8, pos: usize, s: string): usize` | Invokes `exaPush` and returns `usize`. |
| `exaPushByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `exaPushByte` and returns `usize`. |
| `exaNeedsQuote` | `(arg: string): bool` | Invokes `exaNeedsQuote` and returns `bool`. |
| `escapePosixArg` | `(arg: string, out: []u8): usize` | Invokes `escapePosixArg` and returns `usize`. |
| `formatCommand` | `(file: string, args: []string, out: []u8): usize` | Serializes/formats data via `formatCommand` returning `usize`. |
| `stripFinalNewline` | `(s: string): string` | Invokes `stripFinalNewline` and returns `string`. |
| `resolveExecPath` | `(opts: ExecaOptions, out: []u8): usize` | Invokes `resolveExecPath` and returns `usize`. |

## Minimal Example

```cp
import { formatCommand } from "execa";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatCommand("example", /* args: []string */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
