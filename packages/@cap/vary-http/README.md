# @cap/vary-http

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:vary` ([https://github.com/jshttp/vary](https://github.com/jshttp/vary))（原库名 `vary` → CP 包名 `@cap/vary-http`）

Clean-room MIT, zero-heap RFC 7231 HTTP `Vary` header manipulation library for CP — modeled on npm's `vary` v1.1.

## Dependencies
- `@cap/unicase` (`1.0.0`) — case-insensitive HTTP header field-name deduplication.

## API
- `appendVary(header, fields, out): usize` — merges & deduplicates `Vary` tokens, collapsing `*`
- `hasVaryField(header, field): bool`
- `countVaryFields(header): usize`
- `isValidFieldName(name): bool`
