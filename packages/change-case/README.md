# @cap/change-case

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:change-case` ([https://github.com/blakeembrey/change-case](https://github.com/blakeembrey/change-case))

零堆分配的 CP 原生全能命名风格转换库，Clean-Room MIT 实现完整对标 npm Top 110 顶流包 `change-case` v5（8000 万+/周下载）。

跨包零重复复用 `@cap/camelcase`（`toCamelCase` / `toPascalCase`）与 `@cap/decamelize`（`decamelize`），零堆分配、单遍/原地转换、自动处理缩写词（如 `parseHTTPResponse`、`XMLHttpRequest`）及小缓冲区安全截断。

## 支持的全部 12 种风格

| 函数 | 示例输入 | 输出 |
| :--- | :--- | :--- |
| `camelCase(input, out)` | `"foo_bar-baz"` | `"fooBarBaz"` |
| `pascalCase(input, out)` | `"foo_bar-baz"` | `"FooBarBaz"` |
| `snakeCase(input, out)` | `"fooBarBaz"` | `"foo_bar_baz"` |
| `kebabCase(input, out)` | `"fooBarBaz"` | `"foo-bar-baz"` |
| `constantCase(input, out)` | `"fooBarBaz"` | `"FOO_BAR_BAZ"` |
| `dotCase(input, out)` | `"fooBarBaz"` | `"foo.bar.baz"` |
| `pathCase(input, out)` | `"fooBarBaz"` | `"foo/bar/baz"` |
| `trainCase(input, out)` | `"content_type"` | `"Content-Type"` |
| `capitalCase(input, out)` | `"fooBarBaz"` | `"Foo Bar Baz"` |
| `sentenceCase(input, out)` | `"fooBarBaz"` | `"Foo bar baz"` |
| `noCase(input, out)` | `"fooBarBaz"` | `"foo bar baz"` |
| `swapCase(input, out)` | `"FooBar"` | `"fOObAR"` |

## 依赖复用

- `@cap/camelcase`: `1.0.0`
- `@cap/decamelize`: `1.0.0`

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`blakeembrey/change-case`](https://github.com/blakeembrey/change-case)。
