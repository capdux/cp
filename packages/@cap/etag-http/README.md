# @cap/etag-http

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:etag` ([https://github.com/jshttp/etag](https://github.com/jshttp/etag))（原库名 `etag` → CP 包名 `@cap/etag-http`）

Clean-room MIT, zero-heap RFC 7232 HTTP strong/weak ETag generator, parser, and comparator for CP — modeled on npm's `etag` v1.8.

## Dependencies
- `@cap/fnv-hash` (`1.0.0`) — reused for 64-bit entity hashing and hex formatting.

## API
- `generateEtag(entity, weak, out): usize`
- `generateStatEtag(size_bytes, mtime_ms, out): usize`
- `parseEtag(raw): ParsedEtag`
- `compareStrong(a, b): bool` & `compareWeak(a, b): bool`
