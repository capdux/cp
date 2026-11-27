# `graphql` — AI Agent Quick Ref

Zero-heap GraphQL Spec lexer, recursive-descent parser/validator & query minifier (npm `graphql` v16 parser parity).

## Import

```cp
import {
    GQL_TOK_EOF, GQL_TOK_BANG, GQL_TOK_DOLLAR, GQL_TOK_AMP,
    GQL_TOK_LPAREN, GQL_TOK_RPAREN, GQL_TOK_SPREAD, GQL_TOK_COLON,
    GQL_TOK_EQUALS, GQL_TOK_AT, GQL_TOK_LBRACKET, GQL_TOK_RBRACKET,
    GQL_TOK_LBRACE, GQL_TOK_PIPE, GQL_TOK_RBRACE,
    GQL_TOK_NAME, GQL_TOK_INT, GQL_TOK_FLOAT, GQL_TOK_STRING, GQL_TOK_BLOCK_STRING,
    GQL_TOK_ERROR,
    GQL_OP_NONE, GQL_OP_QUERY, GQL_OP_MUTATION, GQL_OP_SUBSCRIPTION,
    GqlToken, GqlParseResult, GqlLexer,
    parseGraphql, minifyGraphql,
} from "graphql";
```

## Core API Signatures

```cp
// Constants
// Tokens (u8): GQL_TOK_EOF=0, BANG=1..RBRACE=14, NAME=15, INT=16, FLOAT=17, STRING=18, BLOCK_STRING=19, ERROR=255
// Ops (u8): GQL_OP_NONE=0, GQL_OP_QUERY=1, GQL_OP_MUTATION=2, GQL_OP_SUBSCRIPTION=3

// Types
struct GqlToken { kind: u8, start: usize, end: usize, line: usize, col: usize }
struct GqlParseResult {
    ok: bool, error_pos: usize, operation_count: usize, fragment_count: usize,
    field_count: usize, max_depth: usize, primary_op_type: u8, primary_op_name: string,
}

// Lexer
GqlLexer.new(src: string): GqlLexer;
lex.tokenText(tok: GqlToken): string;    // zero-copy slice into src
lex.peekToken(): GqlToken;               // read-only lookahead
lex.nextToken(*_): GqlToken;             // mutates cursor; requires `let lex`

// Parser & Minifier
fn parseGraphql(src: string): GqlParseResult;
fn minifyGraphql(src: string, out: []u8): usize;
```

## Minimal Example

```cp
import { parseGraphql, minifyGraphql, GQL_OP_QUERY } from "graphql";

const q = "query Hi { user { id name } }";
const res = parseGraphql(q);
if (res.ok && res.primary_op_type == GQL_OP_QUERY) {
    let buf: [128]u8 = undefined;
    const n = minifyGraphql(q, buf[0..128]); // buf[0..n] == "query Hi{user{id name}}"
}
```

## Gotchas & Borrow Rules

- `GqlLexer.nextToken(*_)` mutates the lexer; declare `let lex = GqlLexer.new(src)`.
- `tokenText(tok)` and `res.primary_op_name` borrow slices from `src`; keep `src` alive while using them.
- `minifyGraphql` truncates cleanly if `out` is smaller than minified output; size `out` >= `src.len`.
- Always check `tok.kind == GQL_TOK_EOF || tok.kind == GQL_TOK_ERROR` when looping over `nextToken()`.
