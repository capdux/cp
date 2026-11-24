// Full verification & benchmark suite for `cap:sqlite` (SQLite3 3.54.1)
// Pure-CP 4-Gate 0/0/0/0: covers CRUD, named params, Blob/Float/Null, FTS5,
// JSON1, transactions/savepoints, LRU statement cache, serialize/deserialize,
// online hot backup, and 100,000-row bulk insert + point-lookup benchmark.

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Database,
    Statement,
    SQLITE_INTEGER,
    SQLITE_FLOAT,
    SQLITE_TEXT,
    SQLITE_BLOB,
    SQLITE_NULL,
    SQLITE_OPEN_READWRITE,
    SQLITE_OPEN_CREATE,
    SQLITE_OPEN_MEMORY,
} from "cap:sqlite";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn nowNs(): u64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    * = std.os.linux.clock_gettime(.MONOTONIC, &ts);
    return (@as(u64, @intCast(ts.sec)) * 1000000000) + @as(u64, @intCast(ts.nsec));
}

fn runCase1Crud(): void! {
    const ver = Database.version();
    expect_eq(ver.len > 0, true);
    expect_eq(Database.versionNumber() >= 3035000, true);

    let db = Database.openMemory()?;
    defer db.close();
    db.busyTimeout(5000)?;

    db.exec("CREATE TABLE users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, age INTEGER);")?;
    db.exec("INSERT INTO users (name, age) VALUES ('Alice', 30), ('Bob', 25);")?;
    expect_eq(db.changes(), 2);
    expect_eq(db.totalChanges(), 2);
    expect_eq(db.lastInsertRowid(), 2);

    db.exec("UPDATE users SET age = 31 WHERE name = 'Alice';")?;
    expect_eq(db.changes(), 1);
    expect_eq(db.totalChanges(), 3);

    let stmt = db.prepare("SELECT id, name, age FROM users ORDER BY id ASC;")?;
    defer stmt.close();
    expect_eq(stmt.columnCount(), 3);
    expect_eq(bytesEq(stmt.columnName(0), "id"), true);
    expect_eq(bytesEq(stmt.columnName(1), "name"), true);
    expect_eq(bytesEq(stmt.columnName(2), "age"), true);

    expect_eq(stmt.step()?, true);
    expect_eq(stmt.columnInt(0), 1);
    expect_eq(bytesEq(stmt.columnText(1), "Alice"), true);
    expect_eq(stmt.columnInt(2), 31);

    expect_eq(stmt.step()?, true);
    expect_eq(stmt.columnInt(0), 2);
    expect_eq(bytesEq(stmt.columnText(1), "Bob"), true);
    expect_eq(stmt.columnInt(2), 25);

    expect_eq(stmt.step()?, false);

    db.exec("DELETE FROM users WHERE id = 2;")?;
    expect_eq(db.changes(), 1);
}

fn runCase2NamedAndTypes(): void! {
    let db = Database.openMemory()?;
    defer db.close();
    db.exec("CREATE TABLE items (id INTEGER PRIMARY KEY, label TEXT, score REAL, payload BLOB, note TEXT);")?;

    let ins = db.prepare("INSERT INTO items (id, label, score, payload, note) VALUES (:id, $label, @score, :payload, :note);")?;
    defer ins.close();
    expect_eq(ins.paramCount(), 5);

    const blob_data: [8]u8 = [0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x11, 0x22, 0x33];
    ins.bindIntByName(":id", 42)?;
    ins.bindTextByName("$label", "quantum-chip")?;
    ins.bindDoubleByName("@score", 98.625)?;
    ins.bindBlobByName(":payload", blob_data[0..])?;
    ins.bindNullByName("note")?;
    ins.run()?;

    ins.clearBindings()?;
    ins.bindIntByName("id", 43)?;
    ins.bindTextCopyByName("label", "transient-copy")?;
    ins.bindDoubleByName("score", -12.5)?;
    ins.bindBlobCopyByName("payload", blob_data[0..4])?;
    ins.bindTextCopyByName("note", "ok")?;
    ins.run()?;

    let sel = db.prepare("SELECT id, label, score, payload, note FROM items WHERE id = :id;")?;
    defer sel.close();
    sel.bindIntByName(":id", 42)?;
    expect_eq(sel.get()?, true);
    expect_eq(sel.columnType(0), SQLITE_INTEGER);
    expect_eq(sel.columnType(1), SQLITE_TEXT);
    expect_eq(sel.columnType(2), SQLITE_FLOAT);
    expect_eq(sel.columnType(3), SQLITE_BLOB);
    expect_eq(sel.columnType(4), SQLITE_NULL);
    expect_eq(sel.columnIsNull(4), true);

    expect_eq(sel.columnInt(0), 42);
    expect_eq(bytesEq(sel.columnText(1), "quantum-chip"), true);
    expect_eq(sel.columnDouble(2) > 98.62 && sel.columnDouble(2) < 98.63, true);
    expect_eq(bytesEq(sel.columnBlob(3), blob_data[0..]), true);
}

fn runCase3Fts5JsonMath(): void! {
    let db = Database.openMemory()?;
    defer db.close();

    // FTS5
    db.exec("CREATE VIRTUAL TABLE docs USING fts5(title, body);")?;
    db.exec("INSERT INTO docs (title, body) VALUES ('CP Language', 'Zero-copy SQLite engine with RAII and BEAM actors');")?;
    db.exec("INSERT INTO docs (title, body) VALUES ('Zig Compiler', 'Comptime metaprogramming and cross compilation');")?;
    db.exec("INSERT INTO docs (title, body) VALUES ('SQLite FTS5', 'Full text search engine inside SQLite amalgamation');")?;

    let fts = db.prepare("SELECT title FROM docs WHERE docs MATCH 'SQLite' ORDER BY rank;")?;
    defer fts.close();
    let fts_count: i32 = 0;
    while (fts.step()?) {
        fts_count += 1;
    }
    expect_eq(fts_count, 2);

    // JSON1
    let js = db.prepare("SELECT json_extract('{\"agent\":\"cp\",\"metrics\":{\"ops\":999}}', '$.metrics.ops'), json_array_length('[10,20,30,40]');")?;
    defer js.close();
    expect_eq(js.get()?, true);
    expect_eq(js.columnInt(0), 999);
    expect_eq(js.columnInt(1), 4);

    // Math functions
    let mf = db.prepare("SELECT sqrt(144.0), pow(2.0, 10.0);")?;
    defer mf.close();
    expect_eq(mf.get()?, true);
    expect_eq(mf.columnDouble(0) > 11.99 && mf.columnDouble(0) < 12.01, true);
    expect_eq(mf.columnDouble(1) > 1023.99 && mf.columnDouble(1) < 1024.01, true);
}

fn runCase4TxSavepoint(): void! {
    let db = Database.openMemory()?;
    defer db.close();
    db.exec("CREATE TABLE ledger (id INTEGER PRIMARY KEY, amount INTEGER);")?;

    db.begin()?;
    db.exec("INSERT INTO ledger VALUES (1, 100);")?;
    db.rollback()?;

    let cnt = db.prepare("SELECT count(*) FROM ledger;")?;
    defer cnt.close();
    expect_eq(cnt.get()?, true);
    expect_eq(cnt.columnInt(0), 0);

    db.begin()?;
    db.exec("INSERT INTO ledger VALUES (1, 100);")?;
    db.savepoint("sp1")?;
    db.exec("INSERT INTO ledger VALUES (2, 200);")?;
    db.rollbackTo("sp1")?;
    db.releaseSavepoint("sp1")?;
    db.commit()?;

    cnt.reset()?;
    expect_eq(cnt.get()?, true);
    expect_eq(cnt.columnInt(0), 1);
}

fn runCase5LruCache(): void! {
    let db = Database.openMemory()?;
    defer db.close();
    db.exec("CREATE TABLE kv (k INTEGER PRIMARY KEY, v TEXT);")?;
    db.exec("INSERT INTO kv VALUES (1, 'alpha'), (2, 'beta'), (3, 'gamma');")?;

    const sql = "SELECT v FROM kv WHERE k = ?1;";
    expect_eq(db.cacheHits(), 0);
    expect_eq(db.cacheMisses(), 0);

    let iter: i64 = 1;
    while (iter <= 10) : (iter += 1) {
        let s = db.query(sql)?;
        s.bindInt(1, @rem(iter, 3) + 1)?;
        expect_eq(s.get()?, true);
        expect_eq(s.columnText(0).len > 0, true);
        s.close();
    }

    expect_eq(db.cacheMisses(), 1);
    expect_eq(db.cacheHits(), 9);

    db.clearStmtCache();
    let s2 = db.query(sql)?;
    s2.bindInt(1, 1)?;
    expect_eq(s2.get()?, true);
    expect_eq(bytesEq(s2.columnText(0), "alpha"), true);
    s2.close();
    expect_eq(db.cacheMisses(), 2);
}

fn runCase6SerializeBackup(): void! {
    let src = Database.openMemory()?;
    defer src.close();
    src.exec("CREATE TABLE snap (id INTEGER PRIMARY KEY, val TEXT);")?;
    src.exec("INSERT INTO snap VALUES (1, 'hello'), (2, 'snapshot'), (3, 'sqlite');")?;

    let blob = src.serialize()?;
    defer Database.freeSerialized(blob);
    expect_eq(blob.len >= 4096, true);

    let restored = Database.openMemory()?;
    defer restored.close();
    restored.deserialize(blob)?;

    let q1 = restored.prepare("SELECT count(*), group_concat(val, ',') FROM snap;")?;
    defer q1.close();
    expect_eq(q1.get()?, true);
    expect_eq(q1.columnInt(0), 3);
    expect_eq(bytesEq(q1.columnText(1), "hello,snapshot,sqlite"), true);

    let dst = Database.openMemory()?;
    defer dst.close();
    src.backupTo(&dst)?;

    let q2 = dst.prepare("SELECT count(*), max(id) FROM snap;")?;
    defer q2.close();
    expect_eq(q2.get()?, true);
    expect_eq(q2.columnInt(0), 3);
    expect_eq(q2.columnInt(1), 3);
}

fn runCase7Bench100k(): void! {
    let db = Database.open(
        ":memory:",
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY,
    )?;
    defer db.close();

    db.exec("PRAGMA synchronous = OFF; PRAGMA journal_mode = MEMORY; PRAGMA temp_store = MEMORY;")?;
    db.exec("CREATE TABLE bench (id INTEGER PRIMARY KEY, score INTEGER, tag TEXT);")?;

    const TOTAL_ROWS: i64 = 100000;
    const tag_val = "cp-sqlite-engine";

    const t0 = nowNs();
    db.begin()?;
    let ins = db.prepare("INSERT INTO bench (id, score, tag) VALUES (?1, ?2, ?3);")?;
    let i: i64 = 1;
    while (i <= TOTAL_ROWS) : (i += 1) {
        ins.bindInt(1, i)?;
        ins.bindInt(2, i * 7)?;
        ins.bindText(3, tag_val)?;
        ins.run()?;
    }
    ins.close();
    db.commit()?;
    const t1 = nowNs();

    let sel = db.prepare("SELECT score FROM bench WHERE id = ?1;")?;
    let checksum: i64 = 0;
    const t2 = nowNs();
    let k: i64 = 1;
    while (k <= TOTAL_ROWS) : (k += 1) {
        sel.bindInt(1, k)?;
        if (sel.step()?) {
            checksum += sel.columnInt(0);
        }
        sel.reset()?;
    }
    const t3 = nowNs();
    sel.close();

    expect_eq(checksum, 35000350000);

    const ins_ns: u64 = if (t1 > t0) (t1 - t0) else 1;
    const sel_ns: u64 = if (t3 > t2) (t3 - t2) else 1;
    const rows_u64: u64 = @intCast(TOTAL_ROWS);
    const ins_ops: u64 = (rows_u64 * 1000000000) / ins_ns;
    const sel_ops: u64 = (rows_u64 * 1000000000) / sel_ns;
    std.debug.print(
        "[BENCH sqlite :memory: 100k] insert={} ops/sec ({} ms) | point-lookup={} ops/sec ({} ms)\n",
        .{ ins_ops, ins_ns / 1000000, sel_ops, sel_ns / 1000000 },
    );
}

fn main() {
    test("1. memory CRUD, changes, lastInsertRowid, version", () => {
        runCase1Crud()?;
    });
    test("2. named parameters (:name, $name, @name) and Blob/Float/Null", () => {
        runCase2NamedAndTypes()?;
    });
    test("3. FTS5 full-text search, JSON1 queries, and math functions", () => {
        runCase3Fts5JsonMath()?;
    });
    test("4. transactions, rollback, and nested savepoints", () => {
        runCase4TxSavepoint()?;
    });
    test("5. LRU statement cache hit/miss reuse", () => {
        runCase5LruCache()?;
    });
    test("6. serialize/deserialize memory snapshot roundtrip and online hot backup", () => {
        runCase6SerializeBackup()?;
    });
    test("7. 100,000-row :memory: bulk insert + point-lookup benchmark", () => {
        runCase7Bench100k()?;
    });
}
