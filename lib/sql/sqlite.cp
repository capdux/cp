// Source: Unified pure-CP SQLite re-export & compatibility adapter
// Pure-CP 4-Gate 0/0/0/0 — delegates directly to `lib/sqlite.cp` (zero `zig {}` blocks).

export * from "../sqlite.cp";

import {
    Database,
    Statement,
    SQLITE_OPEN_READWRITE,
    SQLITE_OPEN_CREATE,
} from "../sqlite.cp";

/// Backwards-compatible alias for `Statement` (formerly `SqliteRows`).
pub type SqliteRows = Statement;

/// Backwards-compatible wrapper for `SqliteDatabase` preserving `SqliteDatabase.open(path)`.
pub struct SqliteDatabase {
    pub inner: Database,

    pub static fn open(path: string): SqliteDatabase! {
        const db = Database.open(path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)?;
        return SqliteDatabase { inner: db };
    }

    pub static fn openMemory(): SqliteDatabase! {
        const db = Database.openMemory()?;
        return SqliteDatabase { inner: db };
    }

    pub fn exec(sql: string): void! {
        _.inner.exec(sql)?;
    }

    pub fn query(*_, sql: string): SqliteRows! {
        return _.inner.query(sql);
    }

    pub fn prepare(sql: string): SqliteRows! {
        return _.inner.prepare(sql);
    }

    pub fn changes(): i32 {
        return _.inner.changes();
    }

    pub fn totalChanges(): i32 {
        return _.inner.totalChanges();
    }

    pub fn lastInsertRowId(): i64 {
        return _.inner.lastInsertRowid();
    }

    pub fn errMsg(): []const u8 {
        return _.inner.errmsg();
    }

    pub fn begin(): void! {
        _.inner.begin()?;
    }

    pub fn commit(): void! {
        _.inner.commit()?;
    }

    pub fn rollback(): void! {
        _.inner.rollback()?;
    }

    pub fn close(*_) {
        _.inner.close();
    }

    ~init() {
        _.close();
    }
}
