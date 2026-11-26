# @cap/spdx-expression-parse

Zero-allocation, Clean-room MIT implementation of `spdx-expression-parse` v4 for CP, reusing `@cap/spdx` for SPDX license ID validation.

## Features

- **Node constants**: `SPDX_NODE_LICENSE = 1`, `SPDX_NODE_AND = 2`, `SPDX_NODE_OR = 3`.
- **Fixed 32-node AST pool**: `SpdxAstNode` and `SpdxAst` with zero heap allocations.
- **`parseSpdxExpression(expr: string): SpdxAst`**: Strict recursive-descent parser (`OR` < `AND` < `WITH` < `(...)`), supporting `+`, `WITH` exceptions, and `LicenseRef-*`.
- **`isValidSpdxExpression(expr: string): bool`**: Fast validity check.
- **`satisfiesSpdx(ast: SpdxAst, allowed_license: string): bool`**: Evaluates boolean `AND`/`OR` satisfaction against a single allowed license.

## License

MIT
