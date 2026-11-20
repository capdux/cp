// Source: cp/lib/query/builder.cp (Fluent Parameterized SELECT / INSERT / UPDATE / DELETE Builders)
// Pure CP fluent SQL query builders emitting parameterized SQL with PostgreSQL ($1..$N),
// MySQL (?, `ident`), and SQLite (?, "ident") dialects while preventing SQL injection.

const std = @import("std");
import {
    QueryError,
    QueryDialect,
    QueryParam,
    BuiltQuery,
    IdentCodec,
} from "./types.cp";

enum PendingErr: u8 {
    None = 0,
    InvalidIdentifier = 1,
    EmptyTable = 2,
    EmptyValues = 3,
    OutOfMemory = 4,

    pub fn isSet(): bool {
        return _ != .None;
    }

    pub fn toError(): QueryError? {
        return switch (_) {
            .None => null,
            .InvalidIdentifier => QueryError.InvalidIdentifier,
            .EmptyTable => QueryError.EmptyTable,
            .EmptyValues => QueryError.EmptyValues,
            .OutOfMemory => QueryError.OutOfMemory,
        };
    }
}

enum JoinKind: u8 {
    Inner = 0,
    Left = 1,
}

struct JoinClause {
    kind: JoinKind = .Inner,
    table: string = "",
    on_left: string = "",
    on_right: string = "",
}

enum WhereJoiner: u8 {
    And = 0,
    Or = 1,
}

enum WhereKind: u8 {
    Op = 0,
    In = 1,
    IsNull = 2,
    IsNotNull = 3,
}

struct WhereClause {
    joiner: WhereJoiner = .And,
    kind: WhereKind = .Op,
    col: string = "",
    op: string = "=",
    val: QueryParam = .Null,
    vals: []QueryParam = [],
}

struct OrderItem {
    col: string = "",
    desc: bool = false,
}

struct SetItem {
    col: string = "",
    val: QueryParam = .Null,
}

fn trimAscii(s: string): string {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn isValidOperator(op_raw: string): bool {
    const op = trimAscii(op_raw);
    if (op.len == 0) return false;
    if (std.mem.eql(u8, op, "=")) return true;
    if (std.mem.eql(u8, op, "!=")) return true;
    if (std.mem.eql(u8, op, "<>")) return true;
    if (std.mem.eql(u8, op, "<")) return true;
    if (std.mem.eql(u8, op, "<=")) return true;
    if (std.mem.eql(u8, op, ">")) return true;
    if (std.mem.eql(u8, op, ">=")) return true;
    if (std.mem.eql(u8, op, "~")) return true;
    if (std.mem.eql(u8, op, "~*")) return true;
    if (std.mem.eql(u8, op, "!~")) return true;
    if (std.mem.eql(u8, op, "!~*")) return true;
    if (std.ascii.eqlIgnoreCase(op, "LIKE")) return true;
    if (std.ascii.eqlIgnoreCase(op, "NOT LIKE")) return true;
    if (std.ascii.eqlIgnoreCase(op, "ILIKE")) return true;
    if (std.ascii.eqlIgnoreCase(op, "NOT ILIKE")) return true;
    return false;
}

fn dupOwnedString(alloc: std.mem.Allocator, s: string): string ! QueryError {
    if (s.len == 0) return "";
    return alloc.dupe(u8, s) catch return QueryError.OutOfMemory;
}

fn initTableField(alloc: std.mem.Allocator, table: string, pending_err: *PendingErr): string {
    if (table.len == 0) {
        pending_err.* = .EmptyTable;
        return "";
    }
    return dupOwnedString(alloc, table) catch {
        pending_err.* = .OutOfMemory;
        return "";
    };
}

fn freeOwnedString(alloc: std.mem.Allocator, s: string) {
    if (s.len > 0) alloc.free(s);
}

fn freeStringList(alloc: std.mem.Allocator, list: *std.ArrayList(string)) {
    for (s of list.items) {
        freeOwnedString(alloc, s);
    }
    list.deinit(alloc);
}

fn freeWhereList(alloc: std.mem.Allocator, list: *std.ArrayList(WhereClause)) {
    for (w of list.items) {
        freeOwnedString(alloc, w.col);
        freeOwnedString(alloc, w.op);
        w.val.deinit(alloc);
        for (v of w.vals) {
            v.deinit(alloc);
        }
        if (w.vals.len > 0) alloc.free(w.vals);
    }
    list.deinit(alloc);
}

fn freeParamsList(alloc: std.mem.Allocator, params: *std.ArrayList(QueryParam)) {
    for (p of params.items) {
        p.deinit(alloc);
    }
    params.deinit(alloc);
}

fn appendPlaceholder(
    sql: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    dialect: QueryDialect,
    param_index_1based: usize,
): bool ! QueryError {
    if (dialect.usesNumberedPlaceholders()) {
        let buf: [32]u8 = undefined;
        const rendered = std.fmt.bufPrint(&buf, "${d}", .{param_index_1based}) catch return QueryError.OutOfMemory;
        sql.appendSlice(alloc, rendered) catch return QueryError.OutOfMemory;
    } else {
        sql.append(alloc, '?') catch return QueryError.OutOfMemory;
    }
    return true;
}

fn appendU64Decimal(
    sql: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    n: u64,
): bool ! QueryError {
    let buf: [32]u8 = undefined;
    const rendered = std.fmt.bufPrint(&buf, "{d}", .{n}) catch return QueryError.OutOfMemory;
    sql.appendSlice(alloc, rendered) catch return QueryError.OutOfMemory;
    return true;
}

fn appendSelectColumn(
    sql: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    dialect: QueryDialect,
    col: string,
): bool ! QueryError {
    if (std.mem.eql(u8, col, "*")) {
        sql.append(alloc, '*') catch return QueryError.OutOfMemory;
        return true;
    }
    if (col.len > 2 && std.mem.endsWith(u8, col, ".*")) {
        const prefix = col[0 .. col.len - 2];
        let * = IdentCodec.appendQuoted(sql, alloc, dialect, prefix)?;
        sql.appendSlice(alloc, ".*") catch return QueryError.OutOfMemory;
        return true;
    }
    let * = IdentCodec.appendQuoted(sql, alloc, dialect, col)?;
    return true;
}

fn appendReturningClause(
    sql: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    dialect: QueryDialect,
    cols: []const string,
): bool ! QueryError {
    if (cols.len == 0) return true;
    sql.appendSlice(alloc, " RETURNING ") catch return QueryError.OutOfMemory;
    let i: usize = 0;
    while (i < cols.len) : (i += 1) {
        if (i > 0) {
            sql.appendSlice(alloc, ", ") catch return QueryError.OutOfMemory;
        }
        let * = appendSelectColumn(sql, alloc, dialect, cols[i])?;
    }
    return true;
}

fn appendWhereClauses(
    sql: *std.ArrayList(u8),
    params: *std.ArrayList(QueryParam),
    alloc: std.mem.Allocator,
    dialect: QueryDialect,
    wheres: []const WhereClause,
): bool ! QueryError {
    if (wheres.len == 0) return true;
    sql.appendSlice(alloc, " WHERE ") catch return QueryError.OutOfMemory;
    let i: usize = 0;
    while (i < wheres.len) : (i += 1) {
        const w = wheres[i];
        if (i > 0) {
            switch (w.joiner) {
                .And => {
                    sql.appendSlice(alloc, " AND ") catch return QueryError.OutOfMemory;
                },
                .Or => {
                    sql.appendSlice(alloc, " OR ") catch return QueryError.OutOfMemory;
                },
            }
        }

        let * = IdentCodec.appendQuoted(sql, alloc, dialect, w.col)?;
        switch (w.kind) {
            .Op => {
                if (!isValidOperator(w.op)) {
                    return QueryError.InvalidIdentifier;
                }
                const op = trimAscii(w.op);
                sql.append(alloc, ' ') catch return QueryError.OutOfMemory;
                sql.appendSlice(alloc, op) catch return QueryError.OutOfMemory;
                sql.append(alloc, ' ') catch return QueryError.OutOfMemory;

                const cloned = w.val.clone(alloc)?;
                errdefer cloned.deinit(alloc);
                params.append(alloc, cloned) catch return QueryError.OutOfMemory;
                let * = appendPlaceholder(sql, alloc, dialect, params.items.len)?;
            },
            .In => {
                if (w.vals.len == 0) {
                    return QueryError.EmptyValues;
                }
                sql.appendSlice(alloc, " IN (") catch return QueryError.OutOfMemory;
                let j: usize = 0;
                while (j < w.vals.len) : (j += 1) {
                    if (j > 0) {
                        sql.appendSlice(alloc, ", ") catch return QueryError.OutOfMemory;
                    }
                    const cloned = w.vals[j].clone(alloc)?;
                    errdefer cloned.deinit(alloc);
                    params.append(alloc, cloned) catch return QueryError.OutOfMemory;
                    let * = appendPlaceholder(sql, alloc, dialect, params.items.len)?;
                }
                sql.append(alloc, ')') catch return QueryError.OutOfMemory;
            },
            .IsNull => {
                sql.appendSlice(alloc, " IS NULL") catch return QueryError.OutOfMemory;
            },
            .IsNotNull => {
                sql.appendSlice(alloc, " IS NOT NULL") catch return QueryError.OutOfMemory;
            },
        }
    }
    return true;
}

fn pushWhereOp(
    alloc: std.mem.Allocator,
    wheres: *std.ArrayList(WhereClause),
    pending_err: *PendingErr,
    joiner: WhereJoiner,
    col: string,
    op: string,
    val: QueryParam,
) {
    if (pending_err.isSet()) return;
    if (!isValidOperator(op)) {
        pending_err.* = .InvalidIdentifier;
        return;
    }
    const col_copy = dupOwnedString(alloc, col) catch {
        pending_err.* = .OutOfMemory;
        return;
    };
    const op_copy = dupOwnedString(alloc, trimAscii(op)) catch {
        freeOwnedString(alloc, col_copy);
        pending_err.* = .OutOfMemory;
        return;
    };
    const val_copy = val.clone(alloc) catch {
        freeOwnedString(alloc, col_copy);
        freeOwnedString(alloc, op_copy);
        pending_err.* = .OutOfMemory;
        return;
    };
    wheres.append(alloc, WhereClause{
        joiner: joiner,
        kind: .Op,
        col: col_copy,
        op: op_copy,
        val: val_copy,
        vals: [],
    }) catch {
        freeOwnedString(alloc, col_copy);
        freeOwnedString(alloc, op_copy);
        val_copy.deinit(alloc);
        pending_err.* = .OutOfMemory;
    };
}

fn pushWhereIn(
    alloc: std.mem.Allocator,
    wheres: *std.ArrayList(WhereClause),
    pending_err: *PendingErr,
    col: string,
    vals: []const QueryParam,
) {
    if (pending_err.isSet()) return;
    if (vals.len == 0) {
        pending_err.* = .EmptyValues;
        return;
    }
    const col_copy = dupOwnedString(alloc, col) catch {
        pending_err.* = .OutOfMemory;
        return;
    };
    let copied = alloc.alloc(QueryParam, vals.len) catch {
        freeOwnedString(alloc, col_copy);
        pending_err.* = .OutOfMemory;
        return;
    };
    let count: usize = 0;
    while (count < vals.len) : (count += 1) {
        copied[count] = vals[count].clone(alloc) catch {
            let k: usize = 0;
            while (k < count) : (k += 1) {
                copied[k].deinit(alloc);
            }
            alloc.free(copied);
            freeOwnedString(alloc, col_copy);
            pending_err.* = .OutOfMemory;
            return;
        };
    }
    wheres.append(alloc, WhereClause{
        joiner: .And,
        kind: .In,
        col: col_copy,
        op: "",
        val: .Null,
        vals: copied,
    }) catch {
        for (item of copied) {
            item.deinit(alloc);
        }
        alloc.free(copied);
        freeOwnedString(alloc, col_copy);
        pending_err.* = .OutOfMemory;
    };
}

fn pushWhereNullness(
    alloc: std.mem.Allocator,
    wheres: *std.ArrayList(WhereClause),
    pending_err: *PendingErr,
    col: string,
    is_not_null: bool,
) {
    if (pending_err.isSet()) return;
    const col_copy = dupOwnedString(alloc, col) catch {
        pending_err.* = .OutOfMemory;
        return;
    };
    wheres.append(alloc, WhereClause{
        joiner: .And,
        kind: if (is_not_null) .IsNotNull else .IsNull,
        col: col_copy,
        op: "",
        val: .Null,
        vals: [],
    }) catch {
        freeOwnedString(alloc, col_copy);
        pending_err.* = .OutOfMemory;
    };
}

fn appendColumnNamesToList(
    alloc: std.mem.Allocator,
    dest: *std.ArrayList(string),
    pending_err: *PendingErr,
    cols: []const string,
) {
    if (pending_err.isSet()) return;
    for (c of cols) {
        const duped = dupOwnedString(alloc, c) catch {
            pending_err.* = .OutOfMemory;
            return;
        };
        dest.append(alloc, duped) catch {
            freeOwnedString(alloc, duped);
            pending_err.* = .OutOfMemory;
            return;
        };
    }
}

/// Fluent parameterized `SELECT` query builder.
pub struct SelectBuilder {
    allocator: std.mem.Allocator = undefined,
    dialect: QueryDialect = .Postgres,
    table: string = "",
    is_distinct: bool = false,
    cols: std.ArrayList(string) = .empty,
    joins: std.ArrayList(JoinClause) = .empty,
    wheres: std.ArrayList(WhereClause) = .empty,
    groups: std.ArrayList(string) = .empty,
    orders: std.ArrayList(OrderItem) = .empty,
    limit_val: u64? = null,
    offset_val: u64? = null,
    pending_err: PendingErr = .None,

    /// Initializes a `SELECT` query builder targeting `table` under `dialect`.
    pub static fn init(alloc: std.mem.Allocator, dialect: QueryDialect, table: string): SelectBuilder {
        let b = SelectBuilder{
            allocator: alloc,
            dialect: dialect,
            table: "",
            is_distinct: false,
            cols: .empty,
            joins: .empty,
            wheres: .empty,
            groups: .empty,
            orders: .empty,
            limit_val: null,
            offset_val: null,
            pending_err: .None,
        };
        b.table = initTableField(alloc, table, &b.pending_err);
        return b;
    }

    /// Releases all owned buffers stored inside this builder.
    pub fn deinit(*_) {
        freeOwnedString(_.allocator, _.table);
        _.table = "";
        freeStringList(_.allocator, &_.cols);
        for (j of _.joins.items) {
            freeOwnedString(_.allocator, j.table);
            freeOwnedString(_.allocator, j.on_left);
            freeOwnedString(_.allocator, j.on_right);
        }
        _.joins.deinit(_.allocator);
        freeWhereList(_.allocator, &_.wheres);
        freeStringList(_.allocator, &_.groups);
        for (o of _.orders.items) {
            freeOwnedString(_.allocator, o.col);
        }
        _.orders.deinit(_.allocator);
    }

    /// Adds projection columns (defaults to `*` when omitted).
    pub fn columns(*_, cols: []const string): *SelectBuilder {
        appendColumnNamesToList(_.allocator, &_.cols, &_.pending_err, cols);
        return _;
    }

    /// Enables `SELECT DISTINCT`.
    pub fn distinct(*_): *SelectBuilder {
        _.is_distinct = true;
        return _;
    }

    fn addJoin(*_, kind: JoinKind, table: string, on_left: string, on_right: string): *SelectBuilder {
        if (_.pending_err.isSet()) return _;
        if (table.len == 0) {
            _.pending_err = .EmptyTable;
            return _;
        }
        const t_copy = dupOwnedString(_.allocator, table) catch {
            _.pending_err = .OutOfMemory;
            return _;
        };
        const l_copy = dupOwnedString(_.allocator, on_left) catch {
            freeOwnedString(_.allocator, t_copy);
            _.pending_err = .OutOfMemory;
            return _;
        };
        const r_copy = dupOwnedString(_.allocator, on_right) catch {
            freeOwnedString(_.allocator, t_copy);
            freeOwnedString(_.allocator, l_copy);
            _.pending_err = .OutOfMemory;
            return _;
        };
        _.joins.append(_.allocator, JoinClause{
            kind: kind,
            table: t_copy,
            on_left: l_copy,
            on_right: r_copy,
        }) catch {
            freeOwnedString(_.allocator, t_copy);
            freeOwnedString(_.allocator, l_copy);
            freeOwnedString(_.allocator, r_copy);
            _.pending_err = .OutOfMemory;
        };
        return _;
    }

    /// Appends an `INNER JOIN <table> ON <on_left> = <on_right>` clause.
    pub fn join(*_, table: string, on_left: string, on_right: string): *SelectBuilder {
        return _.addJoin(.Inner, table, on_left, on_right);
    }

    /// Appends a `LEFT JOIN <table> ON <on_left> = <on_right>` clause.
    pub fn leftJoin(*_, table: string, on_left: string, on_right: string): *SelectBuilder {
        return _.addJoin(.Left, table, on_left, on_right);
    }

    /// Appends `AND <col> = <param>`.
    pub fn whereEq(*_, col: string, val: QueryParam): *SelectBuilder {
        pushWhereOp(_.allocator, &_.wheres, &_.pending_err, .And, col, "=", val);
        return _;
    }

    /// Appends `AND <col> <op> <param>` using a validated SQL comparison operator.
    pub fn whereOp(*_, col: string, op: string, val: QueryParam): *SelectBuilder {
        pushWhereOp(_.allocator, &_.wheres, &_.pending_err, .And, col, op, val);
        return _;
    }

    /// Appends `AND <col> IN (<param1>, <param2>, ...)`.
    pub fn whereIn(*_, col: string, vals: []const QueryParam): *SelectBuilder {
        pushWhereIn(_.allocator, &_.wheres, &_.pending_err, col, vals);
        return _;
    }

    /// Appends `AND <col> IS NULL`.
    pub fn whereNull(*_, col: string): *SelectBuilder {
        pushWhereNullness(_.allocator, &_.wheres, &_.pending_err, col, false);
        return _;
    }

    /// Appends `AND <col> IS NOT NULL`.
    pub fn whereNotNull(*_, col: string): *SelectBuilder {
        pushWhereNullness(_.allocator, &_.wheres, &_.pending_err, col, true);
        return _;
    }

    /// Appends `OR <col> = <param>`.
    pub fn orWhereEq(*_, col: string, val: QueryParam): *SelectBuilder {
        pushWhereOp(_.allocator, &_.wheres, &_.pending_err, .Or, col, "=", val);
        return _;
    }

    /// Appends a column to the `GROUP BY` clause.
    pub fn groupBy(*_, col: string): *SelectBuilder {
        if (_.pending_err.isSet()) return _;
        const duped = dupOwnedString(_.allocator, col) catch {
            _.pending_err = .OutOfMemory;
            return _;
        };
        _.groups.append(_.allocator, duped) catch {
            freeOwnedString(_.allocator, duped);
            _.pending_err = .OutOfMemory;
        };
        return _;
    }

    fn addOrder(*_, col: string, desc: bool): *SelectBuilder {
        if (_.pending_err.isSet()) return _;
        const duped = dupOwnedString(_.allocator, col) catch {
            _.pending_err = .OutOfMemory;
            return _;
        };
        _.orders.append(_.allocator, OrderItem{ col: duped, desc: desc }) catch {
            freeOwnedString(_.allocator, duped);
            _.pending_err = .OutOfMemory;
        };
        return _;
    }

    /// Appends `<col> ASC` to the `ORDER BY` clause.
    pub fn orderByAsc(*_, col: string): *SelectBuilder {
        return _.addOrder(col, false);
    }

    /// Appends `<col> DESC` to the `ORDER BY` clause.
    pub fn orderByDesc(*_, col: string): *SelectBuilder {
        return _.addOrder(col, true);
    }

    /// Sets the `LIMIT` row count.
    pub fn limit(*_, n: u64): *SelectBuilder {
        _.limit_val = n;
        return _;
    }

    /// Sets the `OFFSET` row count.
    pub fn offset(*_, n: u64): *SelectBuilder {
        _.offset_val = n;
        return _;
    }

    /// Renders the parameterized `SELECT` query and collects all bound parameters in order.
    pub fn build(*_): BuiltQuery ! QueryError {
        if (err_val := _.pending_err.toError()) {
            return err_val;
        }
        if (_.table.len == 0) {
            return QueryError.EmptyTable;
        }

        let sql: std.ArrayList(u8) = .empty;
        errdefer sql.deinit(_.allocator);
        let params: std.ArrayList(QueryParam) = .empty;
        errdefer freeParamsList(_.allocator, &params);

        sql.appendSlice(_.allocator, if (_.is_distinct) "SELECT DISTINCT " else "SELECT ") catch return QueryError.OutOfMemory;

        if (_.cols.items.len == 0) {
            sql.append(_.allocator, '*') catch return QueryError.OutOfMemory;
        } else {
            let i: usize = 0;
            while (i < _.cols.items.len) : (i += 1) {
                if (i > 0) {
                    sql.appendSlice(_.allocator, ", ") catch return QueryError.OutOfMemory;
                }
                let * = appendSelectColumn(&sql, _.allocator, _.dialect, _.cols.items[i])?;
            }
        }

        sql.appendSlice(_.allocator, " FROM ") catch return QueryError.OutOfMemory;
        let * = IdentCodec.appendQuoted(&sql, _.allocator, _.dialect, _.table)?;

        for (j of _.joins.items) {
            sql.appendSlice(_.allocator, if (j.kind == .Inner) " INNER JOIN " else " LEFT JOIN ") catch return QueryError.OutOfMemory;
            let * = IdentCodec.appendQuoted(&sql, _.allocator, _.dialect, j.table)?;
            sql.appendSlice(_.allocator, " ON ") catch return QueryError.OutOfMemory;
            let * = IdentCodec.appendQuoted(&sql, _.allocator, _.dialect, j.on_left)?;
            sql.appendSlice(_.allocator, " = ") catch return QueryError.OutOfMemory;
            let * = IdentCodec.appendQuoted(&sql, _.allocator, _.dialect, j.on_right)?;
        }

        let * = appendWhereClauses(&sql, &params, _.allocator, _.dialect, _.wheres.items)?;

        if (_.groups.items.len > 0) {
            sql.appendSlice(_.allocator, " GROUP BY ") catch return QueryError.OutOfMemory;
            let i: usize = 0;
            while (i < _.groups.items.len) : (i += 1) {
                if (i > 0) {
                    sql.appendSlice(_.allocator, ", ") catch return QueryError.OutOfMemory;
                }
                let * = IdentCodec.appendQuoted(&sql, _.allocator, _.dialect, _.groups.items[i])?;
            }
        }

        if (_.orders.items.len > 0) {
            sql.appendSlice(_.allocator, " ORDER BY ") catch return QueryError.OutOfMemory;
            let i: usize = 0;
            while (i < _.orders.items.len) : (i += 1) {
                if (i > 0) {
                    sql.appendSlice(_.allocator, ", ") catch return QueryError.OutOfMemory;
                }
                let * = IdentCodec.appendQuoted(&sql, _.allocator, _.dialect, _.orders.items[i].col)?;
                sql.appendSlice(_.allocator, if (_.orders.items[i].desc) " DESC" else " ASC") catch return QueryError.OutOfMemory;
            }
        }

        if (lim := _.limit_val) {
            sql.appendSlice(_.allocator, " LIMIT ") catch return QueryError.OutOfMemory;
            let * = appendU64Decimal(&sql, _.allocator, lim)?;
        }

        if (off := _.offset_val) {
            sql.appendSlice(_.allocator, " OFFSET ") catch return QueryError.OutOfMemory;
            let * = appendU64Decimal(&sql, _.allocator, off)?;
        }

        const owned_sql = sql.toOwnedSlice(_.allocator) catch return QueryError.OutOfMemory;
        errdefer _.allocator.free(owned_sql);
        const owned_params = params.toOwnedSlice(_.allocator) catch return QueryError.OutOfMemory;

        return BuiltQuery{
            allocator: _.allocator,
            sql: owned_sql,
            params: owned_params,
        };
    }
}

/// Fluent parameterized `INSERT` query builder supporting single and multi-row batch inserts.
pub struct InsertBuilder {
    allocator: std.mem.Allocator = undefined,
    dialect: QueryDialect = .Postgres,
    table: string = "",
    cols: std.ArrayList(string) = .empty,
    rows: std.ArrayList([]QueryParam) = .empty,
    ret_cols: std.ArrayList(string) = .empty,
    pending_err: PendingErr = .None,

    /// Initializes an `INSERT` query builder targeting `table` under `dialect`.
    pub static fn init(alloc: std.mem.Allocator, dialect: QueryDialect, table: string): InsertBuilder {
        let b = InsertBuilder{
            allocator: alloc,
            dialect: dialect,
            table: "",
            cols: .empty,
            rows: .empty,
            ret_cols: .empty,
            pending_err: .None,
        };
        b.table = initTableField(alloc, table, &b.pending_err);
        return b;
    }

    /// Releases all owned column names, row parameter slices, and buffers.
    pub fn deinit(*_) {
        freeOwnedString(_.allocator, _.table);
        _.table = "";
        freeStringList(_.allocator, &_.cols);
        for (row of _.rows.items) {
            for (p of row) {
                p.deinit(_.allocator);
            }
            if (row.len > 0) _.allocator.free(row);
        }
        _.rows.deinit(_.allocator);
        freeStringList(_.allocator, &_.ret_cols);
    }

    /// Specifies target column names for `INSERT INTO <table> (<cols>)`.
    pub fn columns(*_, cols: []const string): *InsertBuilder {
        appendColumnNamesToList(_.allocator, &_.cols, &_.pending_err, cols);
        return _;
    }

    /// Appends one row of parameter values to `VALUES (...), (...)`.
    pub fn addRow(*_, vals: []const QueryParam): *InsertBuilder {
        if (_.pending_err.isSet()) return _;
        if (vals.len == 0) {
            _.pending_err = .EmptyValues;
            return _;
        }
        let copied = _.allocator.alloc(QueryParam, vals.len) catch {
            _.pending_err = .OutOfMemory;
            return _;
        };
        let count: usize = 0;
        while (count < vals.len) : (count += 1) {
            copied[count] = vals[count].clone(_.allocator) catch {
                let k: usize = 0;
                while (k < count) : (k += 1) {
                    copied[k].deinit(_.allocator);
                }
                _.allocator.free(copied);
                _.pending_err = .OutOfMemory;
                return _;
            };
        }
        _.rows.append(_.allocator, copied) catch {
            for (item of copied) {
                item.deinit(_.allocator);
            }
            _.allocator.free(copied);
            _.pending_err = .OutOfMemory;
        };
        return _;
    }

    /// Appends `RETURNING <cols>` (PostgreSQL / SQLite3).
    pub fn returning(*_, cols: []const string): *InsertBuilder {
        appendColumnNamesToList(_.allocator, &_.ret_cols, &_.pending_err, cols);
        return _;
    }

    /// Renders the parameterized `INSERT` query and collects all bound parameters in row-major order.
    pub fn build(*_): BuiltQuery ! QueryError {
        if (err_val := _.pending_err.toError()) {
            return err_val;
        }
        if (_.table.len == 0) {
            return QueryError.EmptyTable;
        }
        if (_.rows.items.len == 0) {
            return QueryError.EmptyValues;
        }

        const expected_len = if (_.cols.items.len > 0) _.cols.items.len else _.rows.items[0].len;
        if (expected_len == 0) {
            return QueryError.EmptyValues;
        }
        for (row of _.rows.items) {
            if (row.len != expected_len) {
                return QueryError.EmptyValues;
            }
        }

        let sql: std.ArrayList(u8) = .empty;
        errdefer sql.deinit(_.allocator);
        let params: std.ArrayList(QueryParam) = .empty;
        errdefer freeParamsList(_.allocator, &params);

        sql.appendSlice(_.allocator, "INSERT INTO ") catch return QueryError.OutOfMemory;
        let * = IdentCodec.appendQuoted(&sql, _.allocator, _.dialect, _.table)?;

        if (_.cols.items.len > 0) {
            sql.appendSlice(_.allocator, " (") catch return QueryError.OutOfMemory;
            let i: usize = 0;
            while (i < _.cols.items.len) : (i += 1) {
                if (i > 0) {
                    sql.appendSlice(_.allocator, ", ") catch return QueryError.OutOfMemory;
                }
                let * = IdentCodec.appendQuoted(&sql, _.allocator, _.dialect, _.cols.items[i])?;
            }
            sql.append(_.allocator, ')') catch return QueryError.OutOfMemory;
        }

        sql.appendSlice(_.allocator, " VALUES ") catch return QueryError.OutOfMemory;
        let r_idx: usize = 0;
        while (r_idx < _.rows.items.len) : (r_idx += 1) {
            if (r_idx > 0) {
                sql.appendSlice(_.allocator, ", ") catch return QueryError.OutOfMemory;
            }
            sql.append(_.allocator, '(') catch return QueryError.OutOfMemory;
            const row = _.rows.items[r_idx];
            let c_idx: usize = 0;
            while (c_idx < row.len) : (c_idx += 1) {
                if (c_idx > 0) {
                    sql.appendSlice(_.allocator, ", ") catch return QueryError.OutOfMemory;
                }
                const cloned = row[c_idx].clone(_.allocator)?;
                errdefer cloned.deinit(_.allocator);
                params.append(_.allocator, cloned) catch return QueryError.OutOfMemory;
                let * = appendPlaceholder(&sql, _.allocator, _.dialect, params.items.len)?;
            }
            sql.append(_.allocator, ')') catch return QueryError.OutOfMemory;
        }

        let * = appendReturningClause(&sql, _.allocator, _.dialect, _.ret_cols.items)?;

        const owned_sql = sql.toOwnedSlice(_.allocator) catch return QueryError.OutOfMemory;
        errdefer _.allocator.free(owned_sql);
        const owned_params = params.toOwnedSlice(_.allocator) catch return QueryError.OutOfMemory;

        return BuiltQuery{
            allocator: _.allocator,
            sql: owned_sql,
            params: owned_params,
        };
    }
}

/// Fluent parameterized `UPDATE` query builder.
pub struct UpdateBuilder {
    allocator: std.mem.Allocator = undefined,
    dialect: QueryDialect = .Postgres,
    table: string = "",
    sets: std.ArrayList(SetItem) = .empty,
    wheres: std.ArrayList(WhereClause) = .empty,
    ret_cols: std.ArrayList(string) = .empty,
    pending_err: PendingErr = .None,

    /// Initializes an `UPDATE` query builder targeting `table` under `dialect`.
    pub static fn init(alloc: std.mem.Allocator, dialect: QueryDialect, table: string): UpdateBuilder {
        let b = UpdateBuilder{
            allocator: alloc,
            dialect: dialect,
            table: "",
            sets: .empty,
            wheres: .empty,
            ret_cols: .empty,
            pending_err: .None,
        };
        b.table = initTableField(alloc, table, &b.pending_err);
        return b;
    }

    /// Releases all owned buffers stored inside this builder.
    pub fn deinit(*_) {
        freeOwnedString(_.allocator, _.table);
        _.table = "";
        for (s of _.sets.items) {
            freeOwnedString(_.allocator, s.col);
            s.val.deinit(_.allocator);
        }
        _.sets.deinit(_.allocator);
        freeWhereList(_.allocator, &_.wheres);
        freeStringList(_.allocator, &_.ret_cols);
    }

    /// Adds a `<col> = <param>` assignment to the `SET` clause.
    pub fn set(*_, col: string, val: QueryParam): *UpdateBuilder {
        if (_.pending_err.isSet()) return _;
        const col_copy = dupOwnedString(_.allocator, col) catch {
            _.pending_err = .OutOfMemory;
            return _;
        };
        const val_copy = val.clone(_.allocator) catch {
            freeOwnedString(_.allocator, col_copy);
            _.pending_err = .OutOfMemory;
            return _;
        };
        _.sets.append(_.allocator, SetItem{ col: col_copy, val: val_copy }) catch {
            freeOwnedString(_.allocator, col_copy);
            val_copy.deinit(_.allocator);
            _.pending_err = .OutOfMemory;
        };
        return _;
    }

    /// Appends `AND <col> = <param>` to the `WHERE` clause.
    pub fn whereEq(*_, col: string, val: QueryParam): *UpdateBuilder {
        pushWhereOp(_.allocator, &_.wheres, &_.pending_err, .And, col, "=", val);
        return _;
    }

    /// Appends `AND <col> <op> <param>` to the `WHERE` clause.
    pub fn whereOp(*_, col: string, op: string, val: QueryParam): *UpdateBuilder {
        pushWhereOp(_.allocator, &_.wheres, &_.pending_err, .And, col, op, val);
        return _;
    }

    /// Appends `AND <col> IN (<vals...>)` to the `WHERE` clause.
    pub fn whereIn(*_, col: string, vals: []const QueryParam): *UpdateBuilder {
        pushWhereIn(_.allocator, &_.wheres, &_.pending_err, col, vals);
        return _;
    }

    /// Appends `AND <col> IS NULL` to the `WHERE` clause.
    pub fn whereNull(*_, col: string): *UpdateBuilder {
        pushWhereNullness(_.allocator, &_.wheres, &_.pending_err, col, false);
        return _;
    }

    /// Appends `AND <col> IS NOT NULL` to the `WHERE` clause.
    pub fn whereNotNull(*_, col: string): *UpdateBuilder {
        pushWhereNullness(_.allocator, &_.wheres, &_.pending_err, col, true);
        return _;
    }

    /// Appends `RETURNING <cols>` (PostgreSQL / SQLite3).
    pub fn returning(*_, cols: []const string): *UpdateBuilder {
        appendColumnNamesToList(_.allocator, &_.ret_cols, &_.pending_err, cols);
        return _;
    }

    /// Renders the parameterized `UPDATE` query and collects all bound parameters in order.
    pub fn build(*_): BuiltQuery ! QueryError {
        if (err_val := _.pending_err.toError()) {
            return err_val;
        }
        if (_.table.len == 0) {
            return QueryError.EmptyTable;
        }
        if (_.sets.items.len == 0) {
            return QueryError.EmptyValues;
        }

        let sql: std.ArrayList(u8) = .empty;
        errdefer sql.deinit(_.allocator);
        let params: std.ArrayList(QueryParam) = .empty;
        errdefer freeParamsList(_.allocator, &params);

        sql.appendSlice(_.allocator, "UPDATE ") catch return QueryError.OutOfMemory;
        let * = IdentCodec.appendQuoted(&sql, _.allocator, _.dialect, _.table)?;
        sql.appendSlice(_.allocator, " SET ") catch return QueryError.OutOfMemory;

        let i: usize = 0;
        while (i < _.sets.items.len) : (i += 1) {
            if (i > 0) {
                sql.appendSlice(_.allocator, ", ") catch return QueryError.OutOfMemory;
            }
            const s = _.sets.items[i];
            let * = IdentCodec.appendQuoted(&sql, _.allocator, _.dialect, s.col)?;
            sql.appendSlice(_.allocator, " = ") catch return QueryError.OutOfMemory;
            const cloned = s.val.clone(_.allocator)?;
            errdefer cloned.deinit(_.allocator);
            params.append(_.allocator, cloned) catch return QueryError.OutOfMemory;
            let * = appendPlaceholder(&sql, _.allocator, _.dialect, params.items.len)?;
        }

        let * = appendWhereClauses(&sql, &params, _.allocator, _.dialect, _.wheres.items)?;
        let * = appendReturningClause(&sql, _.allocator, _.dialect, _.ret_cols.items)?;

        const owned_sql = sql.toOwnedSlice(_.allocator) catch return QueryError.OutOfMemory;
        errdefer _.allocator.free(owned_sql);
        const owned_params = params.toOwnedSlice(_.allocator) catch return QueryError.OutOfMemory;

        return BuiltQuery{
            allocator: _.allocator,
            sql: owned_sql,
            params: owned_params,
        };
    }
}

/// Fluent parameterized `DELETE` query builder.
pub struct DeleteBuilder {
    allocator: std.mem.Allocator = undefined,
    dialect: QueryDialect = .Postgres,
    table: string = "",
    wheres: std.ArrayList(WhereClause) = .empty,
    ret_cols: std.ArrayList(string) = .empty,
    pending_err: PendingErr = .None,

    /// Initializes a `DELETE` query builder targeting `table` under `dialect`.
    pub static fn init(alloc: std.mem.Allocator, dialect: QueryDialect, table: string): DeleteBuilder {
        let b = DeleteBuilder{
            allocator: alloc,
            dialect: dialect,
            table: "",
            wheres: .empty,
            ret_cols: .empty,
            pending_err: .None,
        };
        b.table = initTableField(alloc, table, &b.pending_err);
        return b;
    }

    /// Releases all owned buffers stored inside this builder.
    pub fn deinit(*_) {
        freeOwnedString(_.allocator, _.table);
        _.table = "";
        freeWhereList(_.allocator, &_.wheres);
        freeStringList(_.allocator, &_.ret_cols);
    }

    /// Appends `AND <col> = <param>` to the `WHERE` clause.
    pub fn whereEq(*_, col: string, val: QueryParam): *DeleteBuilder {
        pushWhereOp(_.allocator, &_.wheres, &_.pending_err, .And, col, "=", val);
        return _;
    }

    /// Appends `AND <col> <op> <param>` to the `WHERE` clause.
    pub fn whereOp(*_, col: string, op: string, val: QueryParam): *DeleteBuilder {
        pushWhereOp(_.allocator, &_.wheres, &_.pending_err, .And, col, op, val);
        return _;
    }

    /// Appends `AND <col> IN (<vals...>)` to the `WHERE` clause.
    pub fn whereIn(*_, col: string, vals: []const QueryParam): *DeleteBuilder {
        pushWhereIn(_.allocator, &_.wheres, &_.pending_err, col, vals);
        return _;
    }

    /// Appends `AND <col> IS NULL` to the `WHERE` clause.
    pub fn whereNull(*_, col: string): *DeleteBuilder {
        pushWhereNullness(_.allocator, &_.wheres, &_.pending_err, col, false);
        return _;
    }

    /// Appends `AND <col> IS NOT NULL` to the `WHERE` clause.
    pub fn whereNotNull(*_, col: string): *DeleteBuilder {
        pushWhereNullness(_.allocator, &_.wheres, &_.pending_err, col, true);
        return _;
    }

    /// Appends `RETURNING <cols>` (PostgreSQL / SQLite3).
    pub fn returning(*_, cols: []const string): *DeleteBuilder {
        appendColumnNamesToList(_.allocator, &_.ret_cols, &_.pending_err, cols);
        return _;
    }

    /// Renders the parameterized `DELETE` query and collects all bound parameters in order.
    pub fn build(*_): BuiltQuery ! QueryError {
        if (err_val := _.pending_err.toError()) {
            return err_val;
        }
        if (_.table.len == 0) {
            return QueryError.EmptyTable;
        }

        let sql: std.ArrayList(u8) = .empty;
        errdefer sql.deinit(_.allocator);
        let params: std.ArrayList(QueryParam) = .empty;
        errdefer freeParamsList(_.allocator, &params);

        sql.appendSlice(_.allocator, "DELETE FROM ") catch return QueryError.OutOfMemory;
        let * = IdentCodec.appendQuoted(&sql, _.allocator, _.dialect, _.table)?;

        let * = appendWhereClauses(&sql, &params, _.allocator, _.dialect, _.wheres.items)?;
        let * = appendReturningClause(&sql, _.allocator, _.dialect, _.ret_cols.items)?;

        const owned_sql = sql.toOwnedSlice(_.allocator) catch return QueryError.OutOfMemory;
        errdefer _.allocator.free(owned_sql);
        const owned_params = params.toOwnedSlice(_.allocator) catch return QueryError.OutOfMemory;

        return BuiltQuery{
            allocator: _.allocator,
            sql: owned_sql,
            params: owned_params,
        };
    }
}
