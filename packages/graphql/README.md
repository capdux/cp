[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `graphql`

Zero-heap GraphQL Spec lexer, recursive-descent document parser/validator, and query minifier in pure CP — a clean-room MIT counterpart to npm [`graphql`](https://github.com/graphql/graphql-js) (v16 parser parity).

## Installation & Import

```bash
cap add graphql
```

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

## Core API

### Constants

| Group | Constants | Values |
| :--- | :--- | :--- |
| Token Kinds (`u8`) | `GQL_TOK_EOF` (0), `GQL_TOK_BANG`..`GQL_TOK_RBRACE` (1..14), `GQL_TOK_NAME` (15), `GQL_TOK_INT` (16), `GQL_TOK_FLOAT` (17), `GQL_TOK_STRING` (18), `GQL_TOK_BLOCK_STRING` (19), `GQL_TOK_ERROR` (255) | Lexer token tags |
| Operation Types (`u8`) | `GQL_OP_NONE` (0), `GQL_OP_QUERY` (1), `GQL_OP_MUTATION` (2), `GQL_OP_SUBSCRIPTION` (3) | Primary operation kind |

### Types & Functions

| Symbol | Signature / Fields | Description |
| :--- | :--- | :--- |
| `GqlToken` | `kind: u8`, `start: usize`, `end: usize`, `line: usize`, `col: usize` | Zero-copy token span referencing the input `string` |
| `GqlParseResult` | `ok: bool`, `error_pos: usize`, `operation_count: usize`, `fragment_count: usize`, `field_count: usize`, `max_depth: usize`, `primary_op_type: u8`, `primary_op_name: string` | Document validation & complexity analysis summary |
| `GqlLexer.new` | `(src: string): GqlLexer` | Create a zero-copy lexer over `src` |
| `GqlLexer.tokenText` | `(tok: GqlToken): string` | Slice original source text for `tok` |
| `GqlLexer.peekToken` | `(): GqlToken` | Look ahead at the next token without advancing |
| `GqlLexer.nextToken` | `(*_): GqlToken` | Consume and return the next token (mutates lexer cursor) |
| `parseGraphql` | `(src: string): GqlParseResult` | Validate full GraphQL executable document and collect operation/depth metrics |
| `minifyGraphql` | `(src: string, out: []u8): usize` | Strip `#` comments, insignificant commas, and redundant whitespace into `out` |

## Examples

### 1. Validating a Query & Enforcing Depth Limits

```cp
import { print } from "cap:io";
import { parseGraphql, GQL_OP_QUERY } from "graphql";

fn main() {
    const query = "query GetUser($id: ID!) { user(id: $id) { id name posts { title } } }";
    const res = parseGraphql(query);
    if (res.ok) {
        print("op:", res.primary_op_name, "isQuery:", res.primary_op_type == GQL_OP_QUERY);
        print("fields:", res.field_count, "max_depth:", res.max_depth);
    } else {
        print("syntax error at byte:", res.error_pos);
    }
}
```

### 2. Zero-Heap Query Minification & Token Streaming

```cp
import { print } from "cap:io";
import { minifyGraphql, GqlLexer, GQL_TOK_EOF, GQL_TOK_ERROR } from "graphql";

fn main() {
    const doc = "# fetch profile\nquery Profile {\n  me { id, name }\n}";
    let out: [256]u8 = undefined;
    const n = minifyGraphql(doc, out[0..256]);
    print("minified:", out[0..n]); // "query Profile{me{id name}}"

    // Stream tokens with mutable lexer (`let`)
    let lex = GqlLexer.new(doc);
    while (true) {
        const tok = lex.nextToken();
        if (tok.kind == GQL_TOK_EOF || tok.kind == GQL_TOK_ERROR) { break; }
        print(tok.kind, lex.tokenText(tok));
    }
}
```

## Differences from Upstream & Performance Highlights

- **Zero-Heap AST-Free Validation**: `parseGraphql` validates syntax, variables, directives, fragments, and selection-set nesting depth in a single recursive-descent pass without allocating AST nodes — ideal for API gateway query guards and complexity/depth limiters.
- **Zero-Copy Token Slices**: `GqlToken` stores byte offsets (`start..end`) rather than copying strings; `GqlLexer.tokenText(tok)` returns a direct slice into `src`.
- **Caller-Owned Minifier Output**: `minifyGraphql` writes directly into a stack or pooled `[]u8` buffer for cache-key normalization.
