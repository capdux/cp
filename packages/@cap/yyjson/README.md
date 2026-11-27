[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/yyjson

High performance JSON library (yyjson v0.10.0) native C binding for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/yyjson` |
| Version | `0.10.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/yyjson
```

```cp
import {
    YyjsonError,
    JsonVal,
    JsonDoc,
    cp_yyjson_read,
    cp_yyjson_doc_free,
    cp_yyjson_doc_get_root,
    cp_yyjson_doc_get_read_size,
    cp_yyjson_doc_get_val_count,
} from "@cap/yyjson";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `YyjsonError` | `ParseFailed, WriteFailed, OutOfMemory` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `JsonVal` | `class` | Fields: `raw: *auto!?` · Methods: `isNull, isBool, isInt, isReal, isStr, isArr, +9 more` |
| `JsonDoc` | `class` | Fields: `doc_ptr: *auto!?` · Methods: `parse, parseWithFlags, root, valCount, readSize, stringify, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_yyjson_read` | `(dat: [*]&u8!, len: usize, flg: u32): *auto!?` | Invokes `cp_yyjson_read` and returns `*auto!?`. |
| `cp_yyjson_doc_free` | `(doc: *auto!?)` | Executes `cp_yyjson_doc_free` in-place. |
| `cp_yyjson_doc_get_root` | `(doc: *auto!?): *auto!?` | Invokes `cp_yyjson_doc_get_root` and returns `*auto!?`. |
| `cp_yyjson_doc_get_read_size` | `(doc: *auto!?): usize` | Invokes `cp_yyjson_doc_get_read_size` and returns `usize`. |
| `cp_yyjson_doc_get_val_count` | `(doc: *auto!?): usize` | Invokes `cp_yyjson_doc_get_val_count` and returns `usize`. |
| `cp_yyjson_write` | `(doc: *auto!?, flg: u32, len: *usize!?): [*]u8!?` | Invokes `cp_yyjson_write` and returns `[*]u8!?`. |
| `cp_yyjson_is_null` | `(val: *auto!?): bool` | Invokes `cp_yyjson_is_null` and returns `bool`. |
| `cp_yyjson_is_bool` | `(val: *auto!?): bool` | Invokes `cp_yyjson_is_bool` and returns `bool`. |
| `cp_yyjson_is_int` | `(val: *auto!?): bool` | Invokes `cp_yyjson_is_int` and returns `bool`. |
| `cp_yyjson_is_real` | `(val: *auto!?): bool` | Invokes `cp_yyjson_is_real` and returns `bool`. |
| `cp_yyjson_is_str` | `(val: *auto!?): bool` | Invokes `cp_yyjson_is_str` and returns `bool`. |
| `cp_yyjson_is_arr` | `(val: *auto!?): bool` | Invokes `cp_yyjson_is_arr` and returns `bool`. |
| `cp_yyjson_is_obj` | `(val: *auto!?): bool` | Invokes `cp_yyjson_is_obj` and returns `bool`. |
| `cp_yyjson_get_bool` | `(val: *auto!?): bool` | Invokes `cp_yyjson_get_bool` and returns `bool`. |
| `cp_yyjson_get_sint` | `(val: *auto!?): i64` | Invokes `cp_yyjson_get_sint` and returns `i64`. |
| `cp_yyjson_get_real` | `(val: *auto!?): f64` | Invokes `cp_yyjson_get_real` and returns `f64`. |
| `cp_yyjson_get_str` | `(val: *auto!?): [*:0]&u8!?` | Invokes `cp_yyjson_get_str` and returns `[*:0]&u8!?`. |
| `cp_yyjson_get_len` | `(val: *auto!?): usize` | Invokes `cp_yyjson_get_len` and returns `usize`. |
| `cp_yyjson_obj_size` | `(obj: *auto!?): usize` | Invokes `cp_yyjson_obj_size` and returns `usize`. |
| `cp_yyjson_obj_getn` | `(obj: *auto!?, key: [*]&u8!, key_len: usize): *auto!?` | Invokes `cp_yyjson_obj_getn` and returns `*auto!?`. |
| `cp_yyjson_arr_size` | `(arr: *auto!?): usize` | Invokes `cp_yyjson_arr_size` and returns `usize`. |
| `cp_yyjson_arr_get` | `(arr: *auto!?, idx: usize): *auto!?` | Invokes `cp_yyjson_arr_get` and returns `*auto!?`. |
| `cp_yyjson_free_str` | `(ptr: *auto!?)` | Executes `cp_yyjson_free_str` in-place. |

## Minimal Example

```cp
import { cp_yyjson_read } from "@cap/yyjson";

pub fn main() {
    let input = "example";
    let result = cp_yyjson_read(input.ptr, 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
