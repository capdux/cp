# @cap/duckdb@1.2.1 (Agent Quickref)

> DuckDB (v1.2.1) in-process analytical SQL OLAP database for CP via prebuilt dynamic library | License: MIT
> Import: `import { ... } from "@cap/duckdb";` | Full types: `src/index.d.cp`

## Functions
- `export fn duckdb_open(path: [*:0]&u8!?, out_database: **auto!?!): i32;`
- `export fn duckdb_close(database: **auto!?!);`
- `export fn duckdb_connect(database: *auto!?, out_connection: **auto!?!): i32;`
- `export fn duckdb_disconnect(connection: **auto!?!);`
- `export fn duckdb_query(connection: *auto!?, query: [*:0]&u8!, out_result: *auto!?): i32;`
- `export fn duckdb_destroy_result(result: *auto!?);`
- `export fn duckdb_row_count(result: *auto!?): u64;`
- `export fn duckdb_column_count(result: *auto!?): u64;`
- `export fn duckdb_value_int64(result: *auto!?, col: u64, row: u64): i64;`
- `export fn duckdb_value_double(result: *auto!?, col: u64, row: u64): f64;`
- `export fn duckdb_value_varchar(result: *auto!?, col: u64, row: u64): [*:0]u8!?;`
- `export fn duckdb_free(ptr: *auto!?);`
- `export fn duckdb_library_version(): [*:0]&u8!;`
- `export fn cp_duckdb_query(conn: *auto!?, sql: [*:0]&u8!, out_state: *i32!): *auto!?;`
- `export fn cp_duckdb_row_count(res: *auto!?): u64;`
- `export fn cp_duckdb_col_count(res: *auto!?): u64;`
- `export fn cp_duckdb_val_i64(res: *auto!?, col: u64, row: u64): i64;`
- `export fn cp_duckdb_val_f64(res: *auto!?, col: u64, row: u64): f64;`
- `export fn cp_duckdb_val_str(res: *auto!?, col: u64, row: u64): [*:0]u8!?;`
- `export fn cp_duckdb_free_str(ptr: [*:0]u8!?);`
- `export fn cp_duckdb_result_free(res: *auto!?);`
- `export fn libraryVersion(): string;`
