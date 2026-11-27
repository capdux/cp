[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `camelcase-keys`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:camelcase-keys` ([https://github.com/sindresorhus/camelcase-keys](https://github.com/sindresorhus/camelcase-keys)) / `npm:decamelize-keys` ([https://github.com/sindresorhus/decamelize-keys](https://github.com/sindresorhus/decamelize-keys))

Zero-heap streaming JSON object key case converter (`camelCase`, `snake_case`, `PascalCase`) in pure CP (clean-room counterpart to npm `camelcase-keys` & `decamelize-keys`). Directly composes `ident_case`.

## Features

- **Real Cross-Package Composition**: Delegates identifier conversion directly to `ident_case` (`toCamelCase`, `toSnakeCase`, `toPascalCase`).
- **100% Zero Heap Allocation**: Single-pass RFC 8259 JSON key scanner writing directly into caller-provided `out: []u8`.
- **Values Strictly Preserved**: Converts only object keys (including inside deeply nested objects and arrays); leaves string values, escaped quotes, numbers, and booleans untouched.

## Quick Start

```cp
import { convertJsonKeysToCamel, convertJsonKeysToSnake, convertJsonKeysToPascal } from "camelcase-keys";

let buf: [256]u8 = undefined;
const n = convertJsonKeysToCamel("{\"user_id\":1,\"first_name\":\"foo_bar\"}", &buf);
// "{\"userId\":1,\"firstName\":\"foo_bar\"}"
```

## License

MIT
