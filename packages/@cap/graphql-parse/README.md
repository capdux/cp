# `@cap/graphql-parse`

Zero-heap GraphQL Spec lexer, recursive-descent document parser/validator, and query minifier in pure CP (clean-room MIT counterpart to npm `graphql` v16 parser).

## Features

- **100% Zero Heap Allocation**: Tokenizes, parses, validates, and minifies GraphQL documents directly on slices and caller-provided `out: []u8`.
- **Complete GraphQL Spec Lexer (`GqlLexer`)**:
  - Skips whitespace, insignificant commas, UTF-8 BOM, and `#` comments
  - All punctuators (`! $ & ( ) ... : = @ [ ] { | }`)
  - Names, Int/Float literals (including exponents), single-line escaped strings (`\uXXXX`), and triple-quoted `"""..."""` block strings
  - Accurate 1-based line & column tracking
- **Full Executable Document Parser (`parseGraphql`)**:
  - `query`, `mutation`, `subscription`, and shorthand `{ ... }` operations
  - Variable definitions (`($id: ID! = "1")`), directives (`@include(if: $flag)`), field aliases (`u: user(id: 1)`), nested list/object/enum/variable arguments
  - Named fragments (`fragment Fields on User { ... }`), fragment spreads (`...Fields`), and inline fragments (`... on User { ... }`)
  - Returns `GqlParseResult` with `ok`, `error_pos`, `operation_count`, `fragment_count`, `field_count`, `max_depth`, `primary_op_type`, and `primary_op_name`
- **Zero-Heap Minifier (`minifyGraphql`)**: Strips comments, commas, and redundant whitespace while preserving required separators and string literals.

## Quick Start

```cp
import { parseGraphql, minifyGraphql, GQL_OP_QUERY } from "@cap/graphql-parse";

const doc = "query GetUser($id: ID!) { user(id: $id) { id name } }";
const res = parseGraphql(doc);
// res.ok == true, res.primary_op_type == GQL_OP_QUERY, res.field_count == 3, res.max_depth == 2

let buf: [256]u8 = undefined;
const n = minifyGraphql(doc, &buf);
// "query GetUser($id:ID!){user(id:$id){id name}}"
```

## License

MIT
