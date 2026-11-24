// Source: Custom (cap:sqlite high-performance embedded SQLite3 engine)
// Pure-CP 4-Gate 0/0/0/0 binding over official Public Domain SQLite3 amalgamation.
// Features: LRU prepared-statement cache, zero-copy SQLITE_STATIC bindings,
// named parameter bindings (:name/$name/@name), WAL & Savepoint transactions,
// in-memory serialize/deserialize snapshots, online hot backup, idempotent RAII.

import std from "std";

pub @c {
    fn sqlite3_libversion(): [*:0]const u8;
    fn sqlite3_libversion_number(): i32;

    fn sqlite3_open(filename: [*]const u8!, ppDb: *(*!?)!): i32;
    fn sqlite3_open_v2(filename: [*]const u8!, ppDb: *(*!?)!, flags: i32, zVfs: [*:0]const u8?): i32;
    fn sqlite3_close(db: *!?): i32;
    fn sqlite3_close_v2(db: *!?): i32;

    fn sqlite3_exec(db: *!?, sql: [*]const u8!, callback: *!?, arg: *!?, errmsg: *!?): i32;
    fn sqlite3_errmsg(db: *!?): [*:0]const u8?;
    fn sqlite3_errcode(db: *!?): i32;
    fn sqlite3_extended_errcode(db: *!?): i32;
    fn sqlite3_busy_timeout(db: *!?, ms: i32): i32;
    fn sqlite3_changes(db: *!?): i32;
    fn sqlite3_total_changes(db: *!?): i32;
    fn sqlite3_last_insert_rowid(db: *!?): i64;

    fn sqlite3_prepare_v2(db: *!?, zSql: [*]const u8!, nByte: i32, ppStmt: *(*!?)!, pzTail: *!?): i32;
    fn sqlite3_step(stmt: *!?): i32;
    fn sqlite3_reset(stmt: *!?): i32;
    fn sqlite3_clear_bindings(stmt: *!?): i32;
    fn sqlite3_finalize(stmt: *!?): i32;

    fn sqlite3_bind_parameter_count(stmt: *!?): i32;
    fn sqlite3_bind_parameter_index(stmt: *!?, zName: [*]const u8!): i32;
    fn sqlite3_bind_null(stmt: *!?, idx: i32): i32;
    fn sqlite3_bind_int64(stmt: *!?, idx: i32, val: i64): i32;
    fn sqlite3_bind_double(stmt: *!?, idx: i32, val: f64): i32;
    fn sqlite3_bind_text(stmt: *!?, idx: i32, val: [*]const u8!, n: i32, destructor: isize): i32;
    fn sqlite3_bind_blob(stmt: *!?, idx: i32, val: *const !?, n: i32, destructor: isize): i32;

    fn sqlite3_column_count(stmt: *!?): i32;
    fn sqlite3_column_name(stmt: *!?, iCol: i32): [*:0]const u8?;
    fn sqlite3_column_type(stmt: *!?, iCol: i32): i32;
    fn sqlite3_column_int64(stmt: *!?, iCol: i32): i64;
    fn sqlite3_column_double(stmt: *!?, iCol: i32): f64;
    fn sqlite3_column_text(stmt: *!?, iCol: i32): [*]const u8?;
    fn sqlite3_column_blob(stmt: *!?, iCol: i32): *const !?;
    fn sqlite3_column_bytes(stmt: *!?, iCol: i32): i32;

    fn sqlite3_serialize(db: *!?, zSchema: [*]const u8!, piSize: *i64, mFlags: u32): [*]u8?;
    fn sqlite3_deserialize(db: *!?, zSchema: [*]const u8!, pData: [*]u8?, szDb: i64, szBuf: i64, mFlags: u32): i32;
    fn sqlite3_malloc64(n: u64): *!?;
    fn sqlite3_free(ptr: *!?);

    fn sqlite3_backup_init(pDest: *!?, zDestName: [*]const u8!, pSource: *!?, zSourceName: [*]const u8!): *!?;
    fn sqlite3_backup_step(p: *!?, nPage: i32): i32;
    fn sqlite3_backup_finish(p: *!?): i32;
    fn sqlite3_backup_remaining(p: *!?): i32;
    fn sqlite3_backup_pagecount(p: *!?): i32;
}

// ============================================================================
// SQLite3 Constants
// ============================================================================

pub const SQLITE_OK: i32 = 0;
pub const SQLITE_ERROR: i32 = 1;
pub const SQLITE_BUSY: i32 = 5;
pub const SQLITE_LOCKED: i32 = 6;
pub const SQLITE_NOMEM: i32 = 7;
pub const SQLITE_READONLY: i32 = 8;
pub const SQLITE_CONSTRAINT: i32 = 19;
pub const SQLITE_MISMATCH: i32 = 20;
pub const SQLITE_MISUSE: i32 = 21;
pub const SQLITE_ROW: i32 = 100;
pub const SQLITE_DONE: i32 = 101;

pub const SQLITE_OPEN_READONLY: i32 = 0x00000001;
pub const SQLITE_OPEN_READWRITE: i32 = 0x00000002;
pub const SQLITE_OPEN_CREATE: i32 = 0x00000004;
pub const SQLITE_OPEN_URI: i32 = 0x00000040;
pub const SQLITE_OPEN_MEMORY: i32 = 0x00000080;
pub const SQLITE_OPEN_NOMUTEX: i32 = 0x00008000;
pub const SQLITE_OPEN_FULLMUTEX: i32 = 0x00010000;
pub const SQLITE_OPEN_SHAREDCACHE: i32 = 0x00020000;
pub const SQLITE_OPEN_PRIVATECACHE: i32 = 0x00040000;
pub const SQLITE_OPEN_NOFOLLOW: i32 = 0x01000000;

pub const SQLITE_INTEGER: i32 = 1;
pub const SQLITE_FLOAT: i32 = 2;
pub const SQLITE_TEXT: i32 = 3;
pub const SQLITE_BLOB: i32 = 4;
pub const SQLITE_NULL: i32 = 5;

pub const SQLITE_STATIC: isize = 0;
pub const SQLITE_TRANSIENT: isize = -1;

pub const SQLITE_SERIALIZE_NOCOPY: u32 = 1;
pub const SQLITE_DESERIALIZE_FREEONCLOSE: u32 = 1;
pub const SQLITE_DESERIALIZE_RESIZEABLE: u32 = 2;
pub const SQLITE_DESERIALIZE_READONLY: u32 = 4;

pub const SQLITE_STMT_CACHE_CAP: usize = 32;

pub enum ColumnType: i32 {
    Integer = 1,
    Float = 2,
    Text = 3,
    Blob = 4,
    Null = 5,
}

pub err SqliteError {
    OutOfMemory,
    OpenFailed,
    CloseFailed,
    DatabaseClosed,
    StatementClosed,
    ExecFailed,
    PrepareFailed,
    BindFailed,
    ParameterNotFound,
    StepFailed,
    ResetFailed,
    SerializeFailed,
    DeserializeFailed,
    BackupFailed,
    BusyTimeoutFailed,
    // Legacy aliases for backwards compatibility
    SQLiteOpenFailed,
    SQLiteExecFailed,
    SQLitePrepareFailed,
    SQLiteBindFailed,
    SQLiteResetFailed,
}

// ============================================================================
// Internal Helpers (zero-allocation stack fast-path + FNV-1a hash)
// ============================================================================

const EMPTY_CSTR: [1]u8 = [0];
const MAIN_SCHEMA: [5]u8 = ['m', 'a', 'i', 'n', 0];
const MEMORY_PATH: [9]u8 = [':', 'm', 'e', 'm', 'o', 'r', 'y', ':', 0];

fn hashSql(sql: []const u8): u64 {
    let h: u64 = 0x517cc1b727220a95;
    let i: usize = 0;
    while (i < sql.len) : (i += 1) {
        h ^= @as(u64, sql[i]);
        h = h *% 0x100000001b3;
    }
    return h;
}

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Fixed-capacity LRU Prepared Statement Cache (Heap-owned by Database)
// ============================================================================

struct StmtCacheSlot {
    sql_hash: u64 = 0,
    sql_buf: []u8 = "",
    stmt: *!? = null,
    last_used: u64 = 0,
    in_use: bool = false,
}

pub struct StmtCache {
    slots: [32]StmtCacheSlot = undefined,
    tick: u64 = 0,
    hits: u64 = 0,
    misses: u64 = 0,

    pub static fn create(): (*StmtCache)! {
        const allocator = std.heap.page_allocator;
        let ptr = allocator.create(StmtCache) catch return SqliteError.OutOfMemory;
        ptr.tick = 0;
        ptr.hits = 0;
        ptr.misses = 0;
        let i: usize = 0;
        while (i < SQLITE_STMT_CACHE_CAP) : (i += 1) {
            ptr.slots[i] = StmtCacheSlot {
                sql_hash: 0,
                sql_buf: "",
                stmt: null,
                last_used: 0,
                in_use: false,
            };
        }
        return ptr;
    }

    pub fn releaseSlot(*_, slot_idx: usize) {
        if (slot_idx >= SQLITE_STMT_CACHE_CAP) return;
        if (s := _.slots[slot_idx].stmt) {
            * = sqlite3_reset(s);
            * = sqlite3_clear_bindings(s);
        }
        _.slots[slot_idx].in_use = false;
    }

    pub fn clearAll(*_) {
        const allocator = std.heap.page_allocator;
        let i: usize = 0;
        while (i < SQLITE_STMT_CACHE_CAP) : (i += 1) {
            if (s := _.slots[i].stmt) {
                * = sqlite3_finalize(s);
                _.slots[i].stmt = null;
            }
            if (_.slots[i].sql_buf.len > 0) {
                allocator.free(_.slots[i].sql_buf);
                _.slots[i].sql_buf = "";
            }
            _.slots[i].sql_hash = 0;
            _.slots[i].last_used = 0;
            _.slots[i].in_use = false;
        }
    }

    pub fn destroy(*_) {
        _.clearAll();
        const allocator = std.heap.page_allocator;
        allocator.destroy(_);
    }
}

// ============================================================================
// Prepared Statement (RAII, Zero-Copy Bindings, Named Parameters)
// ============================================================================

pub struct Statement {
    pub stmt_ptr: *!? = null,
    pub db_ptr: *!? = null,
    pub owner_cache: (*StmtCache)? = null,
    pub cache_slot: i32 = -1,

    pub fn isOpen(): bool {
        return _.stmt_ptr != null;
    }

    // ---- Parameter Inspection ----

    pub fn paramCount(): i32 {
        if (s := _.stmt_ptr) {
            return sqlite3_bind_parameter_count(s);
        }
        return 0;
    }

    pub fn paramIndex(name: []const u8): i32! {
        if (s := _.stmt_ptr) {
            if (name.len == 0) return SqliteError.ParameterNotFound;
            let stack_buf: [256]u8 = undefined;
            if (name.len < 256) {
                @memcpy(stack_buf[0..name.len], name);
                stack_buf[name.len] = 0;
                const idx = sqlite3_bind_parameter_index(s, &stack_buf);
                if (idx > 0) return idx;
                // Auto-try ':' / '$' / '@' prefix if caller passed bare name
                if (name[0] != ':' && name[0] != '$' && name[0] != '@' && name.len < 255) {
                    @memcpy(stack_buf[1..name.len + 1], name);
                    stack_buf[name.len + 1] = 0;
                    stack_buf[0] = ':';
                    const idx_colon = sqlite3_bind_parameter_index(s, &stack_buf);
                    if (idx_colon > 0) return idx_colon;
                    stack_buf[0] = '$';
                    const idx_dollar = sqlite3_bind_parameter_index(s, &stack_buf);
                    if (idx_dollar > 0) return idx_dollar;
                    stack_buf[0] = '@';
                    const idx_at = sqlite3_bind_parameter_index(s, &stack_buf);
                    if (idx_at > 0) return idx_at;
                }
                return SqliteError.ParameterNotFound;
            }
            const allocator = std.heap.page_allocator;
            let buf = allocator.alloc(u8, name.len + 1) catch return SqliteError.OutOfMemory;
            defer allocator.free(buf);
            @memcpy(buf[0..name.len], name);
            buf[name.len] = 0;
            const idx = sqlite3_bind_parameter_index(s, buf.ptr);
            if (idx <= 0) return SqliteError.ParameterNotFound;
            return idx;
        }
        return SqliteError.StatementClosed;
    }

    // ---- Indexed Bindings (1-based; SQLITE_STATIC zero-copy by default) ----

    pub fn bindNull(*_, idx: i32): void! {
        if (s := _.stmt_ptr) {
            if (sqlite3_bind_null(s, idx) != SQLITE_OK) return SqliteError.BindFailed;
            return;
        }
        return SqliteError.StatementClosed;
    }

    pub fn bindInt(*_, idx: i32, val: i64): void! {
        if (s := _.stmt_ptr) {
            if (sqlite3_bind_int64(s, idx, val) != SQLITE_OK) return SqliteError.BindFailed;
            return;
        }
        return SqliteError.StatementClosed;
    }

    pub fn bindDouble(*_, idx: i32, val: f64): void! {
        if (s := _.stmt_ptr) {
            if (sqlite3_bind_double(s, idx, val) != SQLITE_OK) return SqliteError.BindFailed;
            return;
        }
        return SqliteError.StatementClosed;
    }

    /// Zero-copy text bind (`SQLITE_STATIC`). Caller must keep `val` alive until step/reset.
    pub fn bindText(*_, idx: i32, val: []const u8): void! {
        if (s := _.stmt_ptr) {
            const p: [*]const u8 = if (val.len > 0) val.ptr else &EMPTY_CSTR;
            if (sqlite3_bind_text(s, idx, p, @intCast(val.len), SQLITE_STATIC) != SQLITE_OK) {
                return SqliteError.BindFailed;
            }
            return;
        }
        return SqliteError.StatementClosed;
    }

    /// Copying text bind (`SQLITE_TRANSIENT`). Safe when `val` is temporary.
    pub fn bindTextCopy(*_, idx: i32, val: []const u8): void! {
        if (s := _.stmt_ptr) {
            const p: [*]const u8 = if (val.len > 0) val.ptr else &EMPTY_CSTR;
            if (sqlite3_bind_text(s, idx, p, @intCast(val.len), SQLITE_TRANSIENT) != SQLITE_OK) {
                return SqliteError.BindFailed;
            }
            return;
        }
        return SqliteError.StatementClosed;
    }

    /// Zero-copy blob bind (`SQLITE_STATIC`). Caller must keep `val` alive until step/reset.
    pub fn bindBlob(*_, idx: i32, val: []const u8): void! {
        if (s := _.stmt_ptr) {
            let p: *const !? = null;
            if (val.len > 0) {
                unsafe { p = @ptrCast(val.ptr); }
            }
            if (sqlite3_bind_blob(s, idx, p, @intCast(val.len), SQLITE_STATIC) != SQLITE_OK) {
                return SqliteError.BindFailed;
            }
            return;
        }
        return SqliteError.StatementClosed;
    }

    /// Copying blob bind (`SQLITE_TRANSIENT`). Safe when `val` is temporary.
    pub fn bindBlobCopy(*_, idx: i32, val: []const u8): void! {
        if (s := _.stmt_ptr) {
            let p: *const !? = null;
            if (val.len > 0) {
                unsafe { p = @ptrCast(val.ptr); }
            }
            if (sqlite3_bind_blob(s, idx, p, @intCast(val.len), SQLITE_TRANSIENT) != SQLITE_OK) {
                return SqliteError.BindFailed;
            }
            return;
        }
        return SqliteError.StatementClosed;
    }

    // ---- Named Parameter Bindings (:name / $name / @name / bare name) ----

    pub fn bindNullByName(*_, name: []const u8): void! {
        const idx = _.paramIndex(name)?;
        _.bindNull(idx)?;
    }

    pub fn bindIntByName(*_, name: []const u8, val: i64): void! {
        const idx = _.paramIndex(name)?;
        _.bindInt(idx, val)?;
    }

    pub fn bindDoubleByName(*_, name: []const u8, val: f64): void! {
        const idx = _.paramIndex(name)?;
        _.bindDouble(idx, val)?;
    }

    pub fn bindTextByName(*_, name: []const u8, val: []const u8): void! {
        const idx = _.paramIndex(name)?;
        _.bindText(idx, val)?;
    }

    pub fn bindTextCopyByName(*_, name: []const u8, val: []const u8): void! {
        const idx = _.paramIndex(name)?;
        _.bindTextCopy(idx, val)?;
    }

    pub fn bindBlobByName(*_, name: []const u8, val: []const u8): void! {
        const idx = _.paramIndex(name)?;
        _.bindBlob(idx, val)?;
    }

    pub fn bindBlobCopyByName(*_, name: []const u8, val: []const u8): void! {
        const idx = _.paramIndex(name)?;
        _.bindBlobCopy(idx, val)?;
    }

    // ---- Execution & Cursor ----

    /// Advance statement by one row. Returns `true` on SQLITE_ROW, `false` on SQLITE_DONE.
    pub fn step(*_): bool! {
        if (s := _.stmt_ptr) {
            const rc = sqlite3_step(s);
            if (rc == SQLITE_ROW) return true;
            if (rc == SQLITE_DONE) return false;
            return SqliteError.StepFailed;
        }
        return SqliteError.StatementClosed;
    }

    /// Convenience non-throwing cursor step (returns `true` if a row is available).
    pub fn next(*_): bool {
        if (s := _.stmt_ptr) {
            return sqlite3_step(s) == SQLITE_ROW;
        }
        return false;
    }

    /// Execute a DML/DDL statement to completion and reset it for reuse.
    pub fn run(*_): void! {
        if (s := _.stmt_ptr) {
            while (true) {
                const rc = sqlite3_step(s);
                if (rc == SQLITE_DONE) {
                    * = sqlite3_reset(s);
                    return;
                }
                if (rc != SQLITE_ROW) {
                    * = sqlite3_reset(s);
                    return SqliteError.StepFailed;
                }
            }
        }
        return SqliteError.StatementClosed;
    }

    /// Step to fetch the first row (alias for `step()`).
    pub fn get(*_): bool! {
        return _.step();
    }

    pub fn reset(*_): void! {
        if (s := _.stmt_ptr) {
            if (sqlite3_reset(s) != SQLITE_OK) return SqliteError.ResetFailed;
            return;
        }
        return SqliteError.StatementClosed;
    }

    pub fn clearBindings(*_): void! {
        if (s := _.stmt_ptr) {
            if (sqlite3_clear_bindings(s) != SQLITE_OK) return SqliteError.BindFailed;
            return;
        }
        return SqliteError.StatementClosed;
    }

    // ---- Column Accessors (0-based, zero-copy views valid until next step/reset/close) ----

    pub fn columnCount(): i32 {
        if (s := _.stmt_ptr) {
            return sqlite3_column_count(s);
        }
        return 0;
    }

    pub fn columnName(col: i32): []const u8 {
        if (s := _.stmt_ptr) {
            if (c_name := sqlite3_column_name(s, col)) {
                return std.mem.span(c_name);
            }
        }
        return "";
    }

    pub fn columnType(col: i32): i32 {
        if (s := _.stmt_ptr) {
            return sqlite3_column_type(s, col);
        }
        return SQLITE_NULL;
    }

    pub fn columnIsNull(col: i32): bool {
        return _.columnType(col) == SQLITE_NULL;
    }

    pub fn columnInt(col: i32): i64 {
        if (s := _.stmt_ptr) {
            return sqlite3_column_int64(s, col);
        }
        return 0;
    }

    pub fn columnDouble(col: i32): f64 {
        if (s := _.stmt_ptr) {
            return sqlite3_column_double(s, col);
        }
        return 0.0;
    }

    pub fn columnText(col: i32): []const u8 {
        if (s := _.stmt_ptr) {
            if (txt := sqlite3_column_text(s, col)) {
                const n = sqlite3_column_bytes(s, col);
                if (n > 0) {
                    unsafe {
                        return txt[0..@intCast(n)];
                    }
                }
            }
        }
        return "";
    }

    pub fn columnBlob(col: i32): []const u8 {
        if (s := _.stmt_ptr) {
            const n = sqlite3_column_bytes(s, col);
            if (n > 0) {
                if (raw := sqlite3_column_blob(s, col)) {
                    unsafe {
                        const bp: [*]const u8 = @ptrCast(raw);
                        return bp[0..@intCast(n)];
                    }
                }
            }
        }
        return "";
    }

    pub fn changes(): i32 {
        if (d := _.db_ptr) {
            return sqlite3_changes(d);
        }
        return 0;
    }

    pub fn lastInsertRowId(): i64 {
        if (d := _.db_ptr) {
            return sqlite3_last_insert_rowid(d);
        }
        return 0;
    }

    // ---- Idempotent RAII Cleanup ----

    pub fn close(*_) {
        if (_.stmt_ptr == null) return;
        if (_.cache_slot >= 0) {
            if (c := _.owner_cache) {
                c.releaseSlot(@intCast(_.cache_slot));
                _.stmt_ptr = null;
                _.owner_cache = null;
                _.cache_slot = -1;
                return;
            }
        }
        if (s := _.stmt_ptr) {
            * = sqlite3_finalize(s);
            _.stmt_ptr = null;
        }
    }

    ~init() {
        _.close();
    }
}

// ============================================================================
// Database (High-Performance Connection + LRU Statement Cache + Snapshot/Backup)
// ============================================================================

pub struct Database {
    pub ptr: *!? = null,
    pub cache: (*StmtCache)? = null,

    // ---- Version Info ----

    pub static fn version(): []const u8 {
        return std.mem.span(sqlite3_libversion());
    }

    pub static fn versionNumber(): i32 {
        return sqlite3_libversion_number();
    }

    // ---- Constructors ----

    pub static fn open(path: []const u8, flags: i32): Database! {
        let raw_db: *!? = null;
        let stack_path: [512]u8 = undefined;
        let rc: i32 = SQLITE_ERROR;

        if (path.len < 512) {
            if (path.len > 0) {
                @memcpy(stack_path[0..path.len], path);
            }
            stack_path[path.len] = 0;
            rc = sqlite3_open_v2(&stack_path, &raw_db, flags, null);
        } else {
            const allocator = std.heap.page_allocator;
            let path_z = allocator.alloc(u8, path.len + 1) catch return SqliteError.OutOfMemory;
            defer allocator.free(path_z);
            @memcpy(path_z[0..path.len], path);
            path_z[path.len] = 0;
            rc = sqlite3_open_v2(path_z.ptr, &raw_db, flags, null);
        }

        if (rc != SQLITE_OK) {
            if (d := raw_db) {
                * = sqlite3_close_v2(d);
            }
            return SqliteError.OpenFailed;
        }
        return Database {
            ptr: raw_db,
            cache: null,
        };
    }

    pub static fn openDefault(path: []const u8): Database! {
        return Database.open(path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    }

    pub static fn openReadOnly(path: []const u8): Database! {
        return Database.open(path, SQLITE_OPEN_READONLY);
    }

    pub static fn openMemory(): Database! {
        let raw_db: *!? = null;
        const flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY;
        const rc = sqlite3_open_v2(&MEMORY_PATH, &raw_db, flags, null);
        if (rc != SQLITE_OK) {
            if (d := raw_db) {
                * = sqlite3_close_v2(d);
            }
            return SqliteError.OpenFailed;
        }
        return Database {
            ptr: raw_db,
            cache: null,
        };
    }

    /// Legacy initializer for backwards compatibility (`db.init(path)`).
    pub fn init(*_, path: string): void! {
        const opened = Database.openDefault(path)?;
        _.close();
        _.ptr = opened.ptr;
        _.cache = null;
    }

    pub fn isOpen(): bool {
        return _.ptr != null;
    }

    // ---- Direct SQL Execution ----

    pub fn exec(sql: []const u8): void! {
        if (d := _.ptr) {
            let stack_sql: [1024]u8 = undefined;
            if (sql.len < 1024) {
                if (sql.len > 0) {
                    @memcpy(stack_sql[0..sql.len], sql);
                }
                stack_sql[sql.len] = 0;
                if (sqlite3_exec(d, &stack_sql, null, null, null) != SQLITE_OK) {
                    return SqliteError.ExecFailed;
                }
                return;
            }
            const allocator = std.heap.page_allocator;
            let sql_z = allocator.alloc(u8, sql.len + 1) catch return SqliteError.OutOfMemory;
            defer allocator.free(sql_z);
            @memcpy(sql_z[0..sql.len], sql);
            sql_z[sql.len] = 0;
            if (sqlite3_exec(d, sql_z.ptr, null, null, null) != SQLITE_OK) {
                return SqliteError.ExecFailed;
            }
            return;
        }
        return SqliteError.DatabaseClosed;
    }

    // ---- Prepare (uncached) & Query (LRU cached) ----

    pub fn prepare(sql: []const u8): Statement! {
        if (d := _.ptr) {
            let raw_stmt: *!? = null;
            const p: [*]const u8 = if (sql.len > 0) sql.ptr else &EMPTY_CSTR;
            if (sqlite3_prepare_v2(d, p, @intCast(sql.len), &raw_stmt, null) != SQLITE_OK) {
                return SqliteError.PrepareFailed;
            }
            return Statement {
                stmt_ptr: raw_stmt,
                db_ptr: d,
                owner_cache: null,
                cache_slot: -1,
            };
        }
        return SqliteError.DatabaseClosed;
    }

    /// Compile or reuse a prepared statement via the built-in 32-slot LRU cache.
    pub fn query(*_, sql: []const u8): Statement! {
        if (d := _.ptr) {
            if (_.cache == null) {
                _.cache = StmtCache.create()?;
            }
            if (c := _.cache) {
                const h = hashSql(sql);
                c.tick += 1;
                const now = c.tick;

                // 1. Fast-path cache lookup
                let i: usize = 0;
                while (i < SQLITE_STMT_CACHE_CAP) : (i += 1) {
                    if (!c.slots[i].in_use && c.slots[i].stmt != null && c.slots[i].sql_hash == h) {
                        if (sliceEq(c.slots[i].sql_buf, sql)) {
                            c.slots[i].in_use = true;
                            c.slots[i].last_used = now;
                            c.hits += 1;
                            if (st := c.slots[i].stmt) {
                                * = sqlite3_reset(st);
                                * = sqlite3_clear_bindings(st);
                            }
                            return Statement {
                                stmt_ptr: c.slots[i].stmt,
                                db_ptr: d,
                                owner_cache: c,
                                cache_slot: @intCast(i),
                            };
                        }
                    }
                }

                // 2. Cache miss -> prepare new statement
                c.misses += 1;
                let raw_stmt: *!? = null;
                const p: [*]const u8 = if (sql.len > 0) sql.ptr else &EMPTY_CSTR;
                if (sqlite3_prepare_v2(d, p, @intCast(sql.len), &raw_stmt, null) != SQLITE_OK) {
                    return SqliteError.PrepareFailed;
                }

                // 3. Find empty slot or evict least-recently-used idle slot
                let victim: i32 = -1;
                let oldest_tick: u64 = std.math.maxInt(u64);
                let j: usize = 0;
                while (j < SQLITE_STMT_CACHE_CAP) : (j += 1) {
                    if (c.slots[j].stmt == null) {
                        victim = @intCast(j);
                        break;
                    }
                    if (!c.slots[j].in_use && c.slots[j].last_used < oldest_tick) {
                        oldest_tick = c.slots[j].last_used;
                        victim = @intCast(j);
                    }
                }

                if (victim >= 0) {
                    const idx: usize = @intCast(victim);
                    const allocator = std.heap.page_allocator;
                    let sql_copy = allocator.alloc(u8, sql.len) catch {
                        return Statement {
                            stmt_ptr: raw_stmt,
                            db_ptr: d,
                            owner_cache: null,
                            cache_slot: -1,
                        };
                    };
                    if (sql.len > 0) {
                        @memcpy(sql_copy, sql);
                    }
                    if (old_stmt := c.slots[idx].stmt) {
                        * = sqlite3_finalize(old_stmt);
                    }
                    if (c.slots[idx].sql_buf.len > 0) {
                        allocator.free(c.slots[idx].sql_buf);
                    }
                    c.slots[idx] = StmtCacheSlot {
                        sql_hash: h,
                        sql_buf: sql_copy,
                        stmt: raw_stmt,
                        last_used: now,
                        in_use: true,
                    };
                    return Statement {
                        stmt_ptr: raw_stmt,
                        db_ptr: d,
                        owner_cache: c,
                        cache_slot: victim,
                    };
                }

                // All 32 slots concurrently checked out -> return uncached statement
                return Statement {
                    stmt_ptr: raw_stmt,
                    db_ptr: d,
                    owner_cache: null,
                    cache_slot: -1,
                };
            }
        }
        return SqliteError.DatabaseClosed;
    }

    pub fn cacheHits(): u64 {
        if (c := _.cache) return c.hits;
        return 0;
    }

    pub fn cacheMisses(): u64 {
        if (c := _.cache) return c.misses;
        return 0;
    }

    pub fn clearStmtCache(*_) {
        if (c := _.cache) {
            c.clearAll();
        }
    }

    /// Legacy single-cell string query helper.
    pub fn query_string(sql: string): string?! {
        let stmt = _.prepare(sql)?;
        defer stmt.close();
        if (stmt.step()?) {
            const text = stmt.columnText(0);
            const allocator = std.heap.page_allocator;
            let result = allocator.alloc(u8, text.len) catch return SqliteError.OutOfMemory;
            if (text.len > 0) {
                @memcpy(result, text);
            }
            return result;
        }
        return "";
    }

    // ---- Transactions, Savepoints & Concurrency ----

    pub fn begin(): void! {
        _.exec("BEGIN")?;
    }

    pub fn beginImmediate(): void! {
        _.exec("BEGIN IMMEDIATE")?;
    }

    pub fn beginExclusive(): void! {
        _.exec("BEGIN EXCLUSIVE")?;
    }

    pub fn commit(): void! {
        _.exec("COMMIT")?;
    }

    pub fn rollback(): void! {
        _.exec("ROLLBACK")?;
    }

    pub fn savepoint(name: []const u8): void! {
        let buf: [256]u8 = undefined;
        const prefix = "SAVEPOINT ";
        if (prefix.len + name.len >= 256) return SqliteError.ExecFailed;
        @memcpy(buf[0..prefix.len], prefix);
        @memcpy(buf[prefix.len..prefix.len + name.len], name);
        _.exec(buf[0..prefix.len + name.len])?;
    }

    pub fn releaseSavepoint(name: []const u8): void! {
        let buf: [256]u8 = undefined;
        const prefix = "RELEASE SAVEPOINT ";
        if (prefix.len + name.len >= 256) return SqliteError.ExecFailed;
        @memcpy(buf[0..prefix.len], prefix);
        @memcpy(buf[prefix.len..prefix.len + name.len], name);
        _.exec(buf[0..prefix.len + name.len])?;
    }

    pub fn rollbackTo(name: []const u8): void! {
        let buf: [256]u8 = undefined;
        const prefix = "ROLLBACK TO SAVEPOINT ";
        if (prefix.len + name.len >= 256) return SqliteError.ExecFailed;
        @memcpy(buf[0..prefix.len], prefix);
        @memcpy(buf[prefix.len..prefix.len + name.len], name);
        _.exec(buf[0..prefix.len + name.len])?;
    }

    pub fn enableWal(): void! {
        _.exec("PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;")?;
    }

    pub fn busyTimeout(ms: i32): void! {
        if (d := _.ptr) {
            if (sqlite3_busy_timeout(d, ms) != SQLITE_OK) {
                return SqliteError.BusyTimeoutFailed;
            }
            return;
        }
        return SqliteError.DatabaseClosed;
    }

    // ---- Status & Diagnostics ----

    pub fn changes(): i32 {
        if (d := _.ptr) return sqlite3_changes(d);
        return 0;
    }

    pub fn totalChanges(): i32 {
        if (d := _.ptr) return sqlite3_total_changes(d);
        return 0;
    }

    pub fn lastInsertRowid(): i64 {
        if (d := _.ptr) return sqlite3_last_insert_rowid(d);
        return 0;
    }

    pub fn lastInsertRowId(): i64 {
        return _.lastInsertRowid();
    }

    pub fn errmsg(): []const u8 {
        if (d := _.ptr) {
            if (c_err := sqlite3_errmsg(d)) {
                return std.mem.span(c_err);
            }
        }
        return "";
    }

    pub fn errMsg(): []const u8 {
        return _.errmsg();
    }

    pub fn errcode(): i32 {
        if (d := _.ptr) return sqlite3_errcode(d);
        return SQLITE_OK;
    }

    pub fn extendedErrcode(): i32 {
        if (d := _.ptr) return sqlite3_extended_errcode(d);
        return SQLITE_OK;
    }

    // ---- In-Memory Snapshot Serialize / Deserialize ----

    /// Export the `"main"` database into a standalone byte snapshot.
    /// Free the returned slice with `Database.freeSerialized(buf)`.
    pub fn serialize(): ([]u8)! {
        if (d := _.ptr) {
            let sz: i64 = 0;
            const raw_opt = sqlite3_serialize(d, &MAIN_SCHEMA, &sz, 0);
            if (raw_ptr := raw_opt) {
                defer {
                    unsafe { sqlite3_free(@ptrCast(raw_ptr)); }
                }
                if (sz <= 0) {
                    return SqliteError.SerializeFailed;
                }
                const n: usize = @intCast(sz);
                const allocator = std.heap.page_allocator;
                let out = allocator.alloc(u8, n) catch return SqliteError.OutOfMemory;
                unsafe {
                    @memcpy(out, raw_ptr[0..n]);
                }
                return out;
            }
            return SqliteError.SerializeFailed;
        }
        return SqliteError.DatabaseClosed;
    }

    pub static fn freeSerialized(buf: []u8) {
        if (buf.len > 0) {
            const allocator = std.heap.page_allocator;
            allocator.free(buf);
        }
    }

    /// Replace `"main"` with an in-memory database loaded from `data` bytes.
    pub fn deserialize(*_, data: []const u8): void! {
        if (d := _.ptr) {
            if (data.len == 0) return SqliteError.DeserializeFailed;
            _.clearStmtCache();
            const raw_opt = sqlite3_malloc64(@intCast(data.len));
            if (raw := raw_opt) {
                let dst_ptr: [*]u8? = null;
                unsafe {
                    const dst: [*]u8 = @ptrCast(raw);
                    @memcpy(dst[0..data.len], data);
                    dst_ptr = dst;
                }
                const flags = SQLITE_DESERIALIZE_FREEONCLOSE | SQLITE_DESERIALIZE_RESIZEABLE;
                const rc = sqlite3_deserialize(
                    d,
                    &MAIN_SCHEMA,
                    dst_ptr,
                    @intCast(data.len),
                    @intCast(data.len),
                    flags,
                );
                if (rc != SQLITE_OK) {
                    sqlite3_free(raw);
                    return SqliteError.DeserializeFailed;
                }
                return;
            }
            return SqliteError.OutOfMemory;
        }
        return SqliteError.DatabaseClosed;
    }

    // ---- Online Hot Backup ----

    /// Copy the entire `"main"` database into `dest_db`'s `"main"` schema atomically.
    pub fn backupTo(dest_db: *Database): void! {
        if (src := _.ptr) {
            if (dst := dest_db.ptr) {
                const bk_opt = sqlite3_backup_init(dst, &MAIN_SCHEMA, src, &MAIN_SCHEMA);
                if (bk := bk_opt) {
                    const step_rc = sqlite3_backup_step(bk, -1);
                    const fin_rc = sqlite3_backup_finish(bk);
                    if (step_rc != SQLITE_DONE || fin_rc != SQLITE_OK) {
                        return SqliteError.BackupFailed;
                    }
                    return;
                }
                return SqliteError.BackupFailed;
            }
            return SqliteError.DatabaseClosed;
        }
        return SqliteError.DatabaseClosed;
    }

    // ---- Idempotent RAII Cleanup ----

    pub fn close(*_) {
        if (c := _.cache) {
            c.destroy();
            _.cache = null;
        }
        if (d := _.ptr) {
            * = sqlite3_close_v2(d);
            _.ptr = null;
        }
    }

    ~init() {
        _.close();
    }
}
