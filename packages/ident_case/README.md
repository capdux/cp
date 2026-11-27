# @cap/camelcase

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ident_case` ([https://github.com/TedDriggs/ident_case](https://github.com/TedDriggs/ident_case))（原库名 `ident_case` → CP 包名 `@cap/camelcase`）

Zero-heap programming case conversion engine in pure CP (npm `camelcase` / `change-case` & Rust `heck` parity).

## Features

- `toCamelCase(input, out)`: `foo-bar_baz` -> `fooBarBaz`, `XMLHttpRequest` -> `xmlHttpRequest`
- `toPascalCase(input, out)`: `foo-bar` -> `FooBar`
- `toSnakeCase(input, out)`: `XMLHttpRequest` -> `xml_http_request`, `getHTTPResponseCode` -> `get_http_response_code`
- `toKebabCase(input, out)`: `fooBarBaz` -> `foo-bar-baz`
- `toScreamingSnakeCase(input, out)`: `httpServerPort` -> `HTTP_SERVER_PORT`

## Usage

```cp
import { toCamelCase, toSnakeCase } from "@cap/camelcase";

let buf: [64]u8 = undefined;
const n = toSnakeCase("XMLHttpRequest", buf[0..64]);
// buf[0..n] => "xml_http_request"
```
