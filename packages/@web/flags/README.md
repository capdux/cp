[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/flags

Official deterministic FNV-1a Feature Flags & percentage rollout plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/flags` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/flags
```

```cp
import {
    FlagsConfig,
    FeatureFlag,
    flgStrEql,
    flgInList,
    flagBucket,
    flagReset,
    flagDefine,
    flagEval,
} from "@web/flags";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FlagsConfig` | `class` | Fields: `status_path: string, user_header: string` |
| `FeatureFlag` | `class` | Fields: `name: string, enabled: bool, rollout_pct: u8, allow_users: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `flgStrEql` | `(a: string, b: string): bool` | Invokes `flgStrEql` and returns `bool`. |
| `flgInList` | `(list: string, target: string): bool` | Invokes `flgInList` and returns `bool`. |
| `flagBucket` | `(flag_name: string, user_key: string): u8` | Invokes `flagBucket` and returns `u8`. |
| `flagReset` | `()` | Executes `flagReset` in-place. |
| `flagDefine` | `(name: string, enabled: bool, rollout_pct: u8, allow_users: string): bool` | Invokes `flagDefine` and returns `bool`. |
| `flagEval` | `(flag_name: string, user_key: string): bool` | Invokes `flagEval` and returns `bool`. |
| `flgWriteRaw` | `(dst: []u8, pos: &mut usize, s: string): bool` | Invokes `flgWriteRaw` and returns `bool`. |
| `flgWriteU8` | `(dst: []u8, pos: &mut usize, v: u8): bool` | Invokes `flgWriteU8` and returns `bool`. |
| `handleFlagsStatus` | `(ctx: &mut Context): Response` | Invokes `handleFlagsStatus` and returns `Response`. |
| `createFlagsPlugin` | `(cfg: FlagsConfig): Plugin` | Constructs/initializes an instance via `createFlagsPlugin`. |
| `registerFlags` | `(app: &mut App, cfg: FlagsConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { flgStrEql } from "@web/flags";

pub fn main() {
    let result = flgStrEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
