// Source: Absorbed from /data/dev/bun/src/sql/ | Synced: 2026-11-21T11:59:40Z
// ============================================================================
// Absorbed from Bun: bun/src/sql/sqlite.cp
// Upstream Sync: Refer to bun/src/sql/sqlite.zig for future upstream updates.
// ============================================================================
// lib/sql/sqlite.cp
// High-performance embedded SQLite database engine wrapper
// Pure CP modern syntax with RAII cleanup and zero-copy queries

zig {
    pub extern "c" fn sqlite3_open(filename: [*c]const u8, ppDb: *?*anyopaque) c_int;
    pub extern "c" fn sqlite3_close(db: ?*anyopaque) c_int;
    pub extern "c" fn sqlite3_exec(db: ?*anyopaque, sql: [*c]const u8, callback: ?*anyopaque, arg: ?*anyopaque, errmsg: ?*?*anyopaque) c_int;
    pub extern "c" fn sqlite3_errmsg(db: ?*anyopaque) [*c]const u8;
    pub extern "c" fn sqlite3_prepare_v2(db: ?*anyopaque, zSql: [*c]const u8, nByte: c_int, ppStmt: *?*anyopaque, pzTail: ?*?*anyopaque) c_int;
    pub extern "c" fn sqlite3_step(stmt: ?*anyopaque) c_int;
    pub extern "c" fn sqlite3_column_text(stmt: ?*anyopaque, iCol: c_int) [*c]const u8;
    pub extern "c" fn sqlite3_column_int64(stmt: ?*anyopaque, iCol: c_int) i64;
    pub extern "c" fn sqlite3_column_double(stmt: ?*anyopaque, iCol: c_int) f64;
    pub extern "c" fn sqlite3_column_type(stmt: ?*anyopaque, iCol: c_int) c_int;
    pub extern "c" fn sqlite3_column_count(stmt: ?*anyopaque) c_int;
    pub extern "c" fn sqlite3_changes(db: ?*anyopaque) c_int;
    pub extern "c" fn sqlite3_last_insert_rowid(db: ?*anyopaque) i64;
    pub extern "c" fn sqlite3_finalize(stmt: ?*anyopaque) c_int;
    pub extern "c" fn sqlite3_reset(stmt: ?*anyopaque) c_int;
    pub extern "c" fn sqlite3_clear_bindings(stmt: ?*anyopaque) c_int;
    pub extern "c" fn sqlite3_bind_null(stmt: ?*anyopaque, idx: c_int) c_int;
    pub extern "c" fn sqlite3_bind_int64(stmt: ?*anyopaque, idx: c_int, val: i64) c_int;
    pub extern "c" fn sqlite3_bind_double(stmt: ?*anyopaque, idx: c_int, val: f64) c_int;
    pub extern "c" fn sqlite3_bind_text(stmt: ?*anyopaque, idx: c_int, val: [*c]const u8, n: c_int, destructor: isize) c_int;
    pub extern "c" fn sqlite3_bind_blob(stmt: ?*anyopaque, idx: c_int, val: ?*const anyopaque, n: c_int, destructor: isize) c_int;
    pub extern "c" fn sqlite3_column_blob(stmt: ?*anyopaque, iCol: c_int) ?*const anyopaque;
    pub extern "c" fn sqlite3_column_bytes(stmt: ?*anyopaque, iCol: c_int) c_int;
    pub extern "c" fn sqlite3_column_name(stmt: ?*anyopaque, iCol: c_int) [*c]const u8;
}

pub struct SqliteRows {
    stmt_ptr: *!?,

    pub fn next(*_): bool {
        let has_row: bool = false;
        zig {
            if (self.stmt_ptr) |s| {
                const rc = sqlite3_step(s);
                has_row = (rc == 100); // SQLITE_ROW
            }
        }
        return has_row;
    }

    pub fn columnText(col: i32): []const u8 {
        let text_slice: []const u8 = "";
        zig {
            if (self.stmt_ptr) |s| {
                const c_text = sqlite3_column_text(s, col);
                if (c_text != null) {
                    text_slice = std.mem.span(c_text);
                }
            }
        }
        return text_slice;
    }

    pub fn columnInt(col: i32): i64 {
        let val: i64 = 0;
        zig {
            if (self.stmt_ptr) |s| {
                val = sqlite3_column_int64(s, col);
            }
        }
        return val;
    }

    pub fn columnDouble(col: i32): f64 {
        let val: f64 = 0.0;
        zig {
            if (self.stmt_ptr) |s| {
                val = sqlite3_column_double(s, col);
            }
        }
        return val;
    }

    pub fn columnType(col: i32): i32 {
        let val: i32 = 5; // SQLITE_NULL
        zig {
            if (self.stmt_ptr) |s| {
                val = @intCast(sqlite3_column_type(s, col));
            }
        }
        return val;
    }

    pub fn columnCount(): i32 {
        let val: i32 = 0;
        zig {
            if (self.stmt_ptr) |s| {
                val = @intCast(sqlite3_column_count(s));
            }
        }
        return val;
    }

    pub fn changes(): i32 {
        let val: i32 = 0;
        zig {
            if (self.db_ptr) |d| {
                val = @intCast(sqlite3_changes(d));
            }
        }
        return val;
    }

    pub fn lastInsertRowId(): i64 {
        let val: i64 = 0;
        zig {
            if (self.db_ptr) |d| {
                val = sqlite3_last_insert_rowid(d);
            }
        }
        return val;
    }

    pub fn close(*_) {
        zig {
            if (self.stmt_ptr) |s| {
                _ = sqlite3_finalize(s);
                self.stmt_ptr = null;
            }
        }
    }

    ~init() {
        _.close();
    }

    pub fn columnName(col: i32): []const u8 {
        let s: []const u8 = "";
        zig {
            if (self.stmt_ptr) |st| {
                const c_name = sqlite3_column_name(st, col);
                if (c_name != null) s = std.mem.span(c_name);
            }
        }
        return s;
    }

    pub fn columnBlob(col: i32): []const u8 {
        let s: []const u8 = "";
        zig {
            if (self.stmt_ptr) |st| {
                const ptr_opt = sqlite3_column_blob(st, col);
                const len_i = sqlite3_column_bytes(st, col);
                if (ptr_opt != null and len_i > 0) {
                    const byte_ptr: [*]const u8 = @ptrCast(ptr_opt.?);
                    s = byte_ptr[0..@intCast(len_i)];
                }
            }
        }
        return s;
    }

    pub fn bindNull(*_, idx: i32): ! {
        zig {
            if (self.stmt_ptr) |st| {
                if (sqlite3_bind_null(st, idx) != 0) return error.SQLiteBindFailed;
            } else return error.StatementClosed;
        }
    }

    pub fn bindInt(*_, idx: i32, val: i64): ! {
        zig {
            if (self.stmt_ptr) |st| {
                if (sqlite3_bind_int64(st, idx, val) != 0) return error.SQLiteBindFailed;
            } else return error.StatementClosed;
        }
    }

    pub fn bindDouble(*_, idx: i32, val: f64): ! {
        zig {
            if (self.stmt_ptr) |st| {
                if (sqlite3_bind_double(st, idx, val) != 0) return error.SQLiteBindFailed;
            } else return error.StatementClosed;
        }
    }

    pub fn bindText(*_, idx: i32, val: []const u8): ! {
        zig {
            if (self.stmt_ptr) |st| {
                const empty_c: [1]u8 = .{0};
                const p = if (val.len > 0) val.ptr else &empty_c;
                if (sqlite3_bind_text(st, idx, p, @intCast(val.len), -1) != 0) return error.SQLiteBindFailed;
            } else return error.StatementClosed;
        }
    }

    pub fn bindBlob(*_, idx: i32, val: []const u8): ! {
        zig {
            if (self.stmt_ptr) |st| {
                const p: ?*const anyopaque = if (val.len > 0) @ptrCast(val.ptr) else null;
                if (sqlite3_bind_blob(st, idx, p, @intCast(val.len), -1) != 0) return error.SQLiteBindFailed;
            } else return error.StatementClosed;
        }
    }

    pub fn reset(*_): ! {
        zig {
            if (self.stmt_ptr) |st| {
                if (sqlite3_reset(st) != 0) return error.SQLiteResetFailed;
            } else return error.StatementClosed;
        }
    }

    pub fn clearBindings(*_): ! {
        zig {
            if (self.stmt_ptr) |st| {
                if (sqlite3_clear_bindings(st) != 0) return error.SQLiteBindFailed;
            } else return error.StatementClosed;
        }
    }
}

pub struct SqliteDatabase {
    db_ptr: *!? = null,

    pub static fn open(path: string): SqliteDatabase! {
        let db = SqliteDatabase {
            db_ptr: null,
        };
        zig {
            var raw_db: ?*anyopaque = null;
            const allocator = std.heap.page_allocator;
            const path_z = allocator.alloc(u8, path.len + 1) catch return error.OutOfMemory;
            defer allocator.free(path_z);
            @memcpy(path_z[0..path.len], path);
            path_z[path.len] = 0;

            const rc = sqlite3_open(path_z.ptr, &raw_db);
            if (rc != 0) {
                if (raw_db != null) {
                    _ = sqlite3_close(raw_db);
                }
                return error.SQLiteOpenFailed;
            }
            db.db_ptr = raw_db;
        }
        return db;
    }

    pub fn exec(sql: string): ! {
        zig {
            if (self.db_ptr) |d| {
                const allocator = std.heap.page_allocator;
                const sql_z = allocator.alloc(u8, sql.len + 1) catch return error.OutOfMemory;
                defer allocator.free(sql_z);
                @memcpy(sql_z[0..sql.len], sql);
                sql_z[sql.len] = 0;

                const rc = sqlite3_exec(d, sql_z.ptr, null, null, null);
                if (rc != 0) {
                    return error.SQLiteExecFailed;
                }
            } else {
                return error.DatabaseClosed;
            }
        }
    }

    pub fn query(sql: string): SqliteRows! {
        let rows = SqliteRows {
            stmt_ptr: null,
        };
        zig {
            if (self.db_ptr) |d| {
                var raw_stmt: ?*anyopaque = null;
                const rc = sqlite3_prepare_v2(d, sql.ptr, @intCast(sql.len), &raw_stmt, null);
                if (rc != 0) {
                    return error.SQLitePrepareFailed;
                }
                rows.stmt_ptr = raw_stmt;
            } else {
                return error.DatabaseClosed;
            }
        }
        return rows;
    }

    pub fn changes(): i32 {
        let val: i32 = 0;
        zig {
            if (self.db_ptr) |d| {
                val = @intCast(sqlite3_changes(d));
            }
        }
        return val;
    }

    pub fn lastInsertRowId(): i64 {
        let val: i64 = 0;
        zig {
            if (self.db_ptr) |d| {
                val = sqlite3_last_insert_rowid(d);
            }
        }
        return val;
    }

    pub fn close(*_) {
        zig {
            if (self.db_ptr) |d| {
                _ = sqlite3_close(d);
                self.db_ptr = null;
            }
        }
    }

    ~init() {
        _.close();
    }

    pub fn errMsg(): []const u8 {
        let msg: []const u8 = "";
        zig {
            if (self.db_ptr) |d| {
                const c_err = sqlite3_errmsg(d);
                if (c_err != null) msg = std.mem.span(c_err);
            }
        }
        return msg;
    }

    pub fn begin(): ! {
        _.exec("BEGIN")?;
    }

    pub fn commit(): ! {
        _.exec("COMMIT")?;
    }

    pub fn rollback(): ! {
        _.exec("ROLLBACK")?;
    }
}
