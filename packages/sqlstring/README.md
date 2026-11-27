[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `sqlstring`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sqlstring` ([https://github.com/mysqljs/sqlstring](https://github.com/mysqljs/sqlstring)) / `npm:pg-format` ([https://github.com/datalanche/node-pg-format](https://github.com/datalanche/node-pg-format))（原库名 `sqlstring` → CP 包名 `sqlstring`）

Zero-heap-allocation ANSI SQL / PostgreSQL / MySQL literal, identifier, and `LIKE` pattern escaper with placeholder counter in pure CP.

## Features

- **`escapeStringAnsi`**: Wraps strings in `'...'` with `'` -> `''` escaping and NUL byte rejection.
- **`escapeIdentifierAnsi` & `escapeIdentifierMysql`**: Quotes identifiers as `"..."` (ANSI/PostgreSQL/SQLite) or `` `...` `` (MySQL).
- **`escapeLike`**: Escapes `%`, `_`, and the chosen escape character for `LIKE ... ESCAPE`.
- **`isSafeUnquotedIdent`**: Validates 1..63 character `[A-Za-z_][A-Za-z0-9_]*` identifiers.
- **`countPlaceholders`**: Counts `?` placeholders outside single/double quoted literals.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    escapeStringAnsi,
    escapeIdentifierAnsi,
    escapeIdentifierMysql,
    escapeLike,
    isSafeUnquotedIdent,
    countPlaceholders,
} from "sqlstring";
```

## License

MIT
