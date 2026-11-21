// Source: cp/lib/sql_ast/analyzer.cp (SQL Fingerprint Normalization, Table Extraction & Read-Only Guard)

const std = @import("std");
import { SqlError, SqlTokenKind, SqlToken, tokenizeSql, eqIgnoreCase } from "./lexer.cp";
import { parseSql } from "./parser.cp";

fn appendUpper(alloc: std.mem.Allocator, out: *std.ArrayList(u8), s: string): !void {
    let i: usize = 0;
    while (i < s.len) {
        let c = s[i];
        if (c >= 'a' && c <= 'z') {
            c = c - 32;
        }
        out.append(alloc, c) catch return SqlError.OutOfMemory;
        i += 1;
    }
}

fn appendStr(alloc: std.mem.Allocator, out: *std.ArrayList(u8), s: string): !void {
    let i: usize = 0;
    while (i < s.len) {
        out.append(alloc, s[i]) catch return SqlError.OutOfMemory;
        i += 1;
    }
}

fn needsSpaceBetween(prev: SqlToken, curr: SqlToken): bool {
    if (curr.kind == SqlTokenKind.Comma || curr.kind == SqlTokenKind.Semicolon || curr.kind == SqlTokenKind.RParen || curr.kind == SqlTokenKind.Dot) {
        return false;
    }
    if (prev.kind == SqlTokenKind.LParen || prev.kind == SqlTokenKind.Dot) {
        return false;
    }
    if (prev.kind == SqlTokenKind.Operator && std.mem.eql(u8, prev.lexeme, "::")) {
        return false;
    }
    if (curr.kind == SqlTokenKind.Operator && std.mem.eql(u8, curr.lexeme, "::")) {
        return false;
    }
    if (curr.kind == SqlTokenKind.LParen && (prev.kind == SqlTokenKind.Identifier || prev.kind == SqlTokenKind.QuotedIdentifier)) {
        return false;
    }
    return true;
}

pub fn normalizeSqlFingerprint(alloc: std.mem.Allocator, sql: string): !string {
    const tokens = try tokenizeSql(alloc, sql);
    defer alloc.free(tokens);

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    let has_prev = false;
    let prev = SqlToken{ .kind = SqlTokenKind.Eof, .lexeme = "", .line = 0, .col = 0 };

    let i: usize = 0;
    while (i < tokens.len) {
        const tok = tokens[i];
        if (tok.kind == SqlTokenKind.Eof) {
            break;
        }

        if (has_prev && needsSpaceBetween(prev, tok)) {
            out.append(alloc, ' ') catch return SqlError.OutOfMemory;
        }

        if (tok.kind == SqlTokenKind.StringLiteral || tok.kind == SqlTokenKind.NumberLiteral || tok.kind == SqlTokenKind.Placeholder) {
            out.append(alloc, '?') catch return SqlError.OutOfMemory;
        } else if (tok.kind == SqlTokenKind.Keyword) {
            try appendUpper(alloc, &out, tok.lexeme);
        } else {
            try appendStr(alloc, &out, tok.lexeme);
        }

        prev = tok;
        has_prev = true;
        i += 1;
    }

    return out.toOwnedSlice(alloc) catch return SqlError.OutOfMemory;
}

fn addUniqueTable(alloc: std.mem.Allocator, list: *std.ArrayList(string), name: string): !void {
    let i: usize = 0;
    while (i < list.items.len) {
        if (std.mem.eql(u8, list.items[i], name)) {
            return;
        }
        i += 1;
    }
    list.append(alloc, name) catch return SqlError.OutOfMemory;
}

pub fn extractTableNames(alloc: std.mem.Allocator, sql: string): ![]string {
    let stmt = try parseSql(alloc, sql);
    defer stmt.deinit(alloc);

    let tables = std.ArrayList(string).empty;
    errdefer tables.deinit(alloc);

    if (sel := stmt.select_stmt) {
        if (ft := sel.from_table) {
            try addUniqueTable(alloc, &tables, ft);
        }
        let i: usize = 0;
        while (i < sel.joins.len) {
            try addUniqueTable(alloc, &tables, sel.joins[i].table_name);
            i += 1;
        }
    } else if (ins := stmt.insert_stmt) {
        try addUniqueTable(alloc, &tables, ins.table_name);
    } else if (upd := stmt.update_stmt) {
        try addUniqueTable(alloc, &tables, upd.table_name);
    } else if (del := stmt.delete_stmt) {
        try addUniqueTable(alloc, &tables, del.table_name);
    } else if (ct := stmt.create_table_stmt) {
        try addUniqueTable(alloc, &tables, ct.table_name);
    }

    return tables.toOwnedSlice(alloc) catch return SqlError.OutOfMemory;
}

pub fn isReadOnlyQuery(sql: string): bool {
    let i: usize = 0;
    let first_word_checked = false;
    let saw_semicolon = false;

    while (i < sql.len) {
        const c = sql[i];

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            i += 1;
            continue;
        }

        if (c == '-' && i + 1 < sql.len && sql[i + 1] == '-') {
            i += 2;
            while (i < sql.len && sql[i] != '\n') {
                i += 1;
            }
            continue;
        }

        if (c == '/' && i + 1 < sql.len && sql[i + 1] == '*') {
            i += 2;
            let closed = false;
            while (i + 1 < sql.len) {
                if (sql[i] == '*' && sql[i + 1] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) {
                return false;
            }
            continue;
        }

        if (saw_semicolon) {
            return false;
        }

        if (c == ';') {
            if (!first_word_checked) {
                return false;
            }
            saw_semicolon = true;
            i += 1;
            continue;
        }

        if (c == '\'' || c == '"' || c == '`') {
            if (!first_word_checked) {
                return false;
            }
            const quote = c;
            i += 1;
            let closed = false;
            while (i < sql.len) {
                if (sql[i] == quote) {
                    if (i + 1 < sql.len && sql[i + 1] == quote) {
                        i += 2;
                        continue;
                    }
                    i += 1;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) {
                return false;
            }
            continue;
        }

        if (!first_word_checked) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
                const start = i;
                while (i < sql.len && ((sql[i] >= 'a' && sql[i] <= 'z') || (sql[i] >= 'A' && sql[i] <= 'Z') || (sql[i] >= '0' && sql[i] <= '9') || sql[i] == '_')) {
                    i += 1;
                }
                if (!eqIgnoreCase(sql[start..i], "SELECT")) {
                    return false;
                }
                first_word_checked = true;
                continue;
            }
            return false;
        }

        i += 1;
    }

    return first_word_checked;
}
