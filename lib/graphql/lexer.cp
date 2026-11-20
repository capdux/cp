// Source: cp/lib/graphql/lexer.cp (GraphQL Lexer / Tokenizer)
const std = @import("std");
import { GqlError, GqlTokenKind, GqlToken } from "./types.cp";

fn isNameStart(c: u8): bool {
    return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_';
}

fn isNameContinue(c: u8): bool {
    return isNameStart(c) or (c >= '0' and c <= '9');
}

fn isDigit(c: u8): bool {
    return c >= '0' and c <= '9';
}

pub fn tokenize(alloc: std.mem.Allocator, source: []const u8): GqlError![]GqlToken {
    let tokens = std.ArrayList(GqlToken).empty;
    errdefer tokens.deinit(alloc);

    let pos: usize = 0;
    let line: usize = 1;
    let col: usize = 1;
    const len = source.len;

    while (pos < len) {
        const c = source[pos];

        // Insignificant whitespace, BOM, and commas
        if (c == ' ' or c == '\t' or c == ',') {
            pos += 1;
            col += 1;
            continue;
        }
        if (c == '\n') {
            pos += 1;
            line += 1;
            col = 1;
            continue;
        }
        if (c == '\r') {
            pos += 1;
            if (pos < len and source[pos] == '\n') {
                pos += 1;
            }
            line += 1;
            col = 1;
            continue;
        }
        // UTF-8 BOM (0xEF 0xBB 0xBF)
        if (c == 0xEF and pos + 2 < len and source[pos + 1] == 0xBB and source[pos + 2] == 0xBF) {
            pos += 3;
            continue;
        }
        // Line comment
        if (c == '#') {
            pos += 1;
            col += 1;
            while (pos < len and source[pos] != '\n' and source[pos] != '\r') {
                pos += 1;
                col += 1;
            }
            continue;
        }

        const start_line = line;
        const start_col = col;
        const start_pos = pos;

        // Punctuators
        if (c == '!') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .Bang, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == '$') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .Dollar, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == '&') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .Amp, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == '(') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .ParenL, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == ')') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .ParenR, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == ':') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .Colon, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == '=') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .Equals, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == '@') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .At, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == '[') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .BracketL, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == ']') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .BracketR, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == '{') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .BraceL, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == '|') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .Pipe, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == '}') {
            pos += 1;
            col += 1;
            try tokens.append(alloc, .{ .kind = .BraceR, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }
        if (c == '.') {
            if (pos + 2 < len and source[pos + 1] == '.' and source[pos + 2] == '.') {
                pos += 3;
                col += 3;
                try tokens.append(alloc, .{ .kind = .Spread, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
                continue;
            }
            return error.InvalidCharacter;
        }

        // Name
        if (isNameStart(c)) {
            pos += 1;
            col += 1;
            while (pos < len and isNameContinue(source[pos])) {
                pos += 1;
                col += 1;
            }
            try tokens.append(alloc, .{ .kind = .Name, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }

        // Int or Float
        if (c == '-' or isDigit(c)) {
            let is_float = false;
            if (c == '-') {
                pos += 1;
                col += 1;
                if (pos >= len or !isDigit(source[pos])) {
                    return error.InvalidNumber;
                }
            }
            if (source[pos] == '0') {
                pos += 1;
                col += 1;
                if (pos < len and isDigit(source[pos])) {
                    return error.InvalidNumber;
                }
            } else {
                while (pos < len and isDigit(source[pos])) {
                    pos += 1;
                    col += 1;
                }
            }
            if (pos < len and source[pos] == '.') {
                is_float = true;
                pos += 1;
                col += 1;
                if (pos >= len or !isDigit(source[pos])) {
                    return error.InvalidNumber;
                }
                while (pos < len and isDigit(source[pos])) {
                    pos += 1;
                    col += 1;
                }
            }
            if (pos < len and (source[pos] == 'e' or source[pos] == 'E')) {
                is_float = true;
                pos += 1;
                col += 1;
                if (pos < len and (source[pos] == '+' or source[pos] == '-')) {
                    pos += 1;
                    col += 1;
                }
                if (pos >= len or !isDigit(source[pos])) {
                    return error.InvalidNumber;
                }
                while (pos < len and isDigit(source[pos])) {
                    pos += 1;
                    col += 1;
                }
            }
            if (pos < len and isNameStart(source[pos])) {
                return error.InvalidNumber;
            }
            const kind: GqlTokenKind = if (is_float) .FloatValue else .IntValue;
            try tokens.append(alloc, .{ .kind = kind, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
            continue;
        }

        // String or BlockString
        if (c == '"') {
            if (pos + 2 < len and source[pos + 1] == '"' and source[pos + 2] == '"') {
                // Block string """..."""
                pos += 3;
                col += 3;
                let closed = false;
                while (pos < len) {
                    const ch = source[pos];
                    if (ch == '\\' and pos + 3 < len and source[pos + 1] == '"' and source[pos + 2] == '"' and source[pos + 3] == '"') {
                        pos += 4;
                        col += 4;
                        continue;
                    }
                    if (ch == '"' and pos + 2 < len and source[pos + 1] == '"' and source[pos + 2] == '"') {
                        pos += 3;
                        col += 3;
                        closed = true;
                        break;
                    }
                    if (ch == '\n') {
                        pos += 1;
                        line += 1;
                        col = 1;
                        continue;
                    }
                    if (ch == '\r') {
                        pos += 1;
                        if (pos < len and source[pos] == '\n') {
                            pos += 1;
                        }
                        line += 1;
                        col = 1;
                        continue;
                    }
                    pos += 1;
                    col += 1;
                }
                if (!closed) {
                    return error.UnterminatedString;
                }
                try tokens.append(alloc, .{ .kind = .BlockStringValue, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
                continue;
            } else {
                // Single-line string "..."
                pos += 1;
                col += 1;
                let closed = false;
                while (pos < len) {
                    const ch = source[pos];
                    if (ch == '\n' or ch == '\r') {
                        return error.UnterminatedString;
                    }
                    if (ch == '"') {
                        pos += 1;
                        col += 1;
                        closed = true;
                        break;
                    }
                    if (ch == '\\') {
                        pos += 1;
                        col += 1;
                        if (pos >= len) {
                            return error.UnterminatedString;
                        }
                        pos += 1;
                        col += 1;
                        continue;
                    }
                    pos += 1;
                    col += 1;
                }
                if (!closed) {
                    return error.UnterminatedString;
                }
                try tokens.append(alloc, .{ .kind = .StringValue, .lexeme = source[start_pos..pos], .line = start_line, .column = start_col });
                continue;
            }
        }

        return error.InvalidCharacter;
    }

    try tokens.append(alloc, .{ .kind = .Eof, .lexeme = "", .line = line, .column = col });
    return tokens.toOwnedSlice(alloc);
}
