[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# env-paths

Zero-allocation cross-platform application directory resolver (Linux XDG, macOS Library, Windows AppData) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:env-paths` ([https://github.com/sindresorhus/env-paths](https://github.com/sindresorhus/env-paths), `MIT`)

| Metadata | Value |
| --- | --- |
| Package | `env-paths` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add env-paths
```

```cp
import {
    EnvPlatform,
    EnvPathsInput,
    evpTrimTrailingSep,
    evpAppend,
    evpAppendByte,
    evpAppendAppName,
    evpJoinUnix2,
    evpJoinUnix3,
} from "env-paths";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `EnvPlatform` | `Linux, MacOS, Windows` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EnvPathsInput` | `class` | Fields: `platform: EnvPlatform, home: string, xdg_data_home: string, xdg_config_home: string, xdg_cache_home: string, xdg_state_home: string, +4 more` · Methods: `new, linux, macos, windows` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `evpTrimTrailingSep` | `(s: string, sep: u8): string` | Invokes `evpTrimTrailingSep` and returns `string`. |
| `evpAppend` | `(out: []u8, pos: usize, s: string): usize` | Invokes `evpAppend` and returns `usize`. |
| `evpAppendByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `evpAppendByte` and returns `usize`. |
| `evpAppendAppName` | `(out: []u8, pos: usize, name: string, suffix: string): usize` | Invokes `evpAppendAppName` and returns `usize`. |
| `evpJoinUnix2` | `(out: []u8, base: string, name: string, suffix: string): usize` | Invokes `evpJoinUnix2` and returns `usize`. |
| `evpJoinUnix3` | `(out: []u8, base: string, mid: string, name: string, suffix: string): usize` | Invokes `evpJoinUnix3` and returns `usize`. |
| `evpWinAppBase` | `(out: []u8, explicit: string, home: string, fallback_sub: string): usize` | Invokes `evpWinAppBase` and returns `usize`. |
| `evpWinDir` | `(out: []u8, explicit: string, home: string, fallback_sub: string, name: string, suffix: string, leaf: string): usize` | Invokes `evpWinDir` and returns `usize`. |
| `envDataDir` | `(name: string, inp: EnvPathsInput, out: []u8): usize` | Invokes `envDataDir` and returns `usize`. |
| `envConfigDir` | `(name: string, inp: EnvPathsInput, out: []u8): usize` | Invokes `envConfigDir` and returns `usize`. |
| `envCacheDir` | `(name: string, inp: EnvPathsInput, out: []u8): usize` | Invokes `envCacheDir` and returns `usize`. |
| `envLogDir` | `(name: string, inp: EnvPathsInput, out: []u8): usize` | Invokes `envLogDir` and returns `usize`. |
| `envTempDir` | `(name: string, inp: EnvPathsInput, out: []u8): usize` | Invokes `envTempDir` and returns `usize`. |

## Minimal Example

```cp
import { evpTrimTrailingSep } from "env-paths";

pub fn main() {
    let result = evpTrimTrailingSep("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
