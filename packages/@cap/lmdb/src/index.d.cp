// Type definitions for CP

import {  } from "std";
export class MDB_val {
    mv_size: usize,
    mv_data: *auto!?,
}

export fn mdb_version(major: *i32!?, minor: *i32!?, patch: *i32!?): [*:0]&u8!;
export fn mdb_strerror(err: i32): [*:0]&u8!;
export fn mdb_env_create(env: **auto!?!): i32;
export fn mdb_env_set_mapsize(env: *auto!?, size: usize): i32;
export fn mdb_env_set_maxdbs(env: *auto!?, dbs: u32): i32;
export fn mdb_env_open(env: *auto!?, path: [*:0]&u8!, flags: u32, mode: u32): i32;
export fn mdb_env_close(env: *auto!?);
export fn mdb_env_sync(env: *auto!?, force: i32): i32;
export fn mdb_txn_begin(env: *auto!?, parent: *auto!?, flags: u32, txn: **auto!?!): i32;
export fn mdb_txn_commit(txn: *auto!?): i32;
export fn mdb_txn_abort(txn: *auto!?);
export fn mdb_dbi_open(txn: *auto!?, name: [*:0]&u8!?, flags: u32, dbi: *u32!): i32;
export fn mdb_get(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!): i32;
export fn mdb_put(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!, flags: u32): i32;
export fn mdb_del(txn: *auto!?, dbi: u32, key: *MDB_val!, data: *MDB_val!?): i32;
export error LmdbError { EnvCreateFailed, EnvOpenFailed, MapSizeFailed, TxnBeginFailed, TxnCommitFailed, DbiOpenFailed, PutFailed, DelFailed, GetFailed, OutOfMemory }

export fn versionString(): string;
export class LmdbTxn {
    txn_ptr: *auto!?,
    dbi: u32,
    fn put(key: &[]u8, val: &[]u8): !void;
    fn get(key: &[]u8): !string?;
    fn del(key: &[]u8): !bool;
    fn commit(_: auto): !void;
    fn abort(_: auto);
    fn __cp_deinit(_: &mut LmdbTxn);
}

export class LmdbEnv {
    env_ptr: *auto!?,
    fn open(path: string, map_size_bytes: usize, flags: u32): !LmdbEnv;
    fn beginTxn(readonly: bool): !LmdbTxn;
    fn __cp_deinit(_: &mut LmdbEnv);
}

