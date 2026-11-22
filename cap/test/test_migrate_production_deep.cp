// Source: cp/cap/test/test_migrate_production_deep.cp
// Deep Production Verification Suite for cap:migrate & cap.migrate
// Covers:
//   1. MigrationRegistry.register & parseSqlFile (Up/Down), out-of-order sorting, DuplicateVersion rejection
//   2. Tamper protection: ChecksumMismatch & MissingAppliedMigration interception
//   3. planDown reverse-order step planning & statusReport state summary
//   4. SqliteMigrator end-to-end on :memory: DB (migrateUp -> idempotent 0-change migrateUp -> rollback(1))
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.migrate`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { Database } from "cap:sqlite";
import cap from "cap";
import {
    MigrateError,
    MigrationDirection,
    MigrationStatus,
    MigrationSpec,
    AppliedMigration,
    MigrationPlanItem,
    MigrationStatusEntry,
    MigrationRegistry,
    SqliteMigrator,
} from "cap:devops";

test("cap:migrate - register, parseSqlFile, out-of-order planUp & DuplicateVersion") {
    const alloc = std.heap.page_allocator;
    let reg = MigrationRegistry.init(alloc);
    defer reg.deinit();

    // Register out-of-order: v3, v1, v2
    const s3 = reg.parseSqlFile(
        3,
        "add_orders",
        "-- migrate:up\r\nCREATE TABLE orders (id INTEGER PRIMARY KEY);\r\n-- migrate:down\r\nDROP TABLE orders;\r\n",
    ) catch unreachable;
    expect_eq(s3.version, 3);
    expect_eq(s3.checksum.len, 64);

    const s1 = reg.parseSqlFile(
        1,
        "create_users",
        "-- +migrate Up\nCREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT);\n-- +migrate Down\nDROP TABLE users;\n",
    ) catch unreachable;
    expect_eq(s1.version, 1);

    const s2 = reg.register(
        2,
        "add_email",
        "ALTER TABLE users ADD COLUMN email TEXT;",
        "ALTER TABLE users DROP COLUMN email;",
    ) catch unreachable;
    expect_eq(s2.version, 2);

    // DuplicateVersion must be rejected
    let dup_caught = false;
    * = reg.register(2, "dup_email", "SELECT 1;", "SELECT 1;") catch e => {
        if (e == MigrateError.DuplicateVersion) dup_caught = true;
    };
    expect_eq(dup_caught, true);

    // InvalidVersion (0) must be rejected
    let zero_caught = false;
    * = reg.register(0, "zero_ver", "SELECT 1;", "") catch e => {
        if (e == MigrateError.InvalidVersion) zero_caught = true;
    };
    expect_eq(zero_caught, true);

    // NonTransactionalNotSupported must be rejected
    let nontx_caught = false;
    * = reg.parseSqlFile(4, "vacuum_idx", "-- +migrate NO TRANSACTION\n-- +migrate Up\nVACUUM;\n") catch e => {
        if (e == MigrateError.NonTransactionalNotSupported) nontx_caught = true;
    };
    expect_eq(nontx_caught, true);

    // planUp on empty applied list returns [v1, v2, v3] in ascending order
    const empty_applied: [0]AppliedMigration = [];
    const plan = reg.planUp(alloc, empty_applied[0..0]) catch unreachable;
    defer alloc.free(plan);

    expect_eq(plan.len, 3);
    expect_eq(plan[0].version, 1);
    expect_eq(plan[0].direction == MigrationDirection.Up, true);
    expect_eq(plan[1].version, 2);
    expect_eq(plan[2].version, 3);
}

test("cap:migrate - tamper detection (ChecksumMismatch & MissingAppliedMigration)") {
    const alloc = std.heap.page_allocator;
    let reg = MigrationRegistry.init(alloc);
    defer reg.deinit();

    const s1 = reg.register(1, "init_schema", "CREATE TABLE t1 (id INT);", "DROP TABLE t1;") catch unreachable;
    const s2 = reg.register(2, "add_t2", "CREATE TABLE t2 (id INT);", "DROP TABLE t2;") catch unreachable;

    // 1. Tampered script: v1 has a different checksum than what was applied
    let reg_tampered = MigrationRegistry.init(alloc);
    defer reg_tampered.deinit();
    * = reg_tampered.register(1, "init_schema", "CREATE TABLE t1 (id INT, hacked TEXT);", "DROP TABLE t1;") catch unreachable;
    * = reg_tampered.register(2, "add_t2", "CREATE TABLE t2 (id INT);", "DROP TABLE t2;") catch unreachable;

    const applied_valid: [2]AppliedMigration = [
        AppliedMigration{ version: 1, name: s1.name, checksum: s1.checksum, applied_at_ms: 1000 },
        AppliedMigration{ version: 2, name: s2.name, checksum: s2.checksum, applied_at_ms: 2000 },
    ];

    let mismatch_caught = false;
    reg_tampered.verify(applied_valid[0..2]) catch e => {
        if (e == MigrateError.ChecksumMismatch) mismatch_caught = true;
    };
    expect_eq(mismatch_caught, true);

    // planUp must also reject tampered migrations
    let plan_up_rejected = false;
    * = reg_tampered.planUp(alloc, applied_valid[0..2]) catch e => {
        if (e == MigrateError.ChecksumMismatch) plan_up_rejected = true;
    };
    expect_eq(plan_up_rejected, true);

    // 2. Missing local migration script for an applied version
    const applied_missing: [3]AppliedMigration = [
        AppliedMigration{ version: 1, name: s1.name, checksum: s1.checksum, applied_at_ms: 1000 },
        AppliedMigration{ version: 2, name: s2.name, checksum: s2.checksum, applied_at_ms: 2000 },
        AppliedMigration{ version: 99, name: "deleted_migration", checksum: s2.checksum, applied_at_ms: 3000 },
    ];

    let missing_caught = false;
    reg.verify(applied_missing[0..3]) catch e => {
        if (e == MigrateError.MissingAppliedMigration) missing_caught = true;
    };
    expect_eq(missing_caught, true);
}

test("cap:migrate - planDown reverse-order steps & statusReport") {
    const alloc = std.heap.page_allocator;
    let reg = MigrationRegistry.init(alloc);
    defer reg.deinit();

    const s1 = reg.register(1, "v1", "CREATE TABLE a (id INT);", "DROP TABLE a;") catch unreachable;
    const s2 = reg.register(2, "v2", "CREATE TABLE b (id INT);", "DROP TABLE b;") catch unreachable;
    const s3 = reg.register(3, "v3", "CREATE TABLE c (id INT);", "DROP TABLE c;") catch unreachable;
    * = reg.register(4, "v4", "CREATE TABLE d (id INT);", "DROP TABLE d;") catch unreachable;

    // Applied: v1, v2, v3 (v4 is still Pending)
    const applied: [3]AppliedMigration = [
        AppliedMigration{ version: 1, name: s1.name, checksum: s1.checksum, applied_at_ms: 100 },
        AppliedMigration{ version: 2, name: s2.name, checksum: s2.checksum, applied_at_ms: 200 },
        AppliedMigration{ version: 3, name: s3.name, checksum: s3.checksum, applied_at_ms: 300 },
    ];

    // planDown(2) should return v3 then v2
    const down_plan = reg.planDown(alloc, applied[0..3], 2) catch unreachable;
    defer alloc.free(down_plan);

    expect_eq(down_plan.len, 2);
    expect_eq(down_plan[0].version, 3);
    expect_eq(down_plan[0].direction == MigrationDirection.Down, true);
    expect_eq(down_plan[0].sql, "DROP TABLE c;");
    expect_eq(down_plan[1].version, 2);
    expect_eq(down_plan[1].sql, "DROP TABLE b;");

    // statusReport should report v1..v3 Applied, v4 Pending
    const report = reg.statusReport(alloc, applied[0..3]) catch unreachable;
    defer alloc.free(report);

    expect_eq(report.len, 4);
    expect_eq(report[0].version, 1);
    expect_eq(report[0].status == MigrationStatus.Applied, true);
    expect_eq(report[1].status == MigrationStatus.Applied, true);
    expect_eq(report[2].status == MigrationStatus.Applied, true);
    expect_eq(report[3].version, 4);
    expect_eq(report[3].status == MigrationStatus.Pending, true);
}

test("cap:migrate - SqliteMigrator end-to-end (:memory: migrateUp, idempotent, rollback)") {
    const alloc = std.heap.page_allocator;
    let reg = MigrationRegistry.init(alloc);
    defer reg.deinit();

    * = reg.parseSqlFile(
        1,
        "create_items",
        "-- +migrate Up\nCREATE TABLE items (id INTEGER PRIMARY KEY, title TEXT NOT NULL);\nINSERT INTO items (id, title) VALUES (1, 'boot');\n" ++
        "-- +migrate Down\nDROP TABLE items;\n",
    ) catch unreachable;

    * = reg.parseSqlFile(
        2,
        "seed_more_items",
        "-- +migrate Up\nINSERT INTO items (id, title) VALUES (2, 'second');\n" ++
        "-- +migrate Down\nDELETE FROM items WHERE id = 2;\n",
    ) catch unreachable;

    const db = new Database(":memory:");

    // 1. Initial migrateUp applies 2 migrations
    const count1 = SqliteMigrator.migrateUp(alloc, db, reg, 1700000001000) catch unreachable;
    expect_eq(count1, 2);

    const row2 = (db.query_string("SELECT title FROM items WHERE id = 2;") catch unreachable) ?? "";
    expect_eq(row2, "second");

    // 2. Idempotent second migrateUp applies 0 migrations
    const count2 = SqliteMigrator.migrateUp(alloc, db, reg, 1700000002000) catch unreachable;
    expect_eq(count2, 0);

    // 3. Rollback 1 step removes v2 and deletes id=2 row
    const rb_count = SqliteMigrator.rollback(alloc, db, reg, 1) catch unreachable;
    expect_eq(rb_count, 1);

    const remaining = SqliteMigrator.loadApplied(alloc, db) catch unreachable;
    defer SqliteMigrator.freeApplied(alloc, remaining);
    expect_eq(remaining.len, 1);
    expect_eq(remaining[0].version, 1);

    const row2_after = (db.query_string("SELECT title FROM items WHERE id = 2;") catch unreachable) ?? "";
    expect_eq(row2_after, "");
    const row1_after = (db.query_string("SELECT title FROM items WHERE id = 1;") catch unreachable) ?? "";
    expect_eq(row1_after, "boot");
}

test("cap:migrate - dynamic namespace mounting via import cap from \"cap\"") {
    const alloc = std.heap.page_allocator;
    let reg = cap.devops.MigrationRegistry.init(alloc);
    defer reg.deinit();

    const spec = reg.register(
        10,
        "create_logs",
        "CREATE TABLE logs (id INTEGER PRIMARY KEY, msg TEXT);",
        "DROP TABLE logs;",
    ) catch unreachable;
    expect_eq(spec.version, 10);

    const db = new Database(":memory:");
    const n = cap.devops.SqliteMigrator.migrateUp(alloc, db, reg, 1700000010000) catch unreachable;
    expect_eq(n, 1);

    const applied = cap.devops.SqliteMigrator.loadApplied(alloc, db) catch unreachable;
    defer cap.devops.SqliteMigrator.freeApplied(alloc, applied);
    expect_eq(applied.len, 1);
    expect_eq(applied[0].version, 10);
    expect_eq(applied[0].name, "create_logs");
}
