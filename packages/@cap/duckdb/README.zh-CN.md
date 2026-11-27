[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/duckdb

> 版本：`1.2.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

DuckDB (v1.2.1) in-process analytical SQL OLAP database for CP via prebuilt dynamic library

## 安装与导入

```bash
cap add @cap/duckdb
```

```cp
import { duckdb_open } from "@cap/duckdb";
```

## 核心导出 API

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

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c++` (`duckdb@1.2.1`)，许可证：MIT
