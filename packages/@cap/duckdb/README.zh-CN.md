[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/duckdb

DuckDB (v1.2.1) in-process analytical SQL OLAP database for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`duckdb`](https://github.com/duckdb/duckdb)（`MIT`）（包名 `@cap/duckdb`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/duckdb` |
| 版本 | `1.2.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

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

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `DuckError` | `OpenFailed, ConnectFailed, QueryFailed, ClosedHandle, OutOfMemory` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DuckString` | `class` | 字段: `ptr: [*:0]u8!?` · 方法: `asSlice, __cp_deinit` |
| `DuckResult` | `class` | 字段: `res_ptr: *auto!?` · 方法: `rowCount, colCount, valueI64, valueF64, valueString, __cp_deinit` |
| `DuckConn` | `class` | 字段: `conn_ptr: *auto!?` · 方法: `query, exec, __cp_deinit` |
| `DuckDatabase` | `class` | 字段: `db_ptr: *auto!?` · 方法: `open, connect, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `duckdb_open` | `(path: [*:0]&u8!?, out_database: **auto!?!): i32` | 调用 `duckdb_open` 并返回 `i32`。 |
| `duckdb_close` | `(database: **auto!?!)` | 就地执行 `duckdb_close` 操作。 |
| `duckdb_connect` | `(database: *auto!?, out_connection: **auto!?!): i32` | 调用 `duckdb_connect` 并返回 `i32`。 |
| `duckdb_disconnect` | `(connection: **auto!?!)` | 就地执行 `duckdb_disconnect` 操作。 |
| `duckdb_query` | `(connection: *auto!?, query: [*:0]&u8!, out_result: *auto!?): i32` | 调用 `duckdb_query` 并返回 `i32`。 |
| `duckdb_destroy_result` | `(result: *auto!?)` | 就地执行 `duckdb_destroy_result` 操作。 |
| `duckdb_row_count` | `(result: *auto!?): u64` | 调用 `duckdb_row_count` 并返回 `u64`。 |
| `duckdb_column_count` | `(result: *auto!?): u64` | 调用 `duckdb_column_count` 并返回 `u64`。 |
| `duckdb_value_int64` | `(result: *auto!?, col: u64, row: u64): i64` | 调用 `duckdb_value_int64` 并返回 `i64`。 |
| `duckdb_value_double` | `(result: *auto!?, col: u64, row: u64): f64` | 调用 `duckdb_value_double` 并返回 `f64`。 |
| `duckdb_value_varchar` | `(result: *auto!?, col: u64, row: u64): [*:0]u8!?` | 调用 `duckdb_value_varchar` 并返回 `[*:0]u8!?`。 |
| `duckdb_free` | `(ptr: *auto!?)` | 就地执行 `duckdb_free` 操作。 |
| `duckdb_library_version` | `(): [*:0]&u8!` | 调用 `duckdb_library_version` 并返回 `[*:0]&u8!`。 |
| `cp_duckdb_query` | `(conn: *auto!?, sql: [*:0]&u8!, out_state: *i32!): *auto!?` | 调用 `cp_duckdb_query` 并返回 `*auto!?`。 |
| `cp_duckdb_row_count` | `(res: *auto!?): u64` | 调用 `cp_duckdb_row_count` 并返回 `u64`。 |
| `cp_duckdb_col_count` | `(res: *auto!?): u64` | 调用 `cp_duckdb_col_count` 并返回 `u64`。 |
| `cp_duckdb_val_i64` | `(res: *auto!?, col: u64, row: u64): i64` | 调用 `cp_duckdb_val_i64` 并返回 `i64`。 |
| `cp_duckdb_val_f64` | `(res: *auto!?, col: u64, row: u64): f64` | 调用 `cp_duckdb_val_f64` 并返回 `f64`。 |
| `cp_duckdb_val_str` | `(res: *auto!?, col: u64, row: u64): [*:0]u8!?` | 调用 `cp_duckdb_val_str` 并返回 `[*:0]u8!?`。 |
| `cp_duckdb_free_str` | `(ptr: [*:0]u8!?)` | 就地执行 `cp_duckdb_free_str` 操作。 |
| `cp_duckdb_result_free` | `(res: *auto!?)` | 就地执行 `cp_duckdb_result_free` 操作。 |
| `libraryVersion` | `(): string` | 调用 `libraryVersion` 并返回 `string`。 |

## 最小调用示例

```cp
import { duckdb_library_version } from "@cap/duckdb";

pub fn main() {
    let result = duckdb_library_version();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
