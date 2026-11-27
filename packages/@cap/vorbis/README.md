[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/vorbis

Xiph libvorbis 1.3.7 royalty-free audio codec (self-contained with libogg) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`vorbis`](https://github.com/xiph/vorbis)（`BSD-3-Clause`）（包名 `@cap/vorbis`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/vorbis` |
| Version | `1.3.7` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/vorbis
```

```cp
import {
    VorbisError,
    cp_vorbis_version_string,
    cp_vorbis_encoder_test_init,
    versionString,
    encoderTestInit,
} from "@cap/vorbis";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `VorbisError` | `InvalidParams, InitFailed` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_vorbis_version_string` | `(): [*:0]&u8!` | Invokes `cp_vorbis_version_string` and returns `[*:0]&u8!`. |
| `cp_vorbis_encoder_test_init` | `(channels: i32, rate: isize, quality: f32): i32` | Invokes `cp_vorbis_encoder_test_init` and returns `i32`. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |
| `encoderTestInit` | `(channels: i32, rate: isize, quality: f32): !void` | Serializes/formats data via `encoderTestInit` returning `!void`. |

## Minimal Example

```cp
import { versionString } from "@cap/vorbis";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
