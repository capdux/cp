[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/whisper

whisper.cpp (v1.9.6) local speech-to-text (ASR) engine for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`whisper.cpp`](https://github.com/ggml-org/whisper.cpp)（`MIT`）（原库名 `whisper.cpp`，CP 包名 `@cap/whisper`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/whisper` |
| Version | `1.9.6` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/whisper
```

```cp
import {
    WhisperError,
    WhisperContext,
    cp_whisper_version,
    cp_whisper_print_system_info,
    cp_whisper_lang_max_id,
    cp_whisper_lang_id,
    cp_whisper_lang_str,
    cp_whisper_init_from_file,
} from "@cap/whisper";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `WhisperError` | `InitFailed, OutOfMemory` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `WhisperContext` | `class` | Fields: `ctx_ptr: *auto!?` · Methods: `initFromFile, isValid, nVocab, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_whisper_version` | `(): [*:0]&u8!` | Invokes `cp_whisper_version` and returns `[*:0]&u8!`. |
| `cp_whisper_print_system_info` | `(): [*:0]&u8!` | Invokes `cp_whisper_print_system_info` and returns `[*:0]&u8!`. |
| `cp_whisper_lang_max_id` | `(): i32` | Invokes `cp_whisper_lang_max_id` and returns `i32`. |
| `cp_whisper_lang_id` | `(lang: [*:0]&u8!): i32` | Invokes `cp_whisper_lang_id` and returns `i32`. |
| `cp_whisper_lang_str` | `(id: i32): [*:0]&u8!` | Invokes `cp_whisper_lang_str` and returns `[*:0]&u8!`. |
| `cp_whisper_init_from_file` | `(path: [*:0]&u8!): *auto!?` | Invokes `cp_whisper_init_from_file` and returns `*auto!?`. |
| `cp_whisper_free` | `(ctx: *auto!?)` | Executes `cp_whisper_free` in-place. |
| `cp_whisper_n_vocab` | `(ctx: *auto!?): i32` | Invokes `cp_whisper_n_vocab` and returns `i32`. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |
| `systemInfo` | `(): string` | Invokes `systemInfo` and returns `string`. |
| `langMaxId` | `(): i32` | Invokes `langMaxId` and returns `i32`. |
| `langId` | `(code: string): i32` | Invokes `langId` and returns `i32`. |
| `langStr` | `(id: i32): string` | Invokes `langStr` and returns `string`. |

## Minimal Example

```cp
import { langId } from "@cap/whisper";

pub fn main() {
    let result = langId("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
