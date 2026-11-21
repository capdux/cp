# `cap:json_minify` — Zero-Allocation JSON & JSONC Minifier

Zero-allocation JSON & JSONC (`tsconfig.json`, `cap.json`) minifier that strips `//` & `/* ... */` comments, trailing commas, UTF-8 BOM, and non-string whitespace while preserving string literals verbatim.

## Exports

- `minifyJson(src, out): ([]const u8)?` — Strict JSON whitespace minifier.
- `minifyJsonc(src, out): ([]const u8)?` — JSONC comment + trailing-comma + whitespace minifier.
- `minifyJsoncWithOptions(src, strip_comments, strip_trailing_commas, out): ([]const u8)?`
