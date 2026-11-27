[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/duckdb

DuckDB (v1.2.1) in-process analytical SQL OLAP database for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`duckdb`](https://github.com/duckdb/duckdb)（`MIT`）（包名 `@cap/duckdb`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/duckdb` |
| Version | `1.2.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/duckdb
```

```cp
import {
    DuckError,
    DuckString,
    DuckResult,
    DuckConn,
    DuckDatabase,
    duckdb_open,
    duckdb_close,
    duckdb_connect,
} from "@cap/duckdb";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `DuckError` | `OpenFailed, ConnectFailed, QueryFailed, ClosedHandle, OutOfMemory` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DuckString` | `class` | Fields: `ptr: [*:0]u8!?` · Methods: `asSlice, __cp_deinit` |
| `DuckResult` | `class` | Fields: `res_ptr: *auto!?` · Methods: `rowCount, colCount, valueI64, valueF64, valueString, __cp_deinit` |
| `DuckConn` | `class` | Fields: `conn_ptr: *auto!?` · Methods: `query, exec, __cp_deinit` |
| `DuckDatabase` | `class` | Fields: `db_ptr: *auto!?` · Methods: `open, connect, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `duckdb_open` | `(path: [*:0]&u8!?, out_database: **auto!?!): i32` | Invokes `duckdb_open` and returns `i32`. |
| `duckdb_close` | `(database: **auto!?!)` | Executes `duckdb_close` in-place. |
| `duckdb_connect` | `(database: *auto!?, out_connection: **auto!?!): i32` | Invokes `duckdb_connect` and returns `i32`. |
| `duckdb_disconnect` | `(connection: **auto!?!)` | Executes `duckdb_disconnect` in-place. |
| `duckdb_query` | `(connection: *auto!?, query: [*:0]&u8!, out_result: *auto!?): i32` | Invokes `duckdb_query` and returns `i32`. |
| `duckdb_destroy_result` | `(result: *auto!?)` | Executes `duckdb_destroy_result` in-place. |
| `duckdb_row_count` | `(result: *auto!?): u64` | Invokes `duckdb_row_count` and returns `u64`. |
| `duckdb_column_count` | `(result: *auto!?): u64` | Invokes `duckdb_column_count` and returns `u64`. |
| `duckdb_value_int64` | `(result: *auto!?, col: u64, row: u64): i64` | Invokes `duckdb_value_int64` and returns `i64`. |
| `duckdb_value_double` | `(result: *auto!?, col: u64, row: u64): f64` | Invokes `duckdb_value_double` and returns `f64`. |
| `duckdb_value_varchar` | `(result: *auto!?, col: u64, row: u64): [*:0]u8!?` | Invokes `duckdb_value_varchar` and returns `[*:0]u8!?`. |
| `duckdb_free` | `(ptr: *auto!?)` | Executes `duckdb_free` in-place. |
| `duckdb_library_version` | `(): [*:0]&u8!` | Invokes `duckdb_library_version` and returns `[*:0]&u8!`. |
| `cp_duckdb_query` | `(conn: *auto!?, sql: [*:0]&u8!, out_state: *i32!): *auto!?` | Invokes `cp_duckdb_query` and returns `*auto!?`. |
| `cp_duckdb_row_count` | `(res: *auto!?): u64` | Invokes `cp_duckdb_row_count` and returns `u64`. |
| `cp_duckdb_col_count` | `(res: *auto!?): u64` | Invokes `cp_duckdb_col_count` and returns `u64`. |
| `cp_duckdb_val_i64` | `(res: *auto!?, col: u64, row: u64): i64` | Invokes `cp_duckdb_val_i64` and returns `i64`. |
| `cp_duckdb_val_f64` | `(res: *auto!?, col: u64, row: u64): f64` | Invokes `cp_duckdb_val_f64` and returns `f64`. |
| `cp_duckdb_val_str` | `(res: *auto!?, col: u64, row: u64): [*:0]u8!?` | Invokes `cp_duckdb_val_str` and returns `[*:0]u8!?`. |
| `cp_duckdb_free_str` | `(ptr: [*:0]u8!?)` | Executes `cp_duckdb_free_str` in-place. |
| `cp_duckdb_result_free` | `(res: *auto!?)` | Executes `cp_duckdb_result_free` in-place. |
| `libraryVersion` | `(): string` | Invokes `libraryVersion` and returns `string`. |

## Minimal Example

```cp
import { duckdb_library_version } from "@cap/duckdb";

pub fn main() {
    let result = duckdb_library_version();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
