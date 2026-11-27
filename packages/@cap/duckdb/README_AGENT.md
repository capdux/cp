# @cap/duckdb — AI Agent Quick Ref

DuckDB (v1.2.1) in-process analytical SQL OLAP database for CP via prebuilt dynamic library (`v1.2.1`, `MIT`)

## Import

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

## Core Types & Constants

```cp
error DuckError { OpenFailed, ConnectFailed, QueryFailed, ClosedHandle, OutOfMemory }
class DuckString { ptr: [*:0]u8!? } // methods: asSlice(...) +1
class DuckResult { res_ptr: *auto!? } // methods: rowCount(...) +5
class DuckConn { conn_ptr: *auto!? } // methods: query(...) +2
class DuckDatabase { db_ptr: *auto!? } // methods: open(...) +2
```

## API Signatures

```cp
fn duckdb_open(path: [*:0]&u8!?, out_database: **auto!?!): i32;
fn duckdb_close(database: **auto!?!);
fn duckdb_connect(database: *auto!?, out_connection: **auto!?!): i32;
fn duckdb_disconnect(connection: **auto!?!);
fn duckdb_query(connection: *auto!?, query: [*:0]&u8!, out_result: *auto!?): i32;
fn duckdb_destroy_result(result: *auto!?);
fn duckdb_row_count(result: *auto!?): u64;
fn duckdb_column_count(result: *auto!?): u64;
fn duckdb_value_int64(result: *auto!?, col: u64, row: u64): i64;
fn duckdb_value_double(result: *auto!?, col: u64, row: u64): f64;
fn duckdb_value_varchar(result: *auto!?, col: u64, row: u64): [*:0]u8!?;
fn duckdb_free(ptr: *auto!?);
fn duckdb_library_version(): [*:0]&u8!;
fn cp_duckdb_query(conn: *auto!?, sql: [*:0]&u8!, out_state: *i32!): *auto!?;
fn cp_duckdb_row_count(res: *auto!?): u64;
fn cp_duckdb_col_count(res: *auto!?): u64;
fn cp_duckdb_val_i64(res: *auto!?, col: u64, row: u64): i64;
fn cp_duckdb_val_f64(res: *auto!?, col: u64, row: u64): f64;
fn cp_duckdb_val_str(res: *auto!?, col: u64, row: u64): [*:0]u8!?;
fn cp_duckdb_free_str(ptr: [*:0]u8!?);
fn cp_duckdb_result_free(res: *auto!?);
fn libraryVersion(): string;
```

## Minimal Usage

```cp
import { duckdb_library_version } from "@cap/duckdb";

pub fn main() {
    let result = duckdb_library_version();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/duckdb"`; install via `cap add @cap/duckdb`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
