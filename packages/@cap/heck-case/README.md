# `@cap/heck-case`

Zero-allocation Rust `heck` v0.5 compatible case conversion engine for CP (clean-room MIT).

## Features

- **Zero Heap Allocation**: Writes converted UTF-8 bytes directly into caller-provided `out: []u8` buffers and returns the written byte count.
- **Exact `heck` Word Boundary Rules**:
  - Acronym splitting: `XMLHttpRequest` -> `xml_http_request`, `HTTPSConnection` -> `https_connection`
  - Alphanumeric transitions: `foo2Bar` -> `foo2_bar`
  - Multi-separator & leading/trailing delimiter collapsing (`snake_case`, `kebab-case`, `SHOUTY_CASE`, spaces, dots, slashes)
  - UTF-8 safe truncation when output buffer is small
- **All 8 Standard Case Styles**:
  - `toUpperCamelCase` (`PascalCase`)
  - `toLowerCamelCase` (`camelCase`)
  - `toSnakeCase` (`snake_case`)
  - `toShoutySnakeCase` (`SHOUTY_SNAKE_CASE`)
  - `toKebabCase` (`kebab-case`)
  - `toShoutyKebabCase` (`SHOUTY-KEBAB-CASE`)
  - `toTitleCase` (`Title Case`)
  - `toTrainCase` (`Train-Case`)

## Usage

```typescript
import {
    toUpperCamelCase,
    toLowerCamelCase,
    toSnakeCase,
    toShoutySnakeCase,
    toKebabCase,
    toShoutyKebabCase,
    toTitleCase,
    toTrainCase,
} from "@cap/heck-case";

let buf: [64]u8 = undefined;
const n: usize = toSnakeCase("XMLHttpRequest", buf[0..64]);
// buf[0..n] == "xml_http_request"
```

## License

MIT
