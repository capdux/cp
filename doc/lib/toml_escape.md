# `cap:toml_escape` — TOML 1.0 String & Key Escaper / Unescaper

Zero-allocation TOML 1.0 bare-key validator, basic string literal formatter, and strict unescaper supporting `\uXXXX` and `\UXXXXXXXX` Unicode scalar values.

## Exports

- `isBareTomlKey(key): bool`
- `formatTomlBasicString(src, out): ([]const u8)?`
- `formatTomlKey(key, out): ([]const u8)?`
- `unescapeTomlBasic(escaped, out): ([]const u8)?`
