// Source: cp/lib/migrate/migrator.cp (Deterministic Schema Migration Planner & Tamper-Evident Runner)
// Pure CP zero-C-dependency database/schema migration engine.
// Supports -- +migrate Up / Down (and -- migrate:up / down) SQL script parsing,
// deterministic version ordering & duplicate rejection, canonical SHA-256 checksum
// tamper detection, up/down execution planning, and SqliteMigrator over cap:sqlite.

const std = @import("std");
import { sha256Raw } from "cap:crypto";
import { Database } from "cap:sqlite";

pub err MigrateError {
    InvalidVersion,
    DuplicateVersion,
    ChecksumMismatch,
    MissingAppliedMigration,
    NonTransactionalNotSupported,
    InvalidMigrationFormat,
    SqlExecutionFailed,
    OutOfMemory,
}

pub enum MigrationDirection {
    Up,
    Down,

    pub fn toString(): string {
        switch (_) {
            .Up => return "Up",
            .Down => return "Down",
        }
    }
}

pub enum MigrationStatus {
    Pending,
    Applied,
    DriftDetected,

    pub fn toString(): string {
        switch (_) {
            .Pending => return "Pending",
            .Applied => return "Applied",
            .DriftDetected => return "DriftDetected",
        }
    }
}

pub struct MigrationSpec {
    version: u64;
    name: string;
    up_sql: string;
    down_sql: string;
    checksum: string;
}

pub struct AppliedMigration {
    version: u64;
    name: string;
    checksum: string;
    applied_at_ms: u64;
}

pub struct MigrationPlanItem {
    version: u64;
    name: string;
    direction: MigrationDirection;
    sql: string;
    checksum: string;
}

pub struct MigrationStatusEntry {
    version: u64;
    name: string;
    status: MigrationStatus;
    local_checksum: string;
    applied_checksum: string;
}

fn appendU64Be(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, val: u64): !MigrateError {
    let bytes: [8]u8 = undefined;
    std.mem.writeInt(u64, &bytes, val, .big);
    buf.appendSlice(alloc, bytes[0..8]) catch return MigrateError.OutOfMemory;
}

fn appendLenPrefixed(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, s: string): !MigrateError {
    appendU64Be(buf, alloc, u64(s.len))?;
    buf.appendSlice(alloc, s) catch return MigrateError.OutOfMemory;
}

fn normalizeSqlOwned(alloc: std.mem.Allocator, sql: string): []u8 ! MigrateError {
    const trimmed = std.mem.trim(u8, sql, " \t\r\n");
    let buf: std.ArrayListUnmanaged(u8) = .empty;
    errdefer buf.deinit(alloc);

    for (c of trimmed) {
        if (c == '\r') continue;
        buf.append(alloc, c) catch return MigrateError.OutOfMemory;
    }
    return buf.toOwnedSlice(alloc) catch return MigrateError.OutOfMemory;
}

fn dupSlice(alloc: std.mem.Allocator, s: string): []u8 ! MigrateError {
    const out = alloc.alloc(u8, s.len) catch return MigrateError.OutOfMemory;
    @memcpy(out, s);
    return out;
}

fn computeMigrationChecksumHex(
    alloc: std.mem.Allocator,
    version: u64,
    name: string,
    up_sql: string,
    down_sql: string,
): []u8 ! MigrateError {
    let buf: std.ArrayListUnmanaged(u8) = .empty;
    defer buf.deinit(alloc);

    buf.appendSlice(alloc, "CP-MIGRATE-V1\x00") catch return MigrateError.OutOfMemory;
    appendU64Be(&buf, alloc, version)?;
    appendLenPrefixed(&buf, alloc, name)?;
    appendLenPrefixed(&buf, alloc, up_sql)?;
    appendLenPrefixed(&buf, alloc, down_sql)?;

    let digest: [32]u8 = undefined;
    sha256Raw(&digest, buf.items);
    const hex = std.fmt.bytesToHex(digest, .lower);

    const out = alloc.alloc(u8, 64) catch return MigrateError.OutOfMemory;
    @memcpy(out, hex[0..64]);
    return out;
}

fn eqlIgnoreCase(a: string, b: string): bool {
    return std.ascii.eqlIgnoreCase(a, b);
}

fn startsWithIgnoreCase(haystack: string, prefix: string): bool {
    if (haystack.len < prefix.len) return false;
    return std.ascii.eqlIgnoreCase(haystack[0..prefix.len], prefix);
}

enum ParsedDirective {
    None,
    Up,
    Down,
    NoTransaction,
    StatementSkip,
}

fn classifyDirectiveLine(line: string): ParsedDirective {
    const t = std.mem.trim(u8, line, " \t\r");
    if (t.len < 2 || t[0] != '-' || t[1] != '-') return ParsedDirective.None;

    if (eqlIgnoreCase(t, "-- +migrate up") || eqlIgnoreCase(t, "-- migrate:up")) {
        return ParsedDirective.Up;
    }
    if (eqlIgnoreCase(t, "-- +migrate down") || eqlIgnoreCase(t, "-- migrate:down")) {
        return ParsedDirective.Down;
    }
    if (startsWithIgnoreCase(t, "-- +migrate no transaction") ||
        startsWithIgnoreCase(t, "-- +migrate notransaction") ||
        startsWithIgnoreCase(t, "-- migrate:no-transaction"))
    {
        return ParsedDirective.NoTransaction;
    }
    if (eqlIgnoreCase(t, "-- +migrate statementbegin") || eqlIgnoreCase(t, "-- +migrate statementend")) {
        return ParsedDirective.StatementSkip;
    }
    return ParsedDirective.None;
}

fn hasVersion(items: []const AppliedMigration, version: u64): bool {
    for (item of items) {
        if (item.version == version) return true;
    }
    return false;
}

fn findApplied(items: []const AppliedMigration, version: u64): AppliedMigration? {
    for (item of items) {
        if (item.version == version) return item;
    }
    return null;
}

pub struct MigrationRegistry {
    alloc: std.mem.Allocator;
    specs: std.ArrayListUnmanaged(MigrationSpec);
    owned_strings: std.ArrayListUnmanaged([]u8);

    pub static fn init(alloc: std.mem.Allocator): MigrationRegistry {
        return MigrationRegistry{
            alloc: alloc,
            specs: .empty,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        for (s of _.owned_strings.items) {
            _.alloc.free(s);
        }
        _.owned_strings.deinit(_.alloc);
        _.specs.deinit(_.alloc);
    }

    pub fn len(): usize {
        return _.specs.items.len;
    }

    pub fn get(version: u64): MigrationSpec? {
        for (spec of _.specs.items) {
            if (spec.version == version) return spec;
        }
        return null;
    }

    pub fn register(
        *_,
        version: u64,
        name: string,
        up_sql: string,
        down_sql: string,
    ): MigrationSpec ! MigrateError {
        if (version == 0) return MigrateError.InvalidVersion;

        const trimmed_name = std.mem.trim(u8, name, " \t\r\n");
        if (trimmed_name.len == 0) return MigrateError.InvalidMigrationFormat;

        for (existing of _.specs.items) {
            if (existing.version == version) return MigrateError.DuplicateVersion;
        }

        const owned_name = dupSlice(_.alloc, trimmed_name)?;
        errdefer _.alloc.free(owned_name);

        const owned_up = normalizeSqlOwned(_.alloc, up_sql)?;
        errdefer _.alloc.free(owned_up);

        const owned_down = normalizeSqlOwned(_.alloc, down_sql)?;
        errdefer _.alloc.free(owned_down);

        const owned_sum = computeMigrationChecksumHex(_.alloc, version, owned_name, owned_up, owned_down)?;
        errdefer _.alloc.free(owned_sum);

        _.owned_strings.append(_.alloc, owned_name) catch return MigrateError.OutOfMemory;
        _.owned_strings.append(_.alloc, owned_up) catch return MigrateError.OutOfMemory;
        _.owned_strings.append(_.alloc, owned_down) catch return MigrateError.OutOfMemory;
        _.owned_strings.append(_.alloc, owned_sum) catch return MigrateError.OutOfMemory;

        const spec = MigrationSpec{
            version: version,
            name: owned_name,
            up_sql: owned_up,
            down_sql: owned_down,
            checksum: owned_sum,
        };

        // Maintain ascending version order deterministically.
        let insert_idx: usize = _.specs.items.len;
        let idx: usize = 0;
        while (idx < _.specs.items.len) : (idx += 1) {
            if (_.specs.items[idx].version > version) {
                insert_idx = idx;
                break;
            }
        }
        _.specs.insert(_.alloc, insert_idx, spec) catch return MigrateError.OutOfMemory;
        return spec;
    }

    pub fn parseSqlFile(
        *_,
        version: u64,
        name: string,
        content: string,
    ): MigrationSpec ! MigrateError {
        if (version == 0) return MigrateError.InvalidVersion;

        let up_buf: std.ArrayListUnmanaged(u8) = .empty;
        defer up_buf.deinit(_.alloc);
        let down_buf: std.ArrayListUnmanaged(u8) = .empty;
        defer down_buf.deinit(_.alloc);

        let seen_up = false;
        let seen_down = false;
        let current_section: u8 = 0; // 0 = none, 1 = up, 2 = down

        let pos: usize = 0;
        while (pos <= content.len) {
            let line_end: usize = pos;
            while (line_end < content.len && content[line_end] != '\n') : (line_end += 1) {}
            const raw_line = content[pos..line_end];
            const clean_line = if (raw_line.len > 0 && raw_line[raw_line.len - 1] == '\r')
                raw_line[0..raw_line.len - 1]
            else
                raw_line;

            const directive = classifyDirectiveLine(clean_line);
            switch (directive) {
                .NoTransaction => return MigrateError.NonTransactionalNotSupported,
                .Up => {
                    if (seen_up) return MigrateError.InvalidMigrationFormat;
                    seen_up = true;
                    current_section = 1;
                },
                .Down => {
                    if (seen_down) return MigrateError.InvalidMigrationFormat;
                    seen_down = true;
                    current_section = 2;
                },
                .StatementSkip => {},
                .None => {
                    if (current_section == 1) {
                        if (up_buf.items.len > 0) {
                            up_buf.append(_.alloc, '\n') catch return MigrateError.OutOfMemory;
                        }
                        up_buf.appendSlice(_.alloc, clean_line) catch return MigrateError.OutOfMemory;
                    } else if (current_section == 2) {
                        if (down_buf.items.len > 0) {
                            down_buf.append(_.alloc, '\n') catch return MigrateError.OutOfMemory;
                        }
                        down_buf.appendSlice(_.alloc, clean_line) catch return MigrateError.OutOfMemory;
                    }
                },
            }

            if (line_end >= content.len) break;
            pos = line_end + 1;
        }

        if (!seen_up) return MigrateError.InvalidMigrationFormat;
        const up_trimmed = std.mem.trim(u8, up_buf.items, " \t\r\n");
        if (up_trimmed.len == 0) return MigrateError.InvalidMigrationFormat;

        return _.register(version, name, up_buf.items, down_buf.items);
    }

    pub fn verify(applied: []const AppliedMigration): !MigrateError {
        for (app of applied) {
            const spec_opt = _.get(app.version);
            if (spec := spec_opt) {
                if (!std.mem.eql(u8, spec.checksum, app.checksum)) {
                    return MigrateError.ChecksumMismatch;
                }
            } else {
                return MigrateError.MissingAppliedMigration;
            }
        }
    }

    pub fn planUp(
        alloc: std.mem.Allocator,
        applied: []const AppliedMigration,
    ): []MigrationPlanItem ! MigrateError {
        _.verify(applied)?;

        let plan: std.ArrayListUnmanaged(MigrationPlanItem) = .empty;
        errdefer plan.deinit(alloc);

        for (spec of _.specs.items) {
            if (!hasVersion(applied, spec.version)) {
                plan.append(alloc, MigrationPlanItem{
                    version: spec.version,
                    name: spec.name,
                    direction: MigrationDirection.Up,
                    sql: spec.up_sql,
                    checksum: spec.checksum,
                }) catch return MigrateError.OutOfMemory;
            }
        }

        return plan.toOwnedSlice(alloc) catch return MigrateError.OutOfMemory;
    }

    pub fn planDown(
        alloc: std.mem.Allocator,
        applied: []const AppliedMigration,
        steps: usize,
    ): []MigrationPlanItem ! MigrateError {
        _.verify(applied)?;

        if (steps == 0 || applied.len == 0) {
            return alloc.alloc(MigrationPlanItem, 0) catch return MigrateError.OutOfMemory;
        }

        let versions: std.ArrayListUnmanaged(u64) = .empty;
        defer versions.deinit(alloc);

        for (app of applied) {
            versions.append(alloc, app.version) catch return MigrateError.OutOfMemory;
        }

        // Sort descending by version.
        let i: usize = 0;
        while (i < versions.items.len) : (i += 1) {
            let j: usize = i + 1;
            while (j < versions.items.len) : (j += 1) {
                if (versions.items[j] > versions.items[i]) {
                    const tmp = versions.items[i];
                    versions.items[i] = versions.items[j];
                    versions.items[j] = tmp;
                }
            }
        }

        const limit = if (steps < versions.items.len) steps else versions.items.len;
        let plan: std.ArrayListUnmanaged(MigrationPlanItem) = .empty;
        errdefer plan.deinit(alloc);

        let k: usize = 0;
        while (k < limit) : (k += 1) {
            const ver = versions.items[k];
            const spec = _.get(ver) ?? return MigrateError.MissingAppliedMigration;
            plan.append(alloc, MigrationPlanItem{
                version: spec.version,
                name: spec.name,
                direction: MigrationDirection.Down,
                sql: spec.down_sql,
                checksum: spec.checksum,
            }) catch return MigrateError.OutOfMemory;
        }

        return plan.toOwnedSlice(alloc) catch return MigrateError.OutOfMemory;
    }

    pub fn statusReport(
        alloc: std.mem.Allocator,
        applied: []const AppliedMigration,
    ): []MigrationStatusEntry ! MigrateError {
        let entries: std.ArrayListUnmanaged(MigrationStatusEntry) = .empty;
        errdefer entries.deinit(alloc);

        for (spec of _.specs.items) {
            if (app := findApplied(applied, spec.version)) {
                const st = if (std.mem.eql(u8, spec.checksum, app.checksum))
                    MigrationStatus.Applied
                else
                    MigrationStatus.DriftDetected;
                entries.append(alloc, MigrationStatusEntry{
                    version: spec.version,
                    name: spec.name,
                    status: st,
                    local_checksum: spec.checksum,
                    applied_checksum: app.checksum,
                }) catch return MigrateError.OutOfMemory;
            } else {
                entries.append(alloc, MigrationStatusEntry{
                    version: spec.version,
                    name: spec.name,
                    status: MigrationStatus.Pending,
                    local_checksum: spec.checksum,
                    applied_checksum: "",
                }) catch return MigrateError.OutOfMemory;
            }
        }

        for (app of applied) {
            if (_.get(app.version) == null) {
                entries.append(alloc, MigrationStatusEntry{
                    version: app.version,
                    name: app.name,
                    status: MigrationStatus.DriftDetected,
                    local_checksum: "",
                    applied_checksum: app.checksum,
                }) catch return MigrateError.OutOfMemory;
            }
        }

        return entries.toOwnedSlice(alloc) catch return MigrateError.OutOfMemory;
    }
}

fn appendSqlEscapedString(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, s: string): !MigrateError {
    buf.append(alloc, '\'') catch return MigrateError.OutOfMemory;
    for (c of s) {
        if (c == '\'') {
            buf.appendSlice(alloc, "''") catch return MigrateError.OutOfMemory;
        } else {
            buf.append(alloc, c) catch return MigrateError.OutOfMemory;
        }
    }
    buf.append(alloc, '\'') catch return MigrateError.OutOfMemory;
}

fn decodeHexOwned(alloc: std.mem.Allocator, hex_str: string): []u8 ! MigrateError {
    if (hex_str.len % 2 != 0) return MigrateError.InvalidMigrationFormat;
    const out_len = hex_str.len / 2;
    let out = alloc.alloc(u8, out_len) catch return MigrateError.OutOfMemory;
    errdefer alloc.free(out);

    let i: usize = 0;
    while (i < out_len) : (i += 1) {
        const byte_val = std.fmt.parseInt(u8, hex_str[i * 2 .. i * 2 + 2], 16) catch return MigrateError.InvalidMigrationFormat;
        out[i] = byte_val;
    }
    return out;
}

pub static class SqliteMigrator {
    pub fn ensureTable(db: *Database): !MigrateError {
        db.exec(
            "CREATE TABLE IF NOT EXISTS _cap_migrations (" ++
            "version INTEGER PRIMARY KEY, " ++
            "name TEXT NOT NULL, " ++
            "checksum TEXT NOT NULL, " ++
            "applied_at_ms INTEGER NOT NULL);",
        ) catch return MigrateError.SqlExecutionFailed;
    }

    pub fn freeApplied(alloc: std.mem.Allocator, items: []AppliedMigration) {
        for (item of items) {
            alloc.free(item.name);
            alloc.free(item.checksum);
        }
        alloc.free(items);
    }

    pub fn loadApplied(alloc: std.mem.Allocator, db: *Database): []AppliedMigration ! MigrateError {
        ensureTable(db)?;

        const raw_opt = db.query_string(
            "SELECT COALESCE(GROUP_CONCAT(row_data, char(10)), '') FROM (" ++
            "SELECT version || char(9) || hex(CAST(name AS BLOB)) || char(9) || checksum || char(9) || applied_at_ms AS row_data " ++
            "FROM _cap_migrations ORDER BY version ASC);",
        ) catch return MigrateError.SqlExecutionFailed;

        const raw = raw_opt ?? "";
        if (raw.len == 0) {
            return alloc.alloc(AppliedMigration, 0) catch return MigrateError.OutOfMemory;
        }
        defer std.heap.page_allocator.free(raw);

        let list: std.ArrayListUnmanaged(AppliedMigration) = .empty;
        errdefer {
            for (item of list.items) {
                alloc.free(item.name);
                alloc.free(item.checksum);
            }
            list.deinit(alloc);
        }

        let pos: usize = 0;
        while (pos < raw.len) {
            let line_end: usize = pos;
            while (line_end < raw.len && raw[line_end] != '\n') : (line_end += 1) {}
            const line = raw[pos..line_end];
            if (line.len > 0) {
                let it = std.mem.splitScalar(u8, line, '\t');
                const ver_s = it.next() ?? return MigrateError.InvalidMigrationFormat;
                const name_hex = it.next() ?? return MigrateError.InvalidMigrationFormat;
                const sum_s = it.next() ?? return MigrateError.InvalidMigrationFormat;
                const ts_s = it.next() ?? return MigrateError.InvalidMigrationFormat;

                const ver = std.fmt.parseInt(u64, ver_s, 10) catch return MigrateError.InvalidMigrationFormat;
                const ts = std.fmt.parseInt(u64, ts_s, 10) catch return MigrateError.InvalidMigrationFormat;
                const name_owned = decodeHexOwned(alloc, name_hex)?;
                errdefer alloc.free(name_owned);
                const sum_owned = dupSlice(alloc, sum_s)?;
                errdefer alloc.free(sum_owned);

                list.append(alloc, AppliedMigration{
                    version: ver,
                    name: name_owned,
                    checksum: sum_owned,
                    applied_at_ms: ts,
                }) catch return MigrateError.OutOfMemory;
            }
            if (line_end >= raw.len) break;
            pos = line_end + 1;
        }

        return list.toOwnedSlice(alloc) catch return MigrateError.OutOfMemory;
    }

    pub fn migrateUp(
        alloc: std.mem.Allocator,
        db: *Database,
        registry: MigrationRegistry,
        now_ms: u64,
    ): usize ! MigrateError {
        const applied = loadApplied(alloc, db)?;
        defer freeApplied(alloc, applied);

        const plan = registry.planUp(alloc, applied)?;
        defer alloc.free(plan);

        if (plan.len == 0) return 0;

        db.exec("BEGIN IMMEDIATE;") catch return MigrateError.SqlExecutionFailed;

        for (item of plan) {
            if (item.sql.len > 0) {
                db.exec(item.sql) catch {
                    * = db.exec("ROLLBACK;");
                    return MigrateError.SqlExecutionFailed;
                };
            }

            let sql_buf: std.ArrayListUnmanaged(u8) = .empty;
            defer sql_buf.deinit(alloc);

            const prefix = std.fmt.allocPrint(
                alloc,
                "INSERT INTO _cap_migrations (version, name, checksum, applied_at_ms) VALUES ({d}, ",
                .{item.version},
            ) catch {
                * = db.exec("ROLLBACK;");
                return MigrateError.OutOfMemory;
            };
            defer alloc.free(prefix);

            sql_buf.appendSlice(alloc, prefix) catch {
                * = db.exec("ROLLBACK;");
                return MigrateError.OutOfMemory;
            };
            appendSqlEscapedString(&sql_buf, alloc, item.name) catch {
                * = db.exec("ROLLBACK;");
                return MigrateError.OutOfMemory;
            };
            sql_buf.appendSlice(alloc, ", ") catch {
                * = db.exec("ROLLBACK;");
                return MigrateError.OutOfMemory;
            };
            appendSqlEscapedString(&sql_buf, alloc, item.checksum) catch {
                * = db.exec("ROLLBACK;");
                return MigrateError.OutOfMemory;
            };
            const suffix = std.fmt.allocPrint(alloc, ", {d});", .{now_ms}) catch {
                * = db.exec("ROLLBACK;");
                return MigrateError.OutOfMemory;
            };
            defer alloc.free(suffix);
            sql_buf.appendSlice(alloc, suffix) catch {
                * = db.exec("ROLLBACK;");
                return MigrateError.OutOfMemory;
            };

            db.exec(sql_buf.items) catch {
                * = db.exec("ROLLBACK;");
                return MigrateError.SqlExecutionFailed;
            };
        }

        db.exec("COMMIT;") catch {
            * = db.exec("ROLLBACK;");
            return MigrateError.SqlExecutionFailed;
        };

        return plan.len;
    }

    pub fn rollback(
        alloc: std.mem.Allocator,
        db: *Database,
        registry: MigrationRegistry,
        steps: usize,
    ): usize ! MigrateError {
        const applied = loadApplied(alloc, db)?;
        defer freeApplied(alloc, applied);

        const plan = registry.planDown(alloc, applied, steps)?;
        defer alloc.free(plan);

        if (plan.len == 0) return 0;

        db.exec("BEGIN IMMEDIATE;") catch return MigrateError.SqlExecutionFailed;

        for (item of plan) {
            if (item.sql.len > 0) {
                db.exec(item.sql) catch {
                    * = db.exec("ROLLBACK;");
                    return MigrateError.SqlExecutionFailed;
                };
            }

            const del_sql = std.fmt.allocPrint(
                alloc,
                "DELETE FROM _cap_migrations WHERE version = {d};",
                .{item.version},
            ) catch {
                * = db.exec("ROLLBACK;");
                return MigrateError.OutOfMemory;
            };
            defer alloc.free(del_sql);

            db.exec(del_sql) catch {
                * = db.exec("ROLLBACK;");
                return MigrateError.SqlExecutionFailed;
            };
        }

        db.exec("COMMIT;") catch {
            * = db.exec("ROLLBACK;");
            return MigrateError.SqlExecutionFailed;
        };

        return plan.len;
    }
}
