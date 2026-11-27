# @cap/yyjson — AI Agent Quick Ref

High performance JSON library (yyjson v0.10.0) native C binding for CP (`v0.10.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
error YyjsonError { ParseFailed, WriteFailed, OutOfMemory }
class JsonVal { raw: *auto!? } // methods: isNull(...) +14
class JsonDoc { doc_ptr: *auto!? } // methods: parse(...) +6
```

## API Signatures

```cp
fn cp_yyjson_read(dat: [*]&u8!, len: usize, flg: u32): *auto!?;
fn cp_yyjson_doc_free(doc: *auto!?);
fn cp_yyjson_doc_get_root(doc: *auto!?): *auto!?;
fn cp_yyjson_doc_get_read_size(doc: *auto!?): usize;
fn cp_yyjson_doc_get_val_count(doc: *auto!?): usize;
fn cp_yyjson_write(doc: *auto!?, flg: u32, len: *usize!?): [*]u8!?;
fn cp_yyjson_is_null(val: *auto!?): bool;
fn cp_yyjson_is_bool(val: *auto!?): bool;
fn cp_yyjson_is_int(val: *auto!?): bool;
fn cp_yyjson_is_real(val: *auto!?): bool;
fn cp_yyjson_is_str(val: *auto!?): bool;
fn cp_yyjson_is_arr(val: *auto!?): bool;
fn cp_yyjson_is_obj(val: *auto!?): bool;
fn cp_yyjson_get_bool(val: *auto!?): bool;
fn cp_yyjson_get_sint(val: *auto!?): i64;
fn cp_yyjson_get_real(val: *auto!?): f64;
fn cp_yyjson_get_str(val: *auto!?): [*:0]&u8!?;
fn cp_yyjson_get_len(val: *auto!?): usize;
fn cp_yyjson_obj_size(obj: *auto!?): usize;
fn cp_yyjson_obj_getn(obj: *auto!?, key: [*]&u8!, key_len: usize): *auto!?;
fn cp_yyjson_arr_size(arr: *auto!?): usize;
fn cp_yyjson_arr_get(arr: *auto!?, idx: usize): *auto!?;
fn cp_yyjson_free_str(ptr: *auto!?);
```

## Minimal Usage

```cp
import { cp_yyjson_read } from "@cap/yyjson";

pub fn main() {
    let input = "example";
    let result = cp_yyjson_read(input.ptr, 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/yyjson"`; install via `cap add @cap/yyjson`.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
