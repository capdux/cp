// @cap/graphql-parse — Zero-heap GraphQL Spec lexer, document parser & query minifier in pure CP
// Clean-room MIT implementation inspired by npm `graphql` v16.

import { isDigit, isHexDigit, strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";

// Token Kinds
pub const GQL_TOK_EOF: u8 = 0;
pub const GQL_TOK_BANG: u8 = 1;         // !
pub const GQL_TOK_DOLLAR: u8 = 2;       // $
pub const GQL_TOK_AMP: u8 = 3;          // &
pub const GQL_TOK_LPAREN: u8 = 4;       // (
pub const GQL_TOK_RPAREN: u8 = 5;       // )
pub const GQL_TOK_SPREAD: u8 = 6;       // ...
pub const GQL_TOK_COLON: u8 = 7;        // :
pub const GQL_TOK_EQUALS: u8 = 8;       // =
pub const GQL_TOK_AT: u8 = 9;           // @
pub const GQL_TOK_LBRACKET: u8 = 10;    // [
pub const GQL_TOK_RBRACKET: u8 = 11;    // ]
pub const GQL_TOK_LBRACE: u8 = 12;      // {
pub const GQL_TOK_PIPE: u8 = 13;        // |
pub const GQL_TOK_RBRACE: u8 = 14;      // }
pub const GQL_TOK_NAME: u8 = 15;
pub const GQL_TOK_INT: u8 = 16;
pub const GQL_TOK_FLOAT: u8 = 17;
pub const GQL_TOK_STRING: u8 = 18;
pub const GQL_TOK_BLOCK_STRING: u8 = 19;
pub const GQL_TOK_ERROR: u8 = 255;

// Operation Types
pub const GQL_OP_NONE: u8 = 0;
pub const GQL_OP_QUERY: u8 = 1;
pub const GQL_OP_MUTATION: u8 = 2;
pub const GQL_OP_SUBSCRIPTION: u8 = 3;

pub struct GqlToken {
    pub kind: u8,
    pub start: usize,
    pub end: usize,
    pub line: usize,
    pub col: usize,
}

pub struct GqlParseResult {
    pub ok: bool,
    pub error_pos: usize,
    pub operation_count: usize,
    pub fragment_count: usize,
    pub field_count: usize,
    pub max_depth: usize,
    pub primary_op_type: u8,
    pub primary_op_name: string,
}

// ============================================================================
// Private helpers (all prefixed with `gql` to avoid module flattening clashes)
// ============================================================================

fn gqlIsNameStart(c: u8): bool {
    return c == 95 || (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

fn gqlIsNameContinue(c: u8): bool {
    return gqlIsNameStart(c) || isDigit(c);
}

fn gqlIsWordTok(kind: u8): bool {
    return kind == GQL_TOK_NAME || kind == GQL_TOK_INT || kind == GQL_TOK_FLOAT;
}

fn gqlWriteByte(out: []u8, pos: *usize, b: u8) {
    if (pos.* < out.len) {
        out[pos.*] = b;
        pos.* = pos.* + 1;
    }
}

// ============================================================================
// GraphQL Spec Lexer (`GqlLexer`)
// ============================================================================

pub struct GqlLexer {
    pub src: string,
    pub pos: usize,
    pub line: usize,
    pub col: usize,

    pub static fn new(src: string): GqlLexer {
        return GqlLexer {
            src: src,
            pos: 0,
            line: 1,
            col: 1,
        };
    }

    pub fn tokenText(tok: GqlToken): string {
        if (tok.start <= tok.end && tok.end <= _.src.len) {
            return _.src[tok.start..tok.end];
        }
        return "";
    }

    pub fn peekToken(): GqlToken {
        let copy = GqlLexer {
            src: _.src,
            pos: _.pos,
            line: _.line,
            col: _.col,
        };
        return copy.nextToken();
    }

    pub fn nextToken(*_): GqlToken {
        // Skip ignored whitespace, commas, BOM, and `#` comments
        while (_.pos < _.src.len) {
            const c: u8 = _.src[_.pos];
            // UTF-8 BOM (EF BB BF)
            if (c == 239 && _.pos + 2 < _.src.len && _.src[_.pos + 1] == 187 && _.src[_.pos + 2] == 191) {
                _.pos = _.pos + 3;
                continue;
            }
            if (c == 32 || c == 9 || c == 44) {
                _.pos = _.pos + 1;
                _.col = _.col + 1;
                continue;
            }
            if (c == 10) {
                _.pos = _.pos + 1;
                _.line = _.line + 1;
                _.col = 1;
                continue;
            }
            if (c == 13) {
                _.pos = _.pos + 1;
                if (_.pos < _.src.len && _.src[_.pos] == 10) {
                    _.pos = _.pos + 1;
                }
                _.line = _.line + 1;
                _.col = 1;
                continue;
            }
            if (c == 35) {
                // Comment `# ...` until newline
                while (_.pos < _.src.len && _.src[_.pos] != 10 && _.src[_.pos] != 13) {
                    _.pos = _.pos + 1;
                    _.col = _.col + 1;
                }
                continue;
            }
            break;
        }

        const start: usize = _.pos;
        const start_line: usize = _.line;
        const start_col: usize = _.col;

        if (_.pos >= _.src.len) {
            return GqlToken { kind: GQL_TOK_EOF, start: start, end: start, line: start_line, col: start_col };
        }

        const ch: u8 = _.src[_.pos];

        // Single-char punctuators
        if (ch == 33) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_BANG, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 36) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_DOLLAR, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 38) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_AMP, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 40) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_LPAREN, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 41) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_RPAREN, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 58) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_COLON, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 61) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_EQUALS, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 64) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_AT, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 91) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_LBRACKET, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 93) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_RBRACKET, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 123) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_LBRACE, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 124) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_PIPE, start: start, end: _.pos, line: start_line, col: start_col }; }
        if (ch == 125) { _.pos = _.pos + 1; _.col = _.col + 1; return GqlToken { kind: GQL_TOK_RBRACE, start: start, end: _.pos, line: start_line, col: start_col }; }

        // Spread `...`
        if (ch == 46) {
            if (_.pos + 2 < _.src.len && _.src[_.pos + 1] == 46 && _.src[_.pos + 2] == 46) {
                _.pos = _.pos + 3;
                _.col = _.col + 3;
                return GqlToken { kind: GQL_TOK_SPREAD, start: start, end: _.pos, line: start_line, col: start_col };
            }
            _.pos = _.pos + 1;
            _.col = _.col + 1;
            return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
        }

        // Name
        if (gqlIsNameStart(ch)) {
            _.pos = _.pos + 1;
            _.col = _.col + 1;
            while (_.pos < _.src.len && gqlIsNameContinue(_.src[_.pos])) {
                _.pos = _.pos + 1;
                _.col = _.col + 1;
            }
            return GqlToken { kind: GQL_TOK_NAME, start: start, end: _.pos, line: start_line, col: start_col };
        }

        // Int or Float
        if (ch == 45 || isDigit(ch)) {
            let is_float: bool = false;
            if (ch == 45) {
                _.pos = _.pos + 1;
                _.col = _.col + 1;
                if (_.pos >= _.src.len || !isDigit(_.src[_.pos])) {
                    return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
                }
            }
            if (_.src[_.pos] == 48) {
                _.pos = _.pos + 1;
                _.col = _.col + 1;
                if (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                    return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
                }
            } else {
                while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                    _.pos = _.pos + 1;
                    _.col = _.col + 1;
                }
            }
            // Fractional part
            if (_.pos < _.src.len && _.src[_.pos] == 46) {
                is_float = true;
                _.pos = _.pos + 1;
                _.col = _.col + 1;
                if (_.pos >= _.src.len || !isDigit(_.src[_.pos])) {
                    return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
                }
                while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                    _.pos = _.pos + 1;
                    _.col = _.col + 1;
                }
            }
            // Exponent part
            if (_.pos < _.src.len && (_.src[_.pos] == 101 || _.src[_.pos] == 69)) {
                is_float = true;
                _.pos = _.pos + 1;
                _.col = _.col + 1;
                if (_.pos < _.src.len && (_.src[_.pos] == 43 || _.src[_.pos] == 45)) {
                    _.pos = _.pos + 1;
                    _.col = _.col + 1;
                }
                if (_.pos >= _.src.len || !isDigit(_.src[_.pos])) {
                    return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
                }
                while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                    _.pos = _.pos + 1;
                    _.col = _.col + 1;
                }
            }
            // Numbers cannot be immediately followed by NameStart or '.'
            if (_.pos < _.src.len && (gqlIsNameStart(_.src[_.pos]) || _.src[_.pos] == 46)) {
                return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
            }
            const num_kind: u8 = if (is_float) { GQL_TOK_FLOAT } else { GQL_TOK_INT };
            return GqlToken { kind: num_kind, start: start, end: _.pos, line: start_line, col: start_col };
        }

        // String or Block String (`"""..."""`)
        if (ch == 34) {
            if (_.pos + 2 < _.src.len && _.src[_.pos + 1] == 34 && _.src[_.pos + 2] == 34) {
                _.pos = _.pos + 3;
                _.col = _.col + 3;
                while (_.pos < _.src.len) {
                    const sc: u8 = _.src[_.pos];
                    if (sc == 92 && _.pos + 3 < _.src.len &&
                        _.src[_.pos + 1] == 34 && _.src[_.pos + 2] == 34 && _.src[_.pos + 3] == 34) {
                        _.pos = _.pos + 4;
                        _.col = _.col + 4;
                        continue;
                    }
                    if (sc == 34 && _.pos + 2 < _.src.len && _.src[_.pos + 1] == 34 && _.src[_.pos + 2] == 34) {
                        _.pos = _.pos + 3;
                        _.col = _.col + 3;
                        return GqlToken { kind: GQL_TOK_BLOCK_STRING, start: start, end: _.pos, line: start_line, col: start_col };
                    }
                    if (sc == 10) {
                        _.pos = _.pos + 1;
                        _.line = _.line + 1;
                        _.col = 1;
                    } else if (sc == 13) {
                        _.pos = _.pos + 1;
                        if (_.pos < _.src.len && _.src[_.pos] == 10) {
                            _.pos = _.pos + 1;
                        }
                        _.line = _.line + 1;
                        _.col = 1;
                    } else {
                        _.pos = _.pos + 1;
                        _.col = _.col + 1;
                    }
                }
                return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
            }

            // Single-line string
            _.pos = _.pos + 1;
            _.col = _.col + 1;
            while (_.pos < _.src.len) {
                const sc: u8 = _.src[_.pos];
                if (sc == 34) {
                    _.pos = _.pos + 1;
                    _.col = _.col + 1;
                    return GqlToken { kind: GQL_TOK_STRING, start: start, end: _.pos, line: start_line, col: start_col };
                }
                if (sc == 10 || sc == 13) {
                    return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
                }
                if (sc == 92) {
                    _.pos = _.pos + 1;
                    _.col = _.col + 1;
                    if (_.pos >= _.src.len) {
                        return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
                    }
                    const esc: u8 = _.src[_.pos];
                    if (esc == 34 || esc == 92 || esc == 47 || esc == 98 ||
                        esc == 102 || esc == 110 || esc == 114 || esc == 116) {
                        _.pos = _.pos + 1;
                        _.col = _.col + 1;
                    } else if (esc == 117) {
                        // \uXXXX
                        if (_.pos + 4 >= _.src.len) {
                            return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
                        }
                        if (!isHexDigit(_.src[_.pos + 1]) || !isHexDigit(_.src[_.pos + 2]) ||
                            !isHexDigit(_.src[_.pos + 3]) || !isHexDigit(_.src[_.pos + 4])) {
                            return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
                        }
                        _.pos = _.pos + 5;
                        _.col = _.col + 5;
                    } else {
                        return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
                    }
                    continue;
                }
                _.pos = _.pos + 1;
                _.col = _.col + 1;
            }
            return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
        }

        // Unrecognized character
        _.pos = _.pos + 1;
        _.col = _.col + 1;
        return GqlToken { kind: GQL_TOK_ERROR, start: start, end: _.pos, line: start_line, col: start_col };
    }
}

// ============================================================================
// Recursive-Descent Document Parser (private state)
// ============================================================================

struct GqlParserState {
    lexer: GqlLexer,
    cur: GqlToken,
    ok: bool,
    error_pos: usize,
    operation_count: usize,
    fragment_count: usize,
    field_count: usize,
    max_depth: usize,
    primary_op_type: u8,
    primary_op_name: string,
}

fn gqlAdvance(st: *GqlParserState) {
    st.cur = st.lexer.nextToken();
    if (st.cur.kind == GQL_TOK_ERROR && st.ok) {
        st.ok = false;
        st.error_pos = st.cur.start;
    }
}

fn gqlFail(st: *GqlParserState) {
    if (st.ok) {
        st.ok = false;
        st.error_pos = st.cur.start;
    }
}

fn gqlCurText(st: *GqlParserState): string {
    return st.lexer.tokenText(st.cur);
}

fn gqlExpect(st: *GqlParserState, kind: u8): bool {
    if (!st.ok) {
        return false;
    }
    if (st.cur.kind != kind) {
        gqlFail(st);
        return false;
    }
    gqlAdvance(st);
    return true;
}

fn gqlParseType(st: *GqlParserState, depth: usize) {
    if (!st.ok || depth >= 32) {
        if (depth >= 32) {
            gqlFail(st);
        }
        return;
    }
    if (st.cur.kind == GQL_TOK_LBRACKET) {
        gqlAdvance(st);
        gqlParseType(st, depth + 1);
        if (!gqlExpect(st, GQL_TOK_RBRACKET)) {
            return;
        }
    } else if (st.cur.kind == GQL_TOK_NAME) {
        gqlAdvance(st);
    } else {
        gqlFail(st);
        return;
    }
    if (st.ok && st.cur.kind == GQL_TOK_BANG) {
        gqlAdvance(st);
    }
}

fn gqlParseValue(st: *GqlParserState, is_const: bool, depth: usize) {
    if (!st.ok || depth >= 32) {
        if (depth >= 32) {
            gqlFail(st);
        }
        return;
    }
    const k: u8 = st.cur.kind;
    if (k == GQL_TOK_DOLLAR) {
        if (is_const) {
            gqlFail(st);
            return;
        }
        gqlAdvance(st);
        gqlExpect(st, GQL_TOK_NAME);
        return;
    }
    if (k == GQL_TOK_INT || k == GQL_TOK_FLOAT || k == GQL_TOK_STRING ||
        k == GQL_TOK_BLOCK_STRING || k == GQL_TOK_NAME) {
        gqlAdvance(st);
        return;
    }
    if (k == GQL_TOK_LBRACKET) {
        gqlAdvance(st);
        while (st.ok && st.cur.kind != GQL_TOK_RBRACKET && st.cur.kind != GQL_TOK_EOF) {
            gqlParseValue(st, is_const, depth + 1);
        }
        gqlExpect(st, GQL_TOK_RBRACKET);
        return;
    }
    if (k == GQL_TOK_LBRACE) {
        gqlAdvance(st);
        while (st.ok && st.cur.kind != GQL_TOK_RBRACE && st.cur.kind != GQL_TOK_EOF) {
            if (!gqlExpect(st, GQL_TOK_NAME)) {
                return;
            }
            if (!gqlExpect(st, GQL_TOK_COLON)) {
                return;
            }
            gqlParseValue(st, is_const, depth + 1);
        }
        gqlExpect(st, GQL_TOK_RBRACE);
        return;
    }
    gqlFail(st);
}

fn gqlParseArguments(st: *GqlParserState, is_const: bool) {
    if (!st.ok || st.cur.kind != GQL_TOK_LPAREN) {
        return;
    }
    gqlAdvance(st);
    if (st.cur.kind == GQL_TOK_RPAREN) {
        // Empty argument list `()` is invalid per GraphQL Spec
        gqlFail(st);
        return;
    }
    while (st.ok && st.cur.kind != GQL_TOK_RPAREN && st.cur.kind != GQL_TOK_EOF) {
        if (!gqlExpect(st, GQL_TOK_NAME)) {
            return;
        }
        if (!gqlExpect(st, GQL_TOK_COLON)) {
            return;
        }
        gqlParseValue(st, is_const, 0);
    }
    gqlExpect(st, GQL_TOK_RPAREN);
}

fn gqlParseDirectives(st: *GqlParserState, is_const: bool) {
    while (st.ok && st.cur.kind == GQL_TOK_AT) {
        gqlAdvance(st);
        if (!gqlExpect(st, GQL_TOK_NAME)) {
            return;
        }
        gqlParseArguments(st, is_const);
    }
}

fn gqlParseVariableDefinitions(st: *GqlParserState) {
    if (!st.ok || st.cur.kind != GQL_TOK_LPAREN) {
        return;
    }
    gqlAdvance(st);
    if (st.cur.kind == GQL_TOK_RPAREN) {
        gqlFail(st);
        return;
    }
    while (st.ok && st.cur.kind != GQL_TOK_RPAREN && st.cur.kind != GQL_TOK_EOF) {
        if (!gqlExpect(st, GQL_TOK_DOLLAR)) {
            return;
        }
        if (!gqlExpect(st, GQL_TOK_NAME)) {
            return;
        }
        if (!gqlExpect(st, GQL_TOK_COLON)) {
            return;
        }
        gqlParseType(st, 0);
        if (st.ok && st.cur.kind == GQL_TOK_EQUALS) {
            gqlAdvance(st);
            gqlParseValue(st, true, 0);
        }
        gqlParseDirectives(st, true);
    }
    gqlExpect(st, GQL_TOK_RPAREN);
}

fn gqlParseSelectionSet(st: *GqlParserState, depth: usize) {
    if (!st.ok || depth >= 64) {
        if (depth >= 64) {
            gqlFail(st);
        }
        return;
    }
    if (!gqlExpect(st, GQL_TOK_LBRACE)) {
        return;
    }
    if (depth > st.max_depth) {
        st.max_depth = depth;
    }
    if (st.cur.kind == GQL_TOK_RBRACE) {
        // Empty selection set `{}` is invalid per GraphQL Spec
        gqlFail(st);
        return;
    }
    while (st.ok && st.cur.kind != GQL_TOK_RBRACE && st.cur.kind != GQL_TOK_EOF) {
        if (st.cur.kind == GQL_TOK_SPREAD) {
            gqlAdvance(st);
            if (!st.ok) {
                return;
            }
            if (st.cur.kind == GQL_TOK_NAME && strEq(gqlCurText(st), "on")) {
                // Inline fragment with type condition: `... on TypeName Directives? SelectionSet`
                gqlAdvance(st);
                if (!gqlExpect(st, GQL_TOK_NAME)) {
                    return;
                }
                gqlParseDirectives(st, false);
                gqlParseSelectionSet(st, depth + 1);
            } else if (st.cur.kind == GQL_TOK_AT || st.cur.kind == GQL_TOK_LBRACE) {
                // Inline fragment without type condition: `... Directives? SelectionSet`
                gqlParseDirectives(st, false);
                gqlParseSelectionSet(st, depth + 1);
            } else if (st.cur.kind == GQL_TOK_NAME) {
                // Fragment spread: `...FragmentName Directives?`
                gqlAdvance(st);
                gqlParseDirectives(st, false);
            } else {
                gqlFail(st);
                return;
            }
        } else if (st.cur.kind == GQL_TOK_NAME) {
            // Field: `(alias ':')? name Arguments? Directives? SelectionSet?`
            gqlAdvance(st);
            if (st.ok && st.cur.kind == GQL_TOK_COLON) {
                gqlAdvance(st);
                if (!gqlExpect(st, GQL_TOK_NAME)) {
                    return;
                }
            }
            st.field_count = st.field_count + 1;
            gqlParseArguments(st, false);
            gqlParseDirectives(st, false);
            if (st.ok && st.cur.kind == GQL_TOK_LBRACE) {
                gqlParseSelectionSet(st, depth + 1);
            }
        } else {
            gqlFail(st);
            return;
        }
    }
    gqlExpect(st, GQL_TOK_RBRACE);
}

fn gqlParseDefinition(st: *GqlParserState) {
    if (!st.ok) {
        return;
    }
    if (st.cur.kind == GQL_TOK_LBRACE) {
        // Shorthand query `{ ... }`
        st.operation_count = st.operation_count + 1;
        if (st.primary_op_type == GQL_OP_NONE) {
            st.primary_op_type = GQL_OP_QUERY;
            st.primary_op_name = "";
        }
        gqlParseSelectionSet(st, 1);
        return;
    }
    if (st.cur.kind == GQL_TOK_NAME) {
        const kw: string = gqlCurText(st);
        if (strEq(kw, "query") || strEq(kw, "mutation") || strEq(kw, "subscription")) {
            let op_type: u8 = GQL_OP_QUERY;
            if (strEq(kw, "mutation")) {
                op_type = GQL_OP_MUTATION;
            } else if (strEq(kw, "subscription")) {
                op_type = GQL_OP_SUBSCRIPTION;
            }
            gqlAdvance(st);
            let op_name: string = "";
            if (st.ok && st.cur.kind == GQL_TOK_NAME) {
                op_name = gqlCurText(st);
                gqlAdvance(st);
            }
            st.operation_count = st.operation_count + 1;
            if (st.primary_op_type == GQL_OP_NONE) {
                st.primary_op_type = op_type;
                st.primary_op_name = op_name;
            }
            gqlParseVariableDefinitions(st);
            gqlParseDirectives(st, false);
            gqlParseSelectionSet(st, 1);
            return;
        }
        if (strEq(kw, "fragment")) {
            gqlAdvance(st);
            if (!st.ok || st.cur.kind != GQL_TOK_NAME || strEq(gqlCurText(st), "on")) {
                gqlFail(st);
                return;
            }
            gqlAdvance(st);
            if (!st.ok || st.cur.kind != GQL_TOK_NAME || !strEq(gqlCurText(st), "on")) {
                gqlFail(st);
                return;
            }
            gqlAdvance(st);
            if (!gqlExpect(st, GQL_TOK_NAME)) {
                return;
            }
            st.fragment_count = st.fragment_count + 1;
            gqlParseDirectives(st, false);
            gqlParseSelectionSet(st, 1);
            return;
        }
    }
    gqlFail(st);
}

// ============================================================================
// Public API
// ============================================================================

/// Parse and validate a GraphQL executable document (`query`, `mutation`,
/// `subscription`, shorthand `{ ... }`, `fragment`, variables, directives,
/// aliases, arguments, inline fragments, and fragment spreads).
pub fn parseGraphql(src: string): GqlParseResult {
    let lex = GqlLexer.new(src);
    const first_tok = lex.nextToken();
    let st = GqlParserState {
        lexer: lex,
        cur: first_tok,
        ok: true,
        error_pos: 0,
        operation_count: 0,
        fragment_count: 0,
        field_count: 0,
        max_depth: 0,
        primary_op_type: GQL_OP_NONE,
        primary_op_name: "",
    };

    if (st.cur.kind == GQL_TOK_ERROR) {
        st.ok = false;
        st.error_pos = st.cur.start;
    } else if (st.cur.kind == GQL_TOK_EOF) {
        st.ok = false;
        st.error_pos = 0;
    } else {
        while (st.ok && st.cur.kind != GQL_TOK_EOF) {
            gqlParseDefinition(&st);
        }
    }

    return GqlParseResult {
        ok: st.ok,
        error_pos: st.error_pos,
        operation_count: st.operation_count,
        fragment_count: st.fragment_count,
        field_count: st.field_count,
        max_depth: st.max_depth,
        primary_op_type: st.primary_op_type,
        primary_op_name: st.primary_op_name,
    };
}

/// Minify a GraphQL document into caller-provided `out` with zero heap allocation.
/// Strips `#` comments, insignificant commas, and redundant whitespace while
/// preserving required token separators and string literals verbatim.
/// Returns the number of bytes written to `out`.
pub fn minifyGraphql(src: string, out: []u8): usize {
    if (src.len == 0 || out.len == 0) {
        return 0;
    }
    let lex = GqlLexer.new(src);
    let pos: usize = 0;
    let prev_kind: u8 = GQL_TOK_EOF;
    while (pos < out.len) {
        const tok = lex.nextToken();
        if (tok.kind == GQL_TOK_EOF || tok.kind == GQL_TOK_ERROR) {
            break;
        }
        if (gqlIsWordTok(prev_kind) && (gqlIsWordTok(tok.kind) || tok.kind == GQL_TOK_SPREAD)) {
            gqlWriteByte(out, &pos, 32);
        }
        pos = writeStr(src[tok.start..tok.end], out, pos);
        prev_kind = tok.kind;
    }
    return pos;
}
