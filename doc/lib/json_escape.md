# `cap:json_escape` — RFC 8259 JSON String Escaper & Unescaper

Zero-allocation RFC 8259 JSON string escaper, literal quoter, and strict unescaper with full `\uXXXX` and UTF-16 surrogate-pair (`\uD83D\uDE00`) UTF-8 decoding.

## Exports

- `needsJsonEscape(src): bool`
- `escapeJsonContent(src, out): ([]const u8)?`
- `quoteJsonString(src, out): ([]const u8)?`
- `unescapeJsonContent(escaped, out): ([]const u8)?`
