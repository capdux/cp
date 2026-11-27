[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# regex

PCRE2 10.44 Perl-compatible regular expression engine with SLJIT native JIT acceleration for CP

| Metadata | Value |
| --- | --- |
| Package | `regex` |
| Version | `10.44.0` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add regex
```

```cp
import {
    Pcre2Error,
    Pcre2Match,
    Pcre2Regex,
    cp_pcre2_compile,
    cp_pcre2_match,
    cp_pcre2_free,
    cp_pcre2_is_jit,
    cp_pcre2_version,
} from "regex";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `Pcre2Error` | `CompileFailed, MatchFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Pcre2Match` | `class` | Fields: `start: usize, end: usize` · Methods: `slice` |
| `Pcre2Regex` | `class` | Fields: `raw: *auto!?` · Methods: `compile, compileJit, compileWithOptions, versionString, isJitEnabled, find, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_pcre2_compile` | `(pattern: [*]&u8!, pat_len: usize, opts: u32, jit: i32): *auto!?` | Invokes `cp_pcre2_compile` and returns `*auto!?`. |
| `cp_pcre2_match` | `(re: *auto!?, subj: [*]&u8!, subj_len: usize, start_off: usize, out_start: *usize!?, out_end: *usize!?): i32` | Invokes `cp_pcre2_match` and returns `i32`. |
| `cp_pcre2_free` | `(re: *auto!?)` | Executes `cp_pcre2_free` in-place. |
| `cp_pcre2_is_jit` | `(re: *auto!?): i32` | Invokes `cp_pcre2_is_jit` and returns `i32`. |
| `cp_pcre2_version` | `(): [*:0]&u8!?` | Invokes `cp_pcre2_version` and returns `[*:0]&u8!?`. |

## Minimal Example

```cp
import { cp_pcre2_version } from "regex";

pub fn main() {
    let result = cp_pcre2_version();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
