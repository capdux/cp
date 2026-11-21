# `cap:case_convert` — Acronym-Aware Identifier Case Converter

Zero-allocation identifier case converter supporting `snake_case`, `SCREAMING_SNAKE_CASE`, `kebab-case`, `camelCase`, and `PascalCase` with acronym-aware word boundary splitting (`HTTPServer` -> `http_server`).

## Exports

- `WordIter` — Zero-copy identifier word splitter (`init(src)`, `next(*_): ([]const u8)?`).
- `toSnakeCase(src, out): ([]const u8)?`
- `toScreamingSnakeCase(src, out): ([]const u8)?`
- `toKebabCase(src, out): ([]const u8)?`
- `toCamelCase(src, out): ([]const u8)?`
- `toPascalCase(src, out): ([]const u8)?`
