// Source: Absorbed from /data/dev/bun/src/sql/shared/ColumnIdentifier.cp | Synced: 2026-11-23T01:32:31Z
// ============================================================================
// lib/sql/columnId.cp — Zero-Alloc SQL ColumnIdentifier & Duplicate-Column Map
// ============================================================================
// Implements Bun's shared SQL `ColumnIdentifier` classification (PostgreSQL,
// MySQL, SQLite):
//   - Numeric column names (`"0"` .. `"4294967294"`, length 1..10, strictly
//     `< u32::MAX`) are classified as fast array-index identifiers (`.index`).
//   - Empty, non-numeric, or `>= u32::MAX` names stay as zero-copy `.name`.
//   - `SqlColumnIdMap` tracks up to 64 result-set columns with zero heap
//     allocation, marking shadowed earlier columns as `.duplicate` (matching
//     Bun's object-mode last-column-wins semantics while flagging `hasDuplicates()`
//     so callers know when `.values()` positional mode is required).

pub const SQL_COLUMN_ID_MAX_COLS: usize = 64;
/// Maximum valid array-index column identifier (`u32::MAX - 1`, aligned with Bun).
pub const SQL_COLUMN_ID_MAX_INDEX: u64 = 4294967294;

pub err SqlColumnIdError {
    TooManyColumns,
    BufferTooSmall,
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Classification kind of a SQL result-set column identifier.
pub enum SqlColumnIdKind {
    name,
    index,
    duplicate,
}

/// Zero-allocation SQL column identifier (aligned with Bun `ColumnIdentifier`).
pub struct SqlColumnId {
    kind: SqlColumnIdKind = SqlColumnIdKind.name,
    name_slice: []const u8 = "",
    index_val: u32 = 0,

    /// Classifies a raw SQL column name into `.index` or `.name`:
    /// - `""` -> `.name("")`
    /// - 1..10 ASCII digits with numeric value `< 4294967295` -> `.index(val)`
    /// - otherwise -> `.name(raw)`
    static fn parse(raw: []const u8): SqlColumnId {
        if (raw.len >= 1 && raw.len <= 10) {
            let is_int = true;
            let acc: u64 = 0;
            let i: usize = 0;
            while (i < raw.len) {
                const b = raw[i];
                if (b >= '0' && b <= '9') {
                    acc = acc * 10 + @as(u64, b - '0');
                } else {
                    is_int = false;
                    break;
                }
                i += 1;
            }
            if (is_int && acc <= SQL_COLUMN_ID_MAX_INDEX) {
                return SqlColumnId{
                    kind: SqlColumnIdKind.index,
                    name_slice: raw,
                    index_val: @as(u32, @intCast(acc)),
                };
            }
        }
        return SqlColumnId{
            kind: SqlColumnIdKind.name,
            name_slice: raw,
            index_val: 0,
        };
    }

    static fn fromIndex(idx: u32): SqlColumnId {
        return SqlColumnId{
            kind: SqlColumnIdKind.index,
            name_slice: "",
            index_val: idx,
        };
    }

    static fn duplicateOf(raw: []const u8): SqlColumnId {
        return SqlColumnId{
            kind: SqlColumnIdKind.duplicate,
            name_slice: raw,
            index_val: 0,
        };
    }

    fn isName(): bool {
        return _.kind == SqlColumnIdKind.name;
    }

    fn isIndex(): bool {
        return _.kind == SqlColumnIdKind.index;
    }

    fn isDuplicate(): bool {
        return _.kind == SqlColumnIdKind.duplicate;
    }

    /// Returns whether two non-duplicate identifiers represent the same object key.
    fn sameKey(other: SqlColumnId): bool {
        if (_.kind == SqlColumnIdKind.duplicate || other.kind == SqlColumnIdKind.duplicate) {
            return false;
        }
        if (_.kind != other.kind) return false;
        if (_.kind == SqlColumnIdKind.index) {
            return _.index_val == other.index_val;
        }
        return eqlBytes(_.name_slice, other.name_slice);
    }

    fn eql(other: SqlColumnId): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == SqlColumnIdKind.index) {
            return _.index_val == other.index_val;
        }
        return eqlBytes(_.name_slice, other.name_slice);
    }
}

/// Fixed-capacity result-set column header map tracking numeric indices and
/// duplicate column shadowing (last-column-wins in object mode, full positional
/// access preserved for `.values()` / `.raw()` modes).
pub struct SqlColumnIdMap {
    cols: [64]SqlColumnId = [SqlColumnId{}; 64],
    raw_names: [64][]const u8 = [""; 64],
    len: usize = 0,
    dup_count: usize = 0,

    static fn init(): SqlColumnIdMap {
        return SqlColumnIdMap{};
    }

    /// Appends a column to the result-set schema. If an earlier column shares
    /// the same identifier key (name or numeric index), the earlier slot is
    /// marked `.duplicate` (last-column-wins in object mode) and `dup_count`
    /// is incremented. Returns the 0-based positional index of the added column.
    fn addColumn(*_, raw_name: []const u8): SqlColumnIdError!usize {
        if (_.len >= SQL_COLUMN_ID_MAX_COLS) return SqlColumnIdError.TooManyColumns;
        const id = SqlColumnId.parse(raw_name);
        let i: usize = 0;
        while (i < _.len) {
            if (_.cols[i].sameKey(id)) {
                _.cols[i] = SqlColumnId.duplicateOf(_.raw_names[i]);
                _.dup_count += 1;
            }
            i += 1;
        }
        const pos = _.len;
        _.cols[pos] = id;
        _.raw_names[pos] = raw_name;
        _.len += 1;
        return pos;
    }

    fn count(): usize {
        return _.len;
    }

    fn duplicateCount(): usize {
        return _.dup_count;
    }

    fn activeCount(): usize {
        return _.len - _.dup_count;
    }

    fn hasDuplicates(): bool {
        return _.dup_count > 0;
    }

    fn get(pos: usize): SqlColumnId? {
        if (pos >= _.len) return null;
        return _.cols[pos];
    }

    fn rawNameAt(pos: usize): ([]const u8)? {
        if (pos >= _.len) return null;
        return _.raw_names[pos];
    }

    /// Looks up the active (non-shadowed) column position for a string key.
    fn lookupByName(key: []const u8): usize? {
        const target = SqlColumnId.parse(key);
        let i: usize = 0;
        while (i < _.len) {
            if (_.cols[i].sameKey(target)) return i;
            i += 1;
        }
        return null;
    }

    /// Looks up the active (non-shadowed) column position for a numeric index key.
    fn lookupByIndex(idx: u32): usize? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.cols[i].kind == SqlColumnIdKind.index && _.cols[i].index_val == idx) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// Formats all positional raw column names separated by `","` into `out`.
    fn formatHeaderList(out: []u8): SqlColumnIdError!usize {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                if (pos >= out.len) return SqlColumnIdError.BufferTooSmall;
                out[pos] = ',';
                pos += 1;
            }
            const name = _.raw_names[i];
            if (pos + name.len > out.len) return SqlColumnIdError.BufferTooSmall;
            let j: usize = 0;
            while (j < name.len) {
                out[pos + j] = name[j];
                j += 1;
            }
            pos += name.len;
            i += 1;
        }
        return pos;
    }
}
