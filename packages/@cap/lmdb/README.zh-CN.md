[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/lmdb

Lightning Memory-Mapped Database (LMDB 0.9.33) zero-copy KV engine for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/lmdb` |
| 版本 | `0.9.33` |
| 许可证 | `OLDAP-2.8` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/lmdb
```

```cp
import {
    LmdbError,
    MDB_val,
    LmdbTxn,
    LmdbEnv,
    mdb_version,
    mdb_strerror,
    mdb_env_create,
    mdb_env_set_mapsize,
} from "@cap/lmdb";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `LmdbError` | `EnvCreateFailed, EnvOpenFailed, MapSizeFailed, TxnBeginFailed, TxnCommitFailed, DbiOpenFailed, PutFailed, DelFailed ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MDB_val` | `class` | 字段: `mv_size: usize, mv_data: *auto!?` |
| `LmdbTxn` | `class` | 字段: `txn_ptr: *auto!?, dbi: u32` · 方法: `put, get, del, commit, abort, __cp_deinit` |
| `LmdbEnv` | `class` | 字段: `env_ptr: *auto!?` · 方法: `open, beginTxn, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mdb_version` | `(major: *i32!?, minor: *i32!?, patch: *i32!?): [*:0]&u8!` | 调用 `mdb_version` 并返回 `[*:0]&u8!`。 |
| `mdb_strerror` | `(err: i32): [*:0]&u8!` | 调用 `mdb_strerror` 并返回 `[*:0]&u8!`。 |
| `mdb_env_create` | `(env: **auto!?!): i32` | 调用 `mdb_env_create` 并返回 `i32`。 |
| `mdb_env_set_mapsize` | `(env: *auto!?, size: usize): i32` | 调用 `mdb_env_set_mapsize` 并返回 `i32`。 |
| `mdb_env_set_maxdbs` | `(env: *auto!?, dbs: u32): i32` | 调用 `mdb_env_set_maxdbs` 并返回 `i32`。 |
| `mdb_env_open` | `(env: *auto!?, path: [*:0]&u8!, flags: u32, mode: u32): i32` | 调用 `mdb_env_open` 并返回 `i32`。 |
| `mdb_env_close` | `(env: *auto!?)` | 就地执行 `mdb_env_close` 操作。 |
| `mdb_env_sync` | `(env: *auto!?, force: i32): i32` | 调用 `mdb_env_sync` 并返回 `i32`。 |
| `mdb_txn_begin` | `(env: *auto!?, parent: *auto!?, flags: u32, txn: **auto!?!): i32` | 调用 `mdb_txn_begin` 并返回 `i32`。 |
| `mdb_txn_commit` | `(txn: *auto!?): i32` | 调用 `mdb_txn_commit` 并返回 `i32`。 |
| `mdb_txn_abort` | `(txn: *auto!?)` | 就地执行 `mdb_txn_abort` 操作。 |
| `mdb_dbi_open` | `(txn: *auto!?, name: [*:0]&u8!?, flags: u32, dbi: *u32!): i32` | 调用 `mdb_dbi_open` 并返回 `i32`。 |
| `mdb_get` | `(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!): i32` | 调用 `mdb_get` 并返回 `i32`。 |
| `mdb_put` | `(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!, flags: u32): i32` | 调用 `mdb_put` 并返回 `i32`。 |
| `mdb_del` | `(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!?): i32` | 调用 `mdb_del` 并返回 `i32`。 |
| `versionString` | `(): string` | 调用 `versionString` 并返回 `string`。 |

## 最小调用示例

```cp
import { versionString } from "@cap/lmdb";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
