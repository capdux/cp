[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/lmdb

Lightning Memory-Mapped Database (LMDB 0.9.33) zero-copy KV engine for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/lmdb` |
| Version | `0.9.33` |
| License | `OLDAP-2.8` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `LmdbError` | `EnvCreateFailed, EnvOpenFailed, MapSizeFailed, TxnBeginFailed, TxnCommitFailed, DbiOpenFailed, PutFailed, DelFailed ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MDB_val` | `class` | Fields: `mv_size: usize, mv_data: *auto!?` |
| `LmdbTxn` | `class` | Fields: `txn_ptr: *auto!?, dbi: u32` · Methods: `put, get, del, commit, abort, __cp_deinit` |
| `LmdbEnv` | `class` | Fields: `env_ptr: *auto!?` · Methods: `open, beginTxn, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mdb_version` | `(major: *i32!?, minor: *i32!?, patch: *i32!?): [*:0]&u8!` | Invokes `mdb_version` and returns `[*:0]&u8!`. |
| `mdb_strerror` | `(err: i32): [*:0]&u8!` | Invokes `mdb_strerror` and returns `[*:0]&u8!`. |
| `mdb_env_create` | `(env: **auto!?!): i32` | Invokes `mdb_env_create` and returns `i32`. |
| `mdb_env_set_mapsize` | `(env: *auto!?, size: usize): i32` | Invokes `mdb_env_set_mapsize` and returns `i32`. |
| `mdb_env_set_maxdbs` | `(env: *auto!?, dbs: u32): i32` | Invokes `mdb_env_set_maxdbs` and returns `i32`. |
| `mdb_env_open` | `(env: *auto!?, path: [*:0]&u8!, flags: u32, mode: u32): i32` | Invokes `mdb_env_open` and returns `i32`. |
| `mdb_env_close` | `(env: *auto!?)` | Executes `mdb_env_close` in-place. |
| `mdb_env_sync` | `(env: *auto!?, force: i32): i32` | Invokes `mdb_env_sync` and returns `i32`. |
| `mdb_txn_begin` | `(env: *auto!?, parent: *auto!?, flags: u32, txn: **auto!?!): i32` | Invokes `mdb_txn_begin` and returns `i32`. |
| `mdb_txn_commit` | `(txn: *auto!?): i32` | Invokes `mdb_txn_commit` and returns `i32`. |
| `mdb_txn_abort` | `(txn: *auto!?)` | Executes `mdb_txn_abort` in-place. |
| `mdb_dbi_open` | `(txn: *auto!?, name: [*:0]&u8!?, flags: u32, dbi: *u32!): i32` | Invokes `mdb_dbi_open` and returns `i32`. |
| `mdb_get` | `(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!): i32` | Invokes `mdb_get` and returns `i32`. |
| `mdb_put` | `(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!, flags: u32): i32` | Invokes `mdb_put` and returns `i32`. |
| `mdb_del` | `(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!?): i32` | Invokes `mdb_del` and returns `i32`. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |

## Minimal Example

```cp
import { versionString } from "@cap/lmdb";

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
