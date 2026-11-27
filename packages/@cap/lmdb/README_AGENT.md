# @cap/lmdb@0.9.33 (Agent Quickref)

> Lightning Memory-Mapped Database (LMDB 0.9.33) zero-copy KV engine for CP | License: OLDAP-2.8
> Import: `import { ... } from "@cap/lmdb";` | Full types: `src/index.d.cp`

## Functions
- `export fn mdb_version(major: *i32!?, minor: *i32!?, patch: *i32!?): [*:0]&u8!;`
- `export fn mdb_strerror(err: i32): [*:0]&u8!;`
- `export fn mdb_env_create(env: **auto!?!): i32;`
- `export fn mdb_env_set_mapsize(env: *auto!?, size: usize): i32;`
- `export fn mdb_env_set_maxdbs(env: *auto!?, dbs: u32): i32;`
- `export fn mdb_env_open(env: *auto!?, path: [*:0]&u8!, flags: u32, mode: u32): i32;`
- `export fn mdb_env_close(env: *auto!?);`
- `export fn mdb_env_sync(env: *auto!?, force: i32): i32;`
- `export fn mdb_txn_begin(env: *auto!?, parent: *auto!?, flags: u32, txn: **auto!?!): i32;`
- `export fn mdb_txn_commit(txn: *auto!?): i32;`
- `export fn mdb_txn_abort(txn: *auto!?);`
- `export fn mdb_dbi_open(txn: *auto!?, name: [*:0]&u8!?, flags: u32, dbi: *u32!): i32;`
- `export fn mdb_get(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!): i32;`
- `export fn mdb_put(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!, flags: u32): i32;`
- `export fn mdb_del(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!?): i32;`
- `export fn versionString(): string;`
