// @cap/lmdb — Lightning Memory-Mapped Database (LMDB 0.9.33) for CP
// License: OpenLDAP Public License 2.8 (BSD-like permissive)

import std from "std";

pub @c struct MDB_val {
    mv_size: usize = 0;
    mv_data: *!? = null;
}

pub @c {
    fn mdb_version(major: *i32?!, minor: *i32?!, patch: *i32?!): [*:0]const u8;
    fn mdb_strerror(err: i32): [*:0]const u8;
    fn mdb_env_create(env: *(*!?)!): i32;
    fn mdb_env_set_mapsize(env: *!?, size: usize): i32;
    fn mdb_env_set_maxdbs(env: *!?, dbs: u32): i32;
    fn mdb_env_open(env: *!?, path: [*:0]const u8!, flags: u32, mode: u32): i32;
    fn mdb_env_close(env: *!?);
    fn mdb_env_sync(env: *!?, force: i32): i32;

    fn mdb_txn_begin(env: *!?, parent: *!?, flags: u32, txn: *(*!?)!): i32;
    fn mdb_txn_commit(txn: *!?): i32;
    fn mdb_txn_abort(txn: *!?);

    fn mdb_dbi_open(txn: *!?, name: [*:0]const u8?, flags: u32, dbi: *u32!): i32;
    fn mdb_get(txn: *!?, dbi: u32, key: *MDB_val!, data: *MDB_val!): i32;
    fn mdb_put(txn: *!?, dbi: u32, key: *MDB_val!, data: *MDB_val!, flags: u32): i32;
    fn mdb_del(txn: *!?, dbi: u32, key: *MDB_val!, data: *MDB_val?!): i32;
}

pub const MDB_SUCCESS: i32 = 0;
pub const MDB_NOTFOUND: i32 = -30798;
pub const MDB_RDONLY: u32 = 0x20000;
pub const MDB_NOSUBDIR: u32 = 0x4000;
pub const MDB_CREATE: u32 = 0x40000;

pub err LmdbError {
    EnvCreateFailed,
    EnvOpenFailed,
    MapSizeFailed,
    TxnBeginFailed,
    TxnCommitFailed,
    DbiOpenFailed,
    PutFailed,
    DelFailed,
    GetFailed,
    OutOfMemory,
}

/// Returns the linked LMDB version string.
pub fn versionString(): string {
    unsafe {
        return std.mem.span(mdb_version(null, null, null));
    }
}

/// Owned RAII LMDB transaction. Automatically aborts on scope exit if not committed.
pub struct LmdbTxn {
    txn_ptr: *!? = null,
    dbi: u32 = 0,

    pub fn put(key: []const u8, val: []const u8): void! {
        if (_.txn_ptr == null) return LmdbError.PutFailed;
        let k = MDB_val{ .mv_size = key.len, .mv_data = null };
        let v = MDB_val{ .mv_size = val.len, .mv_data = null };
        let rc: i32 = 0;
        unsafe {
            k.mv_data = @constCast(@ptrCast(key.ptr));
            v.mv_data = @constCast(@ptrCast(val.ptr));
            rc = mdb_put(_.txn_ptr, _.dbi, &k, &v, 0);
        }
        if (rc != MDB_SUCCESS) return LmdbError.PutFailed;
    }

    pub fn get(key: []const u8): string?! {
        if (_.txn_ptr == null) return LmdbError.GetFailed;
        let k = MDB_val{ .mv_size = key.len, .mv_data = null };
        let v = MDB_val{ .mv_size = 0, .mv_data = null };
        let rc: i32 = 0;
        unsafe {
            k.mv_data = @constCast(@ptrCast(key.ptr));
            rc = mdb_get(_.txn_ptr, _.dbi, &k, &v);
        }
        if (rc == MDB_NOTFOUND) return null;
        if (rc != MDB_SUCCESS) return LmdbError.GetFailed;
        if (ptr := v.mv_data) {
            unsafe {
                const bytes: [*]const u8 = @ptrCast(ptr);
                return bytes[0..v.mv_size];
            }
        }
        return "";
    }

    pub fn del(key: []const u8): bool! {
        if (_.txn_ptr == null) return LmdbError.DelFailed;
        let k = MDB_val{ .mv_size = key.len, .mv_data = null };
        let rc: i32 = 0;
        unsafe {
            k.mv_data = @constCast(@ptrCast(key.ptr));
            rc = mdb_del(_.txn_ptr, _.dbi, &k, null);
        }
        if (rc == MDB_NOTFOUND) return false;
        if (rc != MDB_SUCCESS) return LmdbError.DelFailed;
        return true;
    }

    pub fn commit(*_): void! {
        if (t := _.txn_ptr) {
            _.txn_ptr = null;
            let rc: i32 = 0;
            unsafe { rc = mdb_txn_commit(t); }
            if (rc != MDB_SUCCESS) return LmdbError.TxnCommitFailed;
        }
    }

    pub fn abort(*_) {
        if (t := _.txn_ptr) {
            _.txn_ptr = null;
            unsafe { mdb_txn_abort(t); }
        }
    }

    ~init() {
        if (t := _.txn_ptr) {
            _.txn_ptr = null;
            unsafe { mdb_txn_abort(t); }
        }
    }
}

/// Owned RAII LMDB environment. Automatically closes on scope exit.
pub struct LmdbEnv {
    env_ptr: *!? = null,

    pub static fn open(path: string, map_size_bytes: usize, flags: u32): LmdbEnv! {
        let env: *!? = null;
        let rc: i32 = 0;
        unsafe { rc = mdb_env_create(&env); }
        if (rc != MDB_SUCCESS or env == null) return LmdbError.EnvCreateFailed;

        if (map_size_bytes > 0) {
            unsafe { rc = mdb_env_set_mapsize(env, map_size_bytes); }
            if (rc != MDB_SUCCESS) {
                unsafe { mdb_env_close(env); }
                return LmdbError.MapSizeFailed;
            }
        }

        const alloc = std.heap.page_allocator;
        const path_z = alloc.allocSentinel(u8, path.len, 0) catch {
            unsafe { mdb_env_close(env); }
            return LmdbError.OutOfMemory;
        };
        defer alloc.free(path_z);
        @memcpy(path_z[0..path.len], path);

        unsafe { rc = mdb_env_open(env, path_z.ptr, flags, 0o644); }
        if (rc != MDB_SUCCESS) {
            unsafe { mdb_env_close(env); }
            return LmdbError.EnvOpenFailed;
        }
        return LmdbEnv{ .env_ptr = env };
    }

    pub fn beginTxn(readonly: bool): LmdbTxn! {
        if (_.env_ptr == null) return LmdbError.TxnBeginFailed;
        const flg: u32 = if (readonly) MDB_RDONLY else 0;
        let txn: *!? = null;
        let dbi: u32 = 0;
        let rc: i32 = 0;
        unsafe { rc = mdb_txn_begin(_.env_ptr, null, flg, &txn); }
        if (rc != MDB_SUCCESS or txn == null) return LmdbError.TxnBeginFailed;

        unsafe { rc = mdb_dbi_open(txn, null, 0, &dbi); }
        if (rc != MDB_SUCCESS) {
            unsafe { mdb_txn_abort(txn); }
            return LmdbError.DbiOpenFailed;
        }
        return LmdbTxn{ .txn_ptr = txn, .dbi = dbi };
    }

    ~init() {
        if (e := _.env_ptr) {
            _.env_ptr = null;
            unsafe { mdb_env_close(e); }
        }
    }
}
