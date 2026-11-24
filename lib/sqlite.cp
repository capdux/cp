// Source: Custom (cap:sqlite binding, per CHANGELOG)
import std from "std";

pub @c {
    fn sqlite3_open(filename: [*]const u8!, ppDb: *(*!?)!): i32;
    fn sqlite3_close(db: *!?): i32;
    fn sqlite3_exec(db: *!?, sql: [*]const u8!, callback: *!?, arg: *!?, errmsg: *(*!?)?!): i32;
    fn sqlite3_errmsg(db: *!?): [*:0]const u8;
    fn sqlite3_prepare_v2(db: *!?, zSql: [*]const u8!, nByte: i32, ppStmt: *(*!?)!, pzTail: *(*!?)?!): i32;
    fn sqlite3_step(stmt: *!?): i32;
    fn sqlite3_column_text(stmt: *!?, iCol: i32): [*:0]const u8?;
    fn sqlite3_finalize(stmt: *!?): i32;
}

pub err SqliteError {
    OutOfMemory,
    SQLiteOpenFailed,
    SQLiteExecFailed,
    SQLitePrepareFailed,
}

pub class Database {
    pub ptr: *!? = null,

    pub fn init(*_, path: string): void! {
        let db: *!? = null;
        const allocator = std.heap.page_allocator;
        let path_z = allocator.alloc(u8, path.len + 1) catch return SqliteError.OutOfMemory;
        @memcpy(path_z[0..path.len], path);
        path_z[path.len] = 0;
        defer allocator.free(path_z);

        if (sqlite3_open(path_z.ptr, &db) != 0) {
            if (db != null) {
                * = sqlite3_close(db);
            }
            return SqliteError.SQLiteOpenFailed;
        }
        _.ptr = db;
    }

    pub fn exec(sql: string): void! {
        const allocator = std.heap.page_allocator;
        let sql_z = allocator.alloc(u8, sql.len + 1) catch return SqliteError.OutOfMemory;
        @memcpy(sql_z[0..sql.len], sql);
        sql_z[sql.len] = 0;
        defer allocator.free(sql_z);

        if (sqlite3_exec(_.ptr, sql_z.ptr, null, null, null) != 0) {
            return SqliteError.SQLiteExecFailed;
        }
    }

    pub fn query_string(sql: string): string?! {
        const allocator = std.heap.page_allocator;
        let sql_z = allocator.alloc(u8, sql.len + 1) catch return SqliteError.OutOfMemory;
        @memcpy(sql_z[0..sql.len], sql);
        sql_z[sql.len] = 0;
        defer allocator.free(sql_z);

        let stmt: *!? = null;
        if (sqlite3_prepare_v2(_.ptr, sql_z.ptr, -1, &stmt, null) != 0) {
            return SqliteError.SQLitePrepareFailed;
        }
        defer { * = sqlite3_finalize(stmt); }

        if (sqlite3_step(stmt) == 100) {
            if (text_ptr := sqlite3_column_text(stmt, 0)) {
                const span = std.mem.span(text_ptr);
                const result = allocator.alloc(u8, span.len) catch return SqliteError.OutOfMemory;
                @memcpy(result, span);
                return result;
            }
        }
        return "";
    }

    ~init() {
        if (db_ptr := _.ptr) {
            * = sqlite3_close(db_ptr);
            _.ptr = null;
        }
    }
}
