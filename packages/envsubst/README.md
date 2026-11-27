[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# envsubst

Zero-allocation POSIX / Docker / K8s environment variable template expansion engine (dotenv-expand / envsub / Rust shellexpand parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:dotenv-expand` ([https://github.com/motdotla/dotenv-expand](https://github.com/motdotla/dotenv-expand))（原库名 `dotenv-expand` → CP 包名 `envsubst`）

| Metadata | Value |
| --- | --- |
| Package | `envsubst` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add envsubst
```

```cp
import {
    ExpandError,
    EnvMap,
    ExpandResult,
    isVarStart,
    isVarCont,
    expandImpl,
    expandEnv,
    expandEnvStr,
} from "envsubst";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `ExpandError` | `None, UnclosedBrace, MissingRequired, BufferOverflow` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EnvMap` | `class` | Fields: `keys: [MAX_ENV_VARS]string, vals: [MAX_ENV_VARS]string, len: usize` · Methods: `init, set, get, has` |
| `ExpandResult` | `class` | Fields: `ok: bool, err: ExpandError, len: usize, missing_var: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isVarStart` | `(c: u8): bool` | Checks predicate `isVarStart` and returns `bool`. |
| `isVarCont` | `(c: u8): bool` | Checks predicate `isVarCont` and returns `bool`. |
| `expandImpl` | `(tpl: string, env: EnvMap, out: []u8, start_out: usize, depth: u8): ExpandResult` | Invokes `expandImpl` and returns `ExpandResult`. |
| `expandEnv` | `(tpl: string, env: EnvMap, out: []u8): ExpandResult` | Invokes `expandEnv` and returns `ExpandResult`. |
| `expandEnvStr` | `(tpl: string, env: EnvMap, out: []u8): string` | Invokes `expandEnvStr` and returns `string`. |
| `hasEnvVars` | `(tpl: string): bool` | Checks predicate `hasEnvVars` and returns `bool`. |

## Minimal Example

```cp
import { expandEnv } from "envsubst";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = expandEnv("example", /* env: EnvMap */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
