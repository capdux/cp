// @cap/duckdb — DuckDB (v1.2.1) In-Process Analytical SQL OLAP Database for CP
// License: MIT (Upstream: duckdb/duckdb)

import std from "std";

pub const DUCKDB_SUCCESS: i32 = 0;
pub const DUCKDB_ERROR: i32 = 1;

pub @c {
    // Core DuckDB C API (resolved dynamically from libduckdb.so)
    fn duckdb_open(path: [*:0]const u8?, out_database: *(*!?)!): i32;
    fn duckdb_close(database: *(*!?)!);
    fn duckdb_connect(database: *!?, out_connection: *(*!?)!): i32;
    fn duckdb_disconnect(connection: *(*!?)!);
    fn duckdb_query(connection: *!?, query: [*:0]const u8!, out_result: *!?): i32;
    fn duckdb_destroy_result(result: *!?);
    fn duckdb_row_count(result: *!?): u64;
    fn duckdb_column_count(result: *!?): u64;
    fn duckdb_value_int64(result: *!?, col: u64, row: u64): i64;
    fn duckdb_value_double(result: *!?, col: u64, row: u64): f64;
    fn duckdb_value_varchar(result: *!?, col: u64, row: u64): [*:0]u8?;
    fn duckdb_free(ptr: *!?);
    fn duckdb_library_version(): [*:0]const u8;

    // Minimal heap-allocated duckdb_result ABI shim (libduckdb_shim.a)
    fn cp_duckdb_query(conn: *!?, sql: [*:0]const u8!, out_state: *i32!): *!?;
    fn cp_duckdb_row_count(res: *!?): u64;
    fn cp_duckdb_col_count(res: *!?): u64;
    fn cp_duckdb_val_i64(res: *!?, col: u64, row: u64): i64;
    fn cp_duckdb_val_f64(res: *!?, col: u64, row: u64): f64;
    fn cp_duckdb_val_str(res: *!?, col: u64, row: u64): [*:0]u8?;
    fn cp_duckdb_free_str(ptr: [*:0]u8?);
    fn cp_duckdb_result_free(res: *!?);
}

pub err DuckError {
    OpenFailed,
    ConnectFailed,
    QueryFailed,
    ClosedHandle,
    OutOfMemory,
}

/// Returns the linked DuckDB shared library version string (e.g. "v1.2.1").
pub fn libraryVersion(): string {
    unsafe {
        return std.mem.span(duckdb_library_version());
    }
}

/// Owned RAII wrapper for a string returned by `duckdb_value_varchar`.
/// Automatically calls `duckdb_free()` on scope exit.
pub struct DuckString {
    ptr: [*:0]u8? = null,

    pub fn asSlice(): string {
        if (p := _.ptr) {
            unsafe {
                return std.mem.span(p);
            }
        }
        return "";
    }

    ~init() {
        if (p := _.ptr) {
            _.ptr = null;
            unsafe {
                cp_duckdb_free_str(p);
            }
        }
    }
}

/// Owned RAII wrapper around a heap-allocated `duckdb_result`.
/// Automatically destroys and frees the result on scope exit via `~init()`.
pub struct DuckResult {
    res_ptr: *!? = null,

    pub fn rowCount(): u64 {
        if (r := _.res_ptr) {
            unsafe {
                return cp_duckdb_row_count(r);
            }
        }
        return 0;
    }

    pub fn colCount(): u64 {
        if (r := _.res_ptr) {
            unsafe {
                return cp_duckdb_col_count(r);
            }
        }
        return 0;
    }

    pub fn valueI64(col: u64, row: u64): i64 {
        if (r := _.res_ptr) {
            unsafe {
                return cp_duckdb_val_i64(r, col, row);
            }
        }
        return 0;
    }

    pub fn valueF64(col: u64, row: u64): f64 {
        if (r := _.res_ptr) {
            unsafe {
                return cp_duckdb_val_f64(r, col, row);
            }
        }
        return 0.0;
    }

    pub fn valueString(col: u64, row: u64): DuckString {
        if (r := _.res_ptr) {
            unsafe {
                return DuckString{ .ptr = cp_duckdb_val_str(r, col, row) };
            }
        }
        return DuckString{ .ptr = null };
    }

    ~init() {
        if (r := _.res_ptr) {
            _.res_ptr = null;
            unsafe {
                cp_duckdb_result_free(r);
            }
        }
    }
}

/// Owned RAII wrapper around a `duckdb_connection`.
/// Automatically disconnects on scope exit via `~init()`.
pub struct DuckConn {
    conn_ptr: *!? = null,

    /// Executes a SQL statement and returns a RAII `DuckResult`.
    pub fn query(sql: string): DuckResult! {
        if (c := _.conn_ptr) {
            const alloc = std.heap.page_allocator;
            const sql_z = alloc.allocSentinel(u8, sql.len, 0) catch {
                return DuckError.OutOfMemory;
            };
            defer alloc.free(sql_z);
            @memcpy(sql_z[0..sql.len], sql);

            let state: i32 = DUCKDB_ERROR;
            let res: *!? = null;
            unsafe {
                res = cp_duckdb_query(c, sql_z.ptr, &state);
            }
            if (state != DUCKDB_SUCCESS or res == null) {
                return DuckError.QueryFailed;
            }
            return DuckResult{ .res_ptr = res };
        }
        return DuckError.ClosedHandle;
    }

    /// Executes a SQL statement when no result rows need to be inspected (DDL/DML).
    pub fn exec(sql: string): void! {
        let res = try _.query(sql);
        * = res.rowCount();
    }

    ~init() {
        if (_.conn_ptr != null) {
            let c = _.conn_ptr;
            _.conn_ptr = null;
            unsafe {
                duckdb_disconnect(&c);
            }
        }
    }
}

/// Owned RAII wrapper around a `duckdb_database`.
/// Automatically closes the database handle on scope exit via `~init()`.
pub struct DuckDatabase {
    db_ptr: *!? = null,

    /// Opens a DuckDB database at `path` (pass `":memory:"` or `""` for in-memory mode).
    pub static fn open(path: string): DuckDatabase! {
        let db: *!? = null;
        let rc: i32 = DUCKDB_ERROR;

        if (path.len == 0) {
            unsafe {
                rc = duckdb_open(null, &db);
            }
        } else {
            const alloc = std.heap.page_allocator;
            const path_z = alloc.allocSentinel(u8, path.len, 0) catch {
                return DuckError.OutOfMemory;
            };
            defer alloc.free(path_z);
            @memcpy(path_z[0..path.len], path);
            unsafe {
                rc = duckdb_open(path_z.ptr, &db);
            }
        }

        if (rc != DUCKDB_SUCCESS or db == null) {
            return DuckError.OpenFailed;
        }
        return DuckDatabase{ .db_ptr = db };
    }

    /// Opens a new connection to this database.
    pub fn connect(): DuckConn! {
        if (d := _.db_ptr) {
            let conn: *!? = null;
            let rc: i32 = DUCKDB_ERROR;
            unsafe {
                rc = duckdb_connect(d, &conn);
            }
            if (rc != DUCKDB_SUCCESS or conn == null) {
                return DuckError.ConnectFailed;
            }
            return DuckConn{ .conn_ptr = conn };
        }
        return DuckError.ClosedHandle;
    }

    ~init() {
        if (_.db_ptr != null) {
            let d = _.db_ptr;
            _.db_ptr = null;
            unsafe {
                duckdb_close(&d);
            }
        }
    }
}
