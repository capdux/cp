// Source: cp/lib/sql_ast/parser.cp (Pure CP SQL Recursive Descent Parser)

const std = @import("std");
import { SqlError, SqlTokenKind, SqlToken, tokenizeSql, eqIgnoreCase } from "./lexer.cp";
import {
    SqlExprKind,
    SqlExpr,
    SelectItem,
    JoinClause,
    OrderByItem,
    SelectStmt,
    ValueRow,
    InsertStmt,
    AssignItem,
    UpdateStmt,
    DeleteStmt,
    ColumnDef,
    CreateTableStmt,
    SqlStmtKind,
    SqlStmt,
} from "./ast.cp";

struct Parser {
    alloc: std.mem.Allocator,
    tokens: []SqlToken,
    pos: usize,
    depth: usize,
    max_depth: usize,

    fn peek(): SqlToken {
        if (_.pos >= _.tokens.len) {
            return SqlToken{ .kind = SqlTokenKind.Eof, .lexeme = "", .line = 0, .col = 0 };
        }
        return _.tokens[_.pos];
    }

    fn advance(*_): SqlToken {
        const tok = _.peek();
        if (_.pos < _.tokens.len) {
            _.pos += 1;
        }
        return tok;
    }

    fn matchKw(*_, kw: string): bool {
        const tok = _.peek();
        if (tok.kind == SqlTokenKind.Keyword && eqIgnoreCase(tok.lexeme, kw)) {
            _.pos += 1;
            return true;
        }
        return false;
    }

    fn isKw(kw: string): bool {
        const tok = _.peek();
        return tok.kind == SqlTokenKind.Keyword && eqIgnoreCase(tok.lexeme, kw);
    }

    fn expectKw(*_, kw: string): !void {
        if (!_.matchKw(kw)) {
            return SqlError.UnexpectedToken;
        }
    }

    fn expectIdent(*_): !string {
        const tok = _.peek();
        if (tok.kind == SqlTokenKind.Identifier || tok.kind == SqlTokenKind.QuotedIdentifier) {
            _.pos += 1;
            return tok.lexeme;
        }
        return SqlError.UnexpectedToken;
    }

    fn allocExpr(expr: SqlExpr): !*SqlExpr {
        let ptr = _.alloc.create(SqlExpr) catch return SqlError.OutOfMemory;
        ptr.* = expr;
        return ptr;
    }

    fn makeLeaf(kind: SqlExprKind, text: string, qualifier: string?): !*SqlExpr {
        return _.allocExpr(SqlExpr{
            .kind = kind,
            .text = text,
            .qualifier = qualifier,
            .op = "",
            .negated = false,
            .left = null,
            .right = null,
            .args = &.{},
        });
    }

    fn makeUnary(op: string, negated: bool, operand: *SqlExpr): !*SqlExpr {
        return _.allocExpr(SqlExpr{
            .kind = SqlExprKind.UnaryOp,
            .text = "",
            .qualifier = null,
            .op = op,
            .negated = negated,
            .left = operand,
            .right = null,
            .args = &.{},
        }) catch {
            operand.deinit(_.alloc);
            return SqlError.OutOfMemory;
        };
    }

    fn makeBinary(op: string, negated: bool, left: *SqlExpr, right: *SqlExpr): !*SqlExpr {
        return _.allocExpr(SqlExpr{
            .kind = SqlExprKind.BinaryOp,
            .text = "",
            .qualifier = null,
            .op = op,
            .negated = negated,
            .left = left,
            .right = right,
            .args = &.{},
        }) catch {
            right.deinit(_.alloc);
            return SqlError.OutOfMemory;
        };
    }

    fn parseExpr(*_): !*SqlExpr {
        if (_.depth >= _.max_depth) {
            return SqlError.MaxDepthExceeded;
        }
        _.depth += 1;
        defer _.depth -= 1;
        return _.parseOr();
    }

    fn parseOr(*_): !*SqlExpr {
        let left = try _.parseAnd();
        errdefer left.deinit(_.alloc);

        while (_.matchKw("OR")) {
            const right = try _.parseAnd();
            left = try _.makeBinary("OR", false, left, right);
        }
        return left;
    }

    fn parseAnd(*_): !*SqlExpr {
        let left = try _.parseComparison();
        errdefer left.deinit(_.alloc);

        while (_.matchKw("AND")) {
            const right = try _.parseComparison();
            left = try _.makeBinary("AND", false, left, right);
        }
        return left;
    }

    fn parseComparison(*_): !*SqlExpr {
        let left = try _.parseAdditive();
        errdefer left.deinit(_.alloc);

        while (true) {
            const tok = _.peek();
            if (tok.kind == SqlTokenKind.Operator &&
                (std.mem.eql(u8, tok.lexeme, "=") ||
                 std.mem.eql(u8, tok.lexeme, "!=") ||
                 std.mem.eql(u8, tok.lexeme, "<>") ||
                 std.mem.eql(u8, tok.lexeme, "<") ||
                 std.mem.eql(u8, tok.lexeme, "<=") ||
                 std.mem.eql(u8, tok.lexeme, ">") ||
                 std.mem.eql(u8, tok.lexeme, ">="))) {
                * = _.advance();
                const right = try _.parseAdditive();
                left = try _.makeBinary(tok.lexeme, false, left, right);
                continue;
            }

            if (_.matchKw("IS")) {
                const neg = _.matchKw("NOT");
                try _.expectKw("NULL");
                left = try _.allocExpr(SqlExpr{
                    .kind = SqlExprKind.IsNull,
                    .text = "",
                    .qualifier = null,
                    .op = if (neg) "IS NOT NULL" else "IS NULL",
                    .negated = neg,
                    .left = left,
                    .right = null,
                    .args = &.{},
                });
                continue;
            }

            if (_.isKw("NOT")) {
                const saved_pos = _.pos;
                * = _.advance();
                if (_.matchKw("LIKE")) {
                    const right = try _.parseAdditive();
                    left = try _.makeBinary("LIKE", true, left, right);
                    continue;
                }
                if (_.matchKw("IN")) {
                    left = try _.parseInList(left, true);
                    continue;
                }
                _.pos = saved_pos;
                break;
            }

            if (_.matchKw("LIKE")) {
                const right = try _.parseAdditive();
                left = try _.makeBinary("LIKE", false, left, right);
                continue;
            }

            if (_.matchKw("IN")) {
                left = try _.parseInList(left, false);
                continue;
            }

            break;
        }
        return left;
    }

    fn parseInList(*_, left: *SqlExpr, negated: bool): !*SqlExpr {
        if (_.peek().kind != SqlTokenKind.LParen) {
            return SqlError.UnexpectedToken;
        }
        * = _.advance();
        let items = std.ArrayList(*SqlExpr).empty;
        errdefer {
            let k: usize = 0;
            while (k < items.items.len) {
                items.items[k].deinit(_.alloc);
                k += 1;
            }
            items.deinit(_.alloc);
        }

        if (_.peek().kind != SqlTokenKind.RParen) {
            while (true) {
                const elem = try _.parseExpr();
                items.append(_.alloc, elem) catch {
                    elem.deinit(_.alloc);
                    return SqlError.OutOfMemory;
                };
                if (_.peek().kind == SqlTokenKind.Comma) {
                    * = _.advance();
                } else {
                    break;
                }
            }
        }
        if (_.peek().kind != SqlTokenKind.RParen) {
            return SqlError.UnexpectedToken;
        }
        * = _.advance();

        const slice = items.toOwnedSlice(_.alloc) catch return SqlError.OutOfMemory;
        return _.allocExpr(SqlExpr{
            .kind = SqlExprKind.InList,
            .text = "",
            .qualifier = null,
            .op = "IN",
            .negated = negated,
            .left = left,
            .right = null,
            .args = slice,
        });
    }

    fn parseAdditive(*_): !*SqlExpr {
        let left = try _.parseMultiplicative();
        errdefer left.deinit(_.alloc);

        while (true) {
            const tok = _.peek();
            if (tok.kind == SqlTokenKind.Operator &&
                (std.mem.eql(u8, tok.lexeme, "+") ||
                 std.mem.eql(u8, tok.lexeme, "-") ||
                 std.mem.eql(u8, tok.lexeme, "||"))) {
                * = _.advance();
                const right = try _.parseMultiplicative();
                left = try _.makeBinary(tok.lexeme, false, left, right);
                continue;
            }
            break;
        }
        return left;
    }

    fn parseMultiplicative(*_): !*SqlExpr {
        let left = try _.parseUnary();
        errdefer left.deinit(_.alloc);

        while (true) {
            const tok = _.peek();
            if (tok.kind == SqlTokenKind.Star ||
                (tok.kind == SqlTokenKind.Operator &&
                 (std.mem.eql(u8, tok.lexeme, "/") || std.mem.eql(u8, tok.lexeme, "%")))) {
                * = _.advance();
                const right = try _.parseUnary();
                left = try _.makeBinary(tok.lexeme, false, left, right);
                continue;
            }
            break;
        }
        return left;
    }

    fn parseUnary(*_): !*SqlExpr {
        if (_.matchKw("NOT")) {
            const operand = try _.parseUnary();
            return _.makeUnary("NOT", true, operand);
        }
        const tok = _.peek();
        if (tok.kind == SqlTokenKind.Operator &&
            (std.mem.eql(u8, tok.lexeme, "-") || std.mem.eql(u8, tok.lexeme, "+"))) {
            * = _.advance();
            const operand = try _.parseUnary();
            return _.makeUnary(tok.lexeme, false, operand);
        }
        return _.parsePrimary();
    }

    fn parsePrimary(*_): !*SqlExpr {
        const tok = _.peek();

        if (tok.kind == SqlTokenKind.NumberLiteral || tok.kind == SqlTokenKind.StringLiteral) {
            * = _.advance();
            return _.makeLeaf(SqlExprKind.Literal, tok.lexeme, null);
        }

        if (tok.kind == SqlTokenKind.Keyword &&
            (eqIgnoreCase(tok.lexeme, "NULL") || eqIgnoreCase(tok.lexeme, "TRUE") || eqIgnoreCase(tok.lexeme, "FALSE"))) {
            * = _.advance();
            return _.makeLeaf(SqlExprKind.Literal, tok.lexeme, null);
        }

        if (tok.kind == SqlTokenKind.Placeholder) {
            * = _.advance();
            return _.makeLeaf(SqlExprKind.Placeholder, tok.lexeme, null);
        }

        if (tok.kind == SqlTokenKind.Star) {
            * = _.advance();
            return _.makeLeaf(SqlExprKind.ColumnRef, "*", null);
        }

        if (tok.kind == SqlTokenKind.LParen) {
            * = _.advance();
            const inner = try _.parseExpr();
            errdefer inner.deinit(_.alloc);
            if (_.peek().kind != SqlTokenKind.RParen) {
                return SqlError.UnexpectedToken;
            }
            * = _.advance();
            return _.allocExpr(SqlExpr{
                .kind = SqlExprKind.Paren,
                .text = "",
                .qualifier = null,
                .op = "",
                .negated = false,
                .left = inner,
                .right = null,
                .args = &.{},
            });
        }

        if (tok.kind == SqlTokenKind.Identifier || tok.kind == SqlTokenKind.QuotedIdentifier) {
            * = _.advance();
            if (_.peek().kind == SqlTokenKind.LParen) {
                * = _.advance();
                let args = std.ArrayList(*SqlExpr).empty;
                errdefer {
                    let k: usize = 0;
                    while (k < args.items.len) {
                        args.items[k].deinit(_.alloc);
                        k += 1;
                    }
                    args.deinit(_.alloc);
                }
                if (_.peek().kind != SqlTokenKind.RParen) {
                    while (true) {
                        const arg = try _.parseExpr();
                        args.append(_.alloc, arg) catch {
                            arg.deinit(_.alloc);
                            return SqlError.OutOfMemory;
                        };
                        if (_.peek().kind == SqlTokenKind.Comma) {
                            * = _.advance();
                        } else {
                            break;
                        }
                    }
                }
                if (_.peek().kind != SqlTokenKind.RParen) {
                    return SqlError.UnexpectedToken;
                }
                * = _.advance();
                const slice = args.toOwnedSlice(_.alloc) catch return SqlError.OutOfMemory;
                return _.allocExpr(SqlExpr{
                    .kind = SqlExprKind.FuncCall,
                    .text = tok.lexeme,
                    .qualifier = null,
                    .op = "",
                    .negated = false,
                    .left = null,
                    .right = null,
                    .args = slice,
                });
            }

            if (_.peek().kind == SqlTokenKind.Dot) {
                * = _.advance();
                const next_tok = _.peek();
                if (next_tok.kind == SqlTokenKind.Identifier ||
                    next_tok.kind == SqlTokenKind.QuotedIdentifier ||
                    next_tok.kind == SqlTokenKind.Star) {
                    * = _.advance();
                    return _.makeLeaf(SqlExprKind.ColumnRef, next_tok.lexeme, tok.lexeme);
                }
                return SqlError.UnexpectedToken;
            }

            return _.makeLeaf(SqlExprKind.ColumnRef, tok.lexeme, null);
        }

        return SqlError.UnexpectedToken;
    }

    fn parseSelect(*_): !SelectStmt {
        try _.expectKw("SELECT");
        const distinct = _.matchKw("DISTINCT");

        let cols = std.ArrayList(SelectItem).empty;
        errdefer {
            let k: usize = 0;
            while (k < cols.items.len) {
                cols.items[k].expr.deinit(_.alloc);
                k += 1;
            }
            cols.deinit(_.alloc);
        }

        while (true) {
            const expr = try _.parseExpr();
            let is_star = false;
            if (expr.kind == SqlExprKind.ColumnRef && std.mem.eql(u8, expr.text, "*")) {
                is_star = true;
            }
            let alias: string? = null;
            if (_.matchKw("AS")) {
                alias = try _.expectIdent();
            } else if (_.peek().kind == SqlTokenKind.Identifier || _.peek().kind == SqlTokenKind.QuotedIdentifier) {
                alias = _.advance().lexeme;
            }
            cols.append(_.alloc, SelectItem{
                .expr = expr,
                .alias = alias,
                .is_star = is_star,
            }) catch {
                expr.deinit(_.alloc);
                return SqlError.OutOfMemory;
            };
            if (_.peek().kind == SqlTokenKind.Comma) {
                * = _.advance();
            } else {
                break;
            }
        }

        let from_table: string? = null;
        let from_alias: string? = null;
        if (_.matchKw("FROM")) {
            from_table = try _.expectIdent();
            if (_.matchKw("AS")) {
                from_alias = try _.expectIdent();
            } else if (_.peek().kind == SqlTokenKind.Identifier || _.peek().kind == SqlTokenKind.QuotedIdentifier) {
                from_alias = _.advance().lexeme;
            }
        }

        let joins = std.ArrayList(JoinClause).empty;
        errdefer {
            let k: usize = 0;
            while (k < joins.items.len) {
                if (on_e := joins.items[k].on_expr) {
                    on_e.deinit(_.alloc);
                }
                k += 1;
            }
            joins.deinit(_.alloc);
        }

        while (true) {
            let join_type: string? = null;
            if (_.matchKw("INNER")) {
                try _.expectKw("JOIN");
                join_type = "INNER";
            } else if (_.matchKw("LEFT")) {
                * = _.matchKw("OUTER");
                try _.expectKw("JOIN");
                join_type = "LEFT";
            } else if (_.matchKw("RIGHT")) {
                * = _.matchKw("OUTER");
                try _.expectKw("JOIN");
                join_type = "RIGHT";
            } else if (_.matchKw("FULL")) {
                * = _.matchKw("OUTER");
                try _.expectKw("JOIN");
                join_type = "FULL";
            } else if (_.matchKw("CROSS")) {
                try _.expectKw("JOIN");
                join_type = "CROSS";
            } else if (_.matchKw("JOIN")) {
                join_type = "INNER";
            }

            if (jt := join_type) {
                const tbl = try _.expectIdent();
                let j_alias: string? = null;
                if (_.matchKw("AS")) {
                    j_alias = try _.expectIdent();
                } else if (_.peek().kind == SqlTokenKind.Identifier || _.peek().kind == SqlTokenKind.QuotedIdentifier) {
                    j_alias = _.advance().lexeme;
                }
                let on_expr: (*SqlExpr)? = null;
                if (_.matchKw("ON")) {
                    on_expr = try _.parseExpr();
                }
                joins.append(_.alloc, JoinClause{
                    .join_type = jt,
                    .table_name = tbl,
                    .alias = j_alias,
                    .on_expr = on_expr,
                }) catch {
                    if (oe := on_expr) {
                        oe.deinit(_.alloc);
                    }
                    return SqlError.OutOfMemory;
                };
            } else {
                break;
            }
        }

        let where_expr: (*SqlExpr)? = null;
        errdefer {
            if (w := where_expr) {
                w.deinit(_.alloc);
            }
        }
        if (_.matchKw("WHERE")) {
            where_expr = try _.parseExpr();
        }

        let group_by = std.ArrayList(*SqlExpr).empty;
        errdefer {
            let k: usize = 0;
            while (k < group_by.items.len) {
                group_by.items[k].deinit(_.alloc);
                k += 1;
            }
            group_by.deinit(_.alloc);
        }
        if (_.matchKw("GROUP")) {
            try _.expectKw("BY");
            while (true) {
                const gexpr = try _.parseExpr();
                group_by.append(_.alloc, gexpr) catch {
                    gexpr.deinit(_.alloc);
                    return SqlError.OutOfMemory;
                };
                if (_.peek().kind == SqlTokenKind.Comma) {
                    * = _.advance();
                } else {
                    break;
                }
            }
        }

        let order_by = std.ArrayList(OrderByItem).empty;
        errdefer {
            let k: usize = 0;
            while (k < order_by.items.len) {
                order_by.items[k].expr.deinit(_.alloc);
                k += 1;
            }
            order_by.deinit(_.alloc);
        }
        if (_.matchKw("ORDER")) {
            try _.expectKw("BY");
            while (true) {
                const oexpr = try _.parseExpr();
                let is_desc = false;
                if (_.matchKw("DESC")) {
                    is_desc = true;
                } else {
                    * = _.matchKw("ASC");
                }
                order_by.append(_.alloc, OrderByItem{
                    .expr = oexpr,
                    .desc = is_desc,
                }) catch {
                    oexpr.deinit(_.alloc);
                    return SqlError.OutOfMemory;
                };
                if (_.peek().kind == SqlTokenKind.Comma) {
                    * = _.advance();
                } else {
                    break;
                }
            }
        }

        let limit_expr: (*SqlExpr)? = null;
        errdefer {
            if (lim := limit_expr) {
                lim.deinit(_.alloc);
            }
        }
        if (_.matchKw("LIMIT")) {
            limit_expr = try _.parseExpr();
        }

        let offset_expr: (*SqlExpr)? = null;
        errdefer {
            if (off := offset_expr) {
                off.deinit(_.alloc);
            }
        }
        if (_.matchKw("OFFSET")) {
            offset_expr = try _.parseExpr();
        }

        return SelectStmt{
            .distinct = distinct,
            .columns = cols.toOwnedSlice(_.alloc) catch return SqlError.OutOfMemory,
            .from_table = from_table,
            .from_alias = from_alias,
            .joins = joins.toOwnedSlice(_.alloc) catch return SqlError.OutOfMemory,
            .where_expr = where_expr,
            .group_by = group_by.toOwnedSlice(_.alloc) catch return SqlError.OutOfMemory,
            .order_by = order_by.toOwnedSlice(_.alloc) catch return SqlError.OutOfMemory,
            .limit = limit_expr,
            .offset = offset_expr,
        };
    }

    fn parseInsert(*_): !InsertStmt {
        try _.expectKw("INSERT");
        try _.expectKw("INTO");
        const table_name = try _.expectIdent();

        let cols = std.ArrayList(string).empty;
        errdefer cols.deinit(_.alloc);

        if (_.peek().kind == SqlTokenKind.LParen) {
            * = _.advance();
            while (true) {
                const col = try _.expectIdent();
                cols.append(_.alloc, col) catch return SqlError.OutOfMemory;
                if (_.peek().kind == SqlTokenKind.Comma) {
                    * = _.advance();
                } else {
                    break;
                }
            }
            if (_.peek().kind != SqlTokenKind.RParen) {
                return SqlError.UnexpectedToken;
            }
            * = _.advance();
        }

        try _.expectKw("VALUES");

        let rows = std.ArrayList(ValueRow).empty;
        errdefer {
            let r: usize = 0;
            while (r < rows.items.len) {
                let c: usize = 0;
                while (c < rows.items[r].items.len) {
                    rows.items[r].items[c].deinit(_.alloc);
                    c += 1;
                }
                if (rows.items[r].items.len > 0) {
                    _.alloc.free(rows.items[r].items);
                }
                r += 1;
            }
            rows.deinit(_.alloc);
        }

        while (true) {
            if (_.peek().kind != SqlTokenKind.LParen) {
                return SqlError.UnexpectedToken;
            }
            * = _.advance();
            let row_items = std.ArrayList(*SqlExpr).empty;
            errdefer {
                let k: usize = 0;
                while (k < row_items.items.len) {
                    row_items.items[k].deinit(_.alloc);
                    k += 1;
                }
                row_items.deinit(_.alloc);
            }
            while (true) {
                const val = try _.parseExpr();
                row_items.append(_.alloc, val) catch {
                    val.deinit(_.alloc);
                    return SqlError.OutOfMemory;
                };
                if (_.peek().kind == SqlTokenKind.Comma) {
                    * = _.advance();
                } else {
                    break;
                }
            }
            if (_.peek().kind != SqlTokenKind.RParen) {
                return SqlError.UnexpectedToken;
            }
            * = _.advance();
            const owned_row = row_items.toOwnedSlice(_.alloc) catch return SqlError.OutOfMemory;
            rows.append(_.alloc, ValueRow{ .items = owned_row }) catch {
                let k: usize = 0;
                while (k < owned_row.len) {
                    owned_row[k].deinit(_.alloc);
                    k += 1;
                }
                _.alloc.free(owned_row);
                return SqlError.OutOfMemory;
            };
            if (_.peek().kind == SqlTokenKind.Comma) {
                * = _.advance();
            } else {
                break;
            }
        }

        return InsertStmt{
            .table_name = table_name,
            .columns = cols.toOwnedSlice(_.alloc) catch return SqlError.OutOfMemory,
            .rows = rows.toOwnedSlice(_.alloc) catch return SqlError.OutOfMemory,
        };
    }

    fn parseUpdate(*_): !UpdateStmt {
        try _.expectKw("UPDATE");
        const table_name = try _.expectIdent();
        try _.expectKw("SET");

        let assigns = std.ArrayList(AssignItem).empty;
        errdefer {
            let k: usize = 0;
            while (k < assigns.items.len) {
                assigns.items[k].value.deinit(_.alloc);
                k += 1;
            }
            assigns.deinit(_.alloc);
        }

        while (true) {
            const col = try _.expectIdent();
            const eq_tok = _.peek();
            if (eq_tok.kind != SqlTokenKind.Operator || !std.mem.eql(u8, eq_tok.lexeme, "=")) {
                return SqlError.UnexpectedToken;
            }
            * = _.advance();
            const val = try _.parseExpr();
            assigns.append(_.alloc, AssignItem{ .column = col, .value = val }) catch {
                val.deinit(_.alloc);
                return SqlError.OutOfMemory;
            };
            if (_.peek().kind == SqlTokenKind.Comma) {
                * = _.advance();
            } else {
                break;
            }
        }

        let where_expr: (*SqlExpr)? = null;
        if (_.matchKw("WHERE")) {
            where_expr = try _.parseExpr();
        }

        return UpdateStmt{
            .table_name = table_name,
            .assignments = assigns.toOwnedSlice(_.alloc) catch return SqlError.OutOfMemory,
            .where_expr = where_expr,
        };
    }

    fn parseDelete(*_): !DeleteStmt {
        try _.expectKw("DELETE");
        try _.expectKw("FROM");
        const table_name = try _.expectIdent();

        let where_expr: (*SqlExpr)? = null;
        if (_.matchKw("WHERE")) {
            where_expr = try _.parseExpr();
        }

        return DeleteStmt{
            .table_name = table_name,
            .where_expr = where_expr,
        };
    }

    fn parseCreateTable(*_): !CreateTableStmt {
        try _.expectKw("CREATE");
        try _.expectKw("TABLE");
        let if_not_exists = false;
        if (_.matchKw("IF")) {
            try _.expectKw("NOT");
            try _.expectKw("EXISTS");
            if_not_exists = true;
        }
        const table_name = try _.expectIdent();

        if (_.peek().kind != SqlTokenKind.LParen) {
            return SqlError.UnexpectedToken;
        }
        * = _.advance();

        let cols = std.ArrayList(ColumnDef).empty;
        errdefer cols.deinit(_.alloc);

        while (_.peek().kind != SqlTokenKind.RParen && _.peek().kind != SqlTokenKind.Eof) {
            const col_name = try _.expectIdent();
            const dt_tok = _.peek();
            if (dt_tok.kind != SqlTokenKind.Identifier && dt_tok.kind != SqlTokenKind.Keyword) {
                return SqlError.UnexpectedToken;
            }
            const data_type = _.advance().lexeme;
            if (_.peek().kind == SqlTokenKind.LParen) {
                * = _.advance();
                while (_.peek().kind != SqlTokenKind.RParen && _.peek().kind != SqlTokenKind.Eof) {
                    * = _.advance();
                }
                if (_.peek().kind == SqlTokenKind.RParen) {
                    * = _.advance();
                }
            }

            let not_null = false;
            let primary_key = false;
            while (_.peek().kind == SqlTokenKind.Keyword) {
                if (_.matchKw("NOT")) {
                    try _.expectKw("NULL");
                    not_null = true;
                } else if (_.matchKw("PRIMARY")) {
                    try _.expectKw("KEY");
                    primary_key = true;
                } else {
                    break;
                }
            }

            cols.append(_.alloc, ColumnDef{
                .name = col_name,
                .data_type = data_type,
                .not_null = not_null,
                .primary_key = primary_key,
            }) catch return SqlError.OutOfMemory;

            if (_.peek().kind == SqlTokenKind.Comma) {
                * = _.advance();
            } else {
                break;
            }
        }

        if (_.peek().kind != SqlTokenKind.RParen) {
            return SqlError.UnexpectedToken;
        }
        * = _.advance();

        return CreateTableStmt{
            .table_name = table_name,
            .if_not_exists = if_not_exists,
            .columns = cols.toOwnedSlice(_.alloc) catch return SqlError.OutOfMemory,
        };
    }
}

pub fn parseSqlWithMaxDepth(alloc: std.mem.Allocator, sql: string, max_depth: usize): !SqlStmt {
    const tokens = try tokenizeSql(alloc, sql);
    defer alloc.free(tokens);

    let p = Parser{
        .alloc = alloc,
        .tokens = tokens,
        .pos = 0,
        .depth = 0,
        .max_depth = max_depth,
    };

    let stmt = SqlStmt{
        .kind = SqlStmtKind.Select,
        .select_stmt = null,
        .insert_stmt = null,
        .update_stmt = null,
        .delete_stmt = null,
        .create_table_stmt = null,
    };
    errdefer stmt.deinit(alloc);

    if (p.isKw("SELECT")) {
        stmt.kind = SqlStmtKind.Select;
        stmt.select_stmt = try p.parseSelect();
    } else if (p.isKw("INSERT")) {
        stmt.kind = SqlStmtKind.Insert;
        stmt.insert_stmt = try p.parseInsert();
    } else if (p.isKw("UPDATE")) {
        stmt.kind = SqlStmtKind.Update;
        stmt.update_stmt = try p.parseUpdate();
    } else if (p.isKw("DELETE")) {
        stmt.kind = SqlStmtKind.Delete;
        stmt.delete_stmt = try p.parseDelete();
    } else if (p.isKw("CREATE")) {
        stmt.kind = SqlStmtKind.CreateTable;
        stmt.create_table_stmt = try p.parseCreateTable();
    } else if (p.peek().kind == SqlTokenKind.Eof) {
        return SqlError.UnexpectedEof;
    } else {
        return SqlError.UnexpectedToken;
    }

    if (p.peek().kind == SqlTokenKind.Semicolon) {
        * = p.advance();
    }
    if (p.peek().kind != SqlTokenKind.Eof) {
        return SqlError.UnexpectedToken;
    }

    return stmt;
}

pub fn parseSql(alloc: std.mem.Allocator, sql: string): !SqlStmt {
    return parseSqlWithMaxDepth(alloc, sql, 32);
}
