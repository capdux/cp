[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/sdl3

SDL3 (v3.2.8) cross-platform window, audio, input & GPU abstraction for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`SDL`](https://github.com/libsdl-org/SDL)（`Zlib`）（原库名 `SDL`，CP 包名 `@cap/sdl3`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/sdl3` |
| Version | `3.2.8` |
| License | `Zlib` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/sdl3
```

```cp
import {
    SdlError,
    SdlVersion,
    SdlContext,
    cp_sdl_version,
    cp_sdl_revision,
    cp_sdl_cpu_count,
    cp_sdl_system_ram,
    cp_sdl_init_headless,
} from "@cap/sdl3";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `SdlError` | `InitFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SdlVersion` | `class` | Fields: `major: i32, minor: i32, micro: i32` |
| `SdlContext` | `class` | Fields: `active: bool` · Methods: `initHeadless, isActive, ticksNs, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_sdl_version` | `(): i32` | Invokes `cp_sdl_version` and returns `i32`. |
| `cp_sdl_revision` | `(): [*:0]&u8!` | Invokes `cp_sdl_revision` and returns `[*:0]&u8!`. |
| `cp_sdl_cpu_count` | `(): i32` | Invokes `cp_sdl_cpu_count` and returns `i32`. |
| `cp_sdl_system_ram` | `(): i32` | Invokes `cp_sdl_system_ram` and returns `i32`. |
| `cp_sdl_init_headless` | `(): i32` | Invokes `cp_sdl_init_headless` and returns `i32`. |
| `cp_sdl_quit` | `()` | Executes `cp_sdl_quit` in-place. |
| `cp_sdl_ticks_ns` | `(): u64` | Invokes `cp_sdl_ticks_ns` and returns `u64`. |
| `rawVersion` | `(): i32` | Invokes `rawVersion` and returns `i32`. |
| `version` | `(): SdlVersion` | Invokes `version` and returns `SdlVersion`. |
| `revision` | `(): string` | Invokes `revision` and returns `string`. |
| `cpuCount` | `(): i32` | Invokes `cpuCount` and returns `i32`. |
| `systemRamMB` | `(): i32` | Invokes `systemRamMB` and returns `i32`. |
| `ticksNs` | `(): u64` | Invokes `ticksNs` and returns `u64`. |

## Minimal Example

```cp
import { version } from "@cap/sdl3";

pub fn main() {
    let result = version();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
