[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/utf8proc

Clean Unicode 16.0 normalization (NFC/NFD/NFKC/NFKD), case-folding & charwidth library for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/utf8proc` |
| Version | `2.10.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/utf8proc
```

```cp
import {
    Utf8ProcError,
    NormString,
    utf8proc_version,
    utf8proc_unicode_version,
    utf8proc_charwidth,
    utf8proc_tolower,
    utf8proc_toupper,
    utf8proc_NFC,
} from "@cap/utf8proc";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `Utf8ProcError` | `NormalizeFailed, OutOfMemory` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `NormString` | `class` | Fields: `raw: [*:0]u8!?, len: usize` · Methods: `nfc, nfd, nfkc, nfkcCasefold, runOp, slice, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `utf8proc_version` | `(): [*:0]&u8!` | Invokes `utf8proc_version` and returns `[*:0]&u8!`. |
| `utf8proc_unicode_version` | `(): [*:0]&u8!` | Invokes `utf8proc_unicode_version` and returns `[*:0]&u8!`. |
| `utf8proc_charwidth` | `(codepoint: i32): i32` | Invokes `utf8proc_charwidth` and returns `i32`. |
| `utf8proc_tolower` | `(c: i32): i32` | Invokes `utf8proc_tolower` and returns `i32`. |
| `utf8proc_toupper` | `(c: i32): i32` | Invokes `utf8proc_toupper` and returns `i32`. |
| `utf8proc_NFC` | `(str: [*:0]&u8!): [*:0]u8!?` | Invokes `utf8proc_NFC` and returns `[*:0]u8!?`. |
| `utf8proc_NFD` | `(str: [*:0]&u8!): [*:0]u8!?` | Invokes `utf8proc_NFD` and returns `[*:0]u8!?`. |
| `utf8proc_NFKC` | `(str: [*:0]&u8!): [*:0]u8!?` | Invokes `utf8proc_NFKC` and returns `[*:0]u8!?`. |
| `utf8proc_NFKD` | `(str: [*:0]&u8!): [*:0]u8!?` | Invokes `utf8proc_NFKD` and returns `[*:0]u8!?`. |
| `utf8proc_NFKC_Casefold` | `(str: [*:0]&u8!): [*:0]u8!?` | Invokes `utf8proc_NFKC_Casefold` and returns `[*:0]u8!?`. |
| `free` | `(ptr: *auto!?)` | Releases native resources allocated by this module. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |
| `unicodeVersion` | `(): string` | Invokes `unicodeVersion` and returns `string`. |
| `charWidth` | `(cp: i32): i32` | Invokes `charWidth` and returns `i32`. |
| `toLower` | `(cp: i32): i32` | Invokes `toLower` and returns `i32`. |
| `toUpper` | `(cp: i32): i32` | Invokes `toUpper` and returns `i32`. |

## Minimal Example

```cp
import { versionString } from "@cap/utf8proc";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
