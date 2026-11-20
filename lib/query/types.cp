// Source: cp/lib/query/types.cp (SQL Dialect, Parameter Values & Safe Identifier Quoting)
// Pure CP SQL dialect definitions, typed query parameter values, owned BuiltQuery output,
// and injection-safe identifier quoting across PostgreSQL, MySQL, and SQLite3.

const std = @import("std");

/// Error set returned by identifier validation and parameterized SQL query builders.
pub err QueryError {
    InvalidIdentifier,
    EmptyTable,
    EmptyValues,
    OutOfMemory,
}

/// Target SQL dialect controlling placeholder numbering and identifier quote delimiters.
pub enum QueryDialect: u8 {
    Postgres = 0,
    MySql = 1,
    Sqlite = 2,

    /// Returns the dialect-specific identifier quote byte (`"` for Postgres/Sqlite, `` ` `` for MySql).
    pub fn quoteChar(): u8 {
        return switch (_) {
            .Postgres, .Sqlite => '"',
            .MySql => '`',
        };
    }

    /// Returns `true` when placeholders use 1-based `$N` numbering (PostgreSQL).
    pub fn usesNumberedPlaceholders(): bool {
        return _ == .Postgres;
    }
}

/// Strongly-typed parameterized SQL bind value.
/// Values are strictly isolated in `BuiltQuery.params` and never interpolated into SQL text.
pub enum QueryParam {
    Null,
    Bool: bool,
    Int: i64,
    Float: f64,
    Text: string,
    Bytes: []const u8,

    /// Returns `true` when this parameter represents SQL `NULL`.
    pub fn isNull(): bool {
        return switch (_) {
            .Null => true,
            else => false,
        };
    }

    /// Extracts the boolean payload, or `null` if this parameter is another variant.
    pub fn asBool(): bool? {
        return switch (_) {
            .Bool v => v,
            else => null,
        };
    }

    /// Extracts the 64-bit signed integer payload, or `null` if another variant.
    pub fn asInt(): i64? {
        return switch (_) {
            .Int v => v,
            else => null,
        };
    }

    /// Extracts the 64-bit float payload, or `null` if another variant.
    pub fn asFloat(): f64? {
        return switch (_) {
            .Float v => v,
            else => null,
        };
    }

    /// Extracts the UTF-8 text slice payload, or `null` if another variant.
    pub fn asText(): string? {
        return switch (_) {
            .Text v => v,
            else => null,
        };
    }

    /// Extracts the raw binary byte slice payload, or `null` if another variant.
    pub fn asBytes(): ([]const u8)? {
        return switch (_) {
            .Bytes v => v,
            else => null,
        };
    }

    pub static fn makeNull(): QueryParam {
        return QueryParam.Null;
    }

    pub static fn makeBool(v: bool): QueryParam {
        return QueryParam.Bool(v);
    }

    pub static fn makeInt(v: i64): QueryParam {
        return QueryParam.Int(v);
    }

    pub static fn makeFloat(v: f64): QueryParam {
        return QueryParam.Float(v);
    }

    pub static fn makeText(v: string): QueryParam {
        return QueryParam.Text(v);
    }

    pub static fn makeBytes(v: []const u8): QueryParam {
        return QueryParam.Bytes(v);
    }

    /// Produces an ownership-independent deep copy of this parameter using `alloc`.
    pub fn clone(alloc: std.mem.Allocator): QueryParam ! QueryError {
        return switch (_) {
            .Null => QueryParam.Null,
            .Bool v => QueryParam.Bool(v),
            .Int v => QueryParam.Int(v),
            .Float v => QueryParam.Float(v),
            .Text s => {
                if (s.len == 0) return QueryParam.Text("");
                const duped = alloc.dupe(u8, s) catch return QueryError.OutOfMemory;
                return QueryParam.Text(duped);
            },
            .Bytes b => {
                if (b.len == 0) return QueryParam.Bytes("");
                const duped = alloc.dupe(u8, b) catch return QueryError.OutOfMemory;
                return QueryParam.Bytes(duped);
            },
        };
    }

    /// Releases any owned heap slice held by `Text` or `Bytes` variants.
    pub fn deinit(alloc: std.mem.Allocator) {
        switch (_) {
            .Text s => {
                if (s.len > 0) alloc.free(s);
            },
            .Bytes b => {
                if (b.len > 0) alloc.free(b);
            },
            else => {},
        }
    }

    /// Deep structural equality check between two `QueryParam` values.
    pub fn eql(other: QueryParam): bool {
        return switch (_) {
            .Null => switch (other) {
                .Null => true,
                else => false,
            },
            .Bool a => switch (other) {
                .Bool b => a == b,
                else => false,
            },
            .Int a => switch (other) {
                .Int b => a == b,
                else => false,
            },
            .Float a => switch (other) {
                .Float b => a == b,
                else => false,
            },
            .Text a => switch (other) {
                .Text b => std.mem.eql(u8, a, b),
                else => false,
            },
            .Bytes a => switch (other) {
                .Bytes b => std.mem.eql(u8, a, b),
                else => false,
            },
        };
    }
}

/// Owned parameterized SQL query output produced by query builders.
/// Call `deinit()` to free the generated SQL string and cloned parameter buffers.
pub struct BuiltQuery {
    allocator: std.mem.Allocator = undefined,
    sql: string = "",
    params: []QueryParam = [],

    /// Releases `sql`, any owned `Text`/`Bytes` payloads inside `params`, and the `params` slice.
    pub fn deinit(*_) {
        if (_.sql.len > 0) {
            _.allocator.free(_.sql);
            _.sql = "";
        }
        for (p of _.params) {
            p.deinit(_.allocator);
        }
        if (_.params.len > 0) {
            _.allocator.free(_.params);
            _.params = [];
        }
    }
}

fn validateIdentSegment(seg: string): bool {
    if (seg.len == 0) return false;
    let i: usize = 0;
    while (i < seg.len) : (i += 1) {
        const ch = seg[i];
        // Reject NUL, ASCII control characters, DEL, and statement terminator `;`
        if (ch < 0x20 || ch == 0x7F || ch == ';') {
            return false;
        }
        // Reject SQL comment markers `--`, `/*`, `*/`
        if (ch == '-' && i + 1 < seg.len && seg[i + 1] == '-') {
            return false;
        }
        if (ch == '/' && i + 1 < seg.len && seg[i + 1] == '*') {
            return false;
        }
        if (ch == '*' && i + 1 < seg.len && seg[i + 1] == '/') {
            return false;
        }
    }
    return true;
}

/// Injection-safe SQL identifier quoter supporting dot-qualified names (`schema.table.col`).
pub static struct IdentCodec {
    /// Appends a quoted identifier (or dot-separated segments) into `out`.
    /// Rejects empty identifiers, empty segments, `\0`/control bytes, `;`, and SQL comment markers.
    /// Escapes embedded dialect quote characters by doubling (`""` or `` `` ``).
    pub static fn appendQuoted(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        dialect: QueryDialect,
        ident: string,
    ): bool ! QueryError {
        if (ident.len == 0) {
            return QueryError.InvalidIdentifier;
        }
        const q = dialect.quoteChar();

        let seg_start: usize = 0;
        let i: usize = 0;
        while (i <= ident.len) : (i += 1) {
            if (i == ident.len || ident[i] == '.') {
                const seg = ident[seg_start..i];
                if (!validateIdentSegment(seg)) {
                    return QueryError.InvalidIdentifier;
                }
                if (seg_start > 0) {
                    out.append(alloc, '.') catch return QueryError.OutOfMemory;
                }
                out.append(alloc, q) catch return QueryError.OutOfMemory;
                for (ch of seg) {
                    if (ch == q) {
                        out.append(alloc, q) catch return QueryError.OutOfMemory;
                    }
                    out.append(alloc, ch) catch return QueryError.OutOfMemory;
                }
                out.append(alloc, q) catch return QueryError.OutOfMemory;
                seg_start = i + 1;
            }
        }
        return true;
    }

    /// Quotes `ident` (supporting `table.col` dot-separated parts) for `dialect`.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub static fn quote(
        alloc: std.mem.Allocator,
        dialect: QueryDialect,
        ident: string,
    ): ([]u8) ! QueryError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        let * = IdentCodec.appendQuoted(&out, alloc, dialect, ident)?;
        return out.toOwnedSlice(alloc) catch return QueryError.OutOfMemory;
    }
}
