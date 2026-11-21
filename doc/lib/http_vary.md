# `cap:http_vary` — RFC 9110 `Vary` Header Parser & Normalizer

Zero-allocation RFC 9110 §12.5.5 `Vary` header iterator, wildcard `*` detector, case-insensitive membership check, and deduplicated canonical merger.

## Exports

- `VaryIter` — Zero-copy streaming iterator over valid HTTP field-name tokens in `Vary`.
- `isVaryWildcard(header): bool` — Detects `*` (uncacheable response).
- `varyContains(header, field_name): bool` — Case-insensitive header membership check.
- `appendVary(existing, field_name, out): ([]const u8)?` — Case-insensitive deduplicated append (collapses to `"*"` if either side is `*`).
- `mergeVary(a, b, out): ([]const u8)?` — Merges two `Vary` headers into a deduplicated lowercase canonical header value.
