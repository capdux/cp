// Source: cp/lib/sql_ast/ast.cp (Pure CP SQL AST Definitions)

const std = @import("std");

pub enum SqlExprKind {
    Literal,
    ColumnRef,
    Placeholder,
    UnaryOp,
    BinaryOp,
    FuncCall,
    IsNull,
    InList,
    Paren,
}

pub struct SqlExpr {
    pub kind: SqlExprKind,
    pub text: string,
    pub qualifier: string?,
    pub op: string,
    pub negated: bool,
    pub left: (*SqlExpr)?,
    pub right: (*SqlExpr)?,
    pub args: []*SqlExpr,

    pub fn deinit(alloc: std.mem.Allocator) {
        if (l := _.left) {
            l.deinit(alloc);
        }
        if (r := _.right) {
            r.deinit(alloc);
        }
        let i: usize = 0;
        while (i < _.args.len) {
            _.args[i].deinit(alloc);
            i += 1;
        }
        if (_.args.len > 0) {
            alloc.free(_.args);
        }
        alloc.destroy(_);
    }
}

pub struct SelectItem {
    pub expr: *SqlExpr,
    pub alias: string?,
    pub is_star: bool,
}

pub struct JoinClause {
    pub join_type: string,
    pub table_name: string,
    pub alias: string?,
    pub on_expr: (*SqlExpr)?,
}

pub struct OrderByItem {
    pub expr: *SqlExpr,
    pub desc: bool,
}

pub struct SelectStmt {
    pub distinct: bool,
    pub columns: []SelectItem,
    pub from_table: string?,
    pub from_alias: string?,
    pub joins: []JoinClause,
    pub where_expr: (*SqlExpr)?,
    pub group_by: []*SqlExpr,
    pub order_by: []OrderByItem,
    pub limit: (*SqlExpr)?,
    pub offset: (*SqlExpr)?,

    pub fn deinit(alloc: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.columns.len) {
            _.columns[i].expr.deinit(alloc);
            i += 1;
        }
        if (_.columns.len > 0) {
            alloc.free(_.columns);
        }
        i = 0;
        while (i < _.joins.len) {
            if (on_e := _.joins[i].on_expr) {
                on_e.deinit(alloc);
            }
            i += 1;
        }
        if (_.joins.len > 0) {
            alloc.free(_.joins);
        }
        if (w := _.where_expr) {
            w.deinit(alloc);
        }
        i = 0;
        while (i < _.group_by.len) {
            _.group_by[i].deinit(alloc);
            i += 1;
        }
        if (_.group_by.len > 0) {
            alloc.free(_.group_by);
        }
        i = 0;
        while (i < _.order_by.len) {
            _.order_by[i].expr.deinit(alloc);
            i += 1;
        }
        if (_.order_by.len > 0) {
            alloc.free(_.order_by);
        }
        if (lim := _.limit) {
            lim.deinit(alloc);
        }
        if (off := _.offset) {
            off.deinit(alloc);
        }
    }
}

pub struct ValueRow {
    pub items: []*SqlExpr,
}

pub struct InsertStmt {
    pub table_name: string,
    pub columns: []string,
    pub rows: []ValueRow,

    pub fn deinit(alloc: std.mem.Allocator) {
        if (_.columns.len > 0) {
            alloc.free(_.columns);
        }
        let i: usize = 0;
        while (i < _.rows.len) {
            let j: usize = 0;
            while (j < _.rows[i].items.len) {
                _.rows[i].items[j].deinit(alloc);
                j += 1;
            }
            if (_.rows[i].items.len > 0) {
                alloc.free(_.rows[i].items);
            }
            i += 1;
        }
        if (_.rows.len > 0) {
            alloc.free(_.rows);
        }
    }
}

pub struct AssignItem {
    pub column: string,
    pub value: *SqlExpr,
}

pub struct UpdateStmt {
    pub table_name: string,
    pub assignments: []AssignItem,
    pub where_expr: (*SqlExpr)?,

    pub fn deinit(alloc: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.assignments.len) {
            _.assignments[i].value.deinit(alloc);
            i += 1;
        }
        if (_.assignments.len > 0) {
            alloc.free(_.assignments);
        }
        if (w := _.where_expr) {
            w.deinit(alloc);
        }
    }
}

pub struct DeleteStmt {
    pub table_name: string,
    pub where_expr: (*SqlExpr)?,

    pub fn deinit(alloc: std.mem.Allocator) {
        if (w := _.where_expr) {
            w.deinit(alloc);
        }
    }
}

pub struct ColumnDef {
    pub name: string,
    pub data_type: string,
    pub not_null: bool,
    pub primary_key: bool,
}

pub struct CreateTableStmt {
    pub table_name: string,
    pub if_not_exists: bool,
    pub columns: []ColumnDef,

    pub fn deinit(alloc: std.mem.Allocator) {
        if (_.columns.len > 0) {
            alloc.free(_.columns);
        }
    }
}

pub enum SqlStmtKind {
    Select,
    Insert,
    Update,
    Delete,
    CreateTable,
}

pub struct SqlStmt {
    pub kind: SqlStmtKind,
    pub select_stmt: SelectStmt?,
    pub insert_stmt: InsertStmt?,
    pub update_stmt: UpdateStmt?,
    pub delete_stmt: DeleteStmt?,
    pub create_table_stmt: CreateTableStmt?,

    pub fn deinit(alloc: std.mem.Allocator) {
        if (s := _.select_stmt) {
            s.deinit(alloc);
        }
        if (ins := _.insert_stmt) {
            ins.deinit(alloc);
        }
        if (upd := _.update_stmt) {
            upd.deinit(alloc);
        }
        if (del := _.delete_stmt) {
            del.deinit(alloc);
        }
        if (ct := _.create_table_stmt) {
            ct.deinit(alloc);
        }
    }
}
