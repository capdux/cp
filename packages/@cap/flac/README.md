[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/flac

Xiph libFLAC 1.4.3 lossless audio encoder and decoder engine for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/flac` |
| Version | `1.4.3` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/flac
```

```cp
import {
    FlacError,
    cp_flac_version,
    cp_flac_vendor,
    cp_flac_encode_pcm16_test,
    version,
    vendor,
    encodePcm16Test,
} from "@cap/flac";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `FlacError` | `InvalidInput, InitFailed, EncodeFailed` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_flac_version` | `(): [*:0]&u8!?` | Invokes `cp_flac_version` and returns `[*:0]&u8!?`. |
| `cp_flac_vendor` | `(): [*:0]&u8!?` | Invokes `cp_flac_vendor` and returns `[*:0]&u8!?`. |
| `cp_flac_encode_pcm16_test` | `(pcm: [*]&i16!, samples: u32, sample_rate: u32, out_bytes_written: *u64!): i32` | Invokes `cp_flac_encode_pcm16_test` and returns `i32`. |
| `version` | `(): string` | Invokes `version` and returns `string`. |
| `vendor` | `(): string` | Invokes `vendor` and returns `string`. |
| `encodePcm16Test` | `(pcm: &[]i16, sample_rate: u32): !u64` | Serializes/formats data via `encodePcm16Test` returning `!u64`. |

## Minimal Example

```cp
import { version } from "@cap/flac";

pub fn main() {
    let result = version();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
