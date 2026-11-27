# @cap/lmdb — AI Agent Quick Ref

Lightning Memory-Mapped Database (LMDB 0.9.33) zero-copy KV engine for CP (`v0.9.33`, `OLDAP-2.8`)

## Import

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

## Core Types & Constants

```cp
error LmdbError { EnvCreateFailed, EnvOpenFailed, MapSizeFailed, TxnBeginFailed, TxnCommitFailed, DbiOpenFailed }
class MDB_val { mv_size: usize, mv_data: *auto!? }
class LmdbTxn { txn_ptr: *auto!?, dbi: u32 } // methods: put(...) +5
class LmdbEnv { env_ptr: *auto!? } // methods: open(...) +2
```

## API Signatures

```cp
fn mdb_version(major: *i32!?, minor: *i32!?, patch: *i32!?): [*:0]&u8!;
fn mdb_strerror(err: i32): [*:0]&u8!;
fn mdb_env_create(env: **auto!?!): i32;
fn mdb_env_set_mapsize(env: *auto!?, size: usize): i32;
fn mdb_env_set_maxdbs(env: *auto!?, dbs: u32): i32;
fn mdb_env_open(env: *auto!?, path: [*:0]&u8!, flags: u32, mode: u32): i32;
fn mdb_env_close(env: *auto!?);
fn mdb_env_sync(env: *auto!?, force: i32): i32;
fn mdb_txn_begin(env: *auto!?, parent: *auto!?, flags: u32, txn: **auto!?!): i32;
fn mdb_txn_commit(txn: *auto!?): i32;
fn mdb_txn_abort(txn: *auto!?);
fn mdb_dbi_open(txn: *auto!?, name: [*:0]&u8!?, flags: u32, dbi: *u32!): i32;
fn mdb_get(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!): i32;
fn mdb_put(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!, flags: u32): i32;
fn mdb_del(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!?): i32;
fn versionString(): string;
```

## Minimal Usage

```cp
import { versionString } from "@cap/lmdb";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/lmdb"`; install via `cap add @cap/lmdb`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
