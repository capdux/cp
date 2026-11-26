# @cap/etag-http

Clean-room MIT, zero-heap RFC 7232 HTTP strong/weak ETag generator, parser, and comparator for CP — modeled on npm's `etag` v1.8.

## Dependencies
- `@cap/fnv-hash` (`1.0.0`) — reused for 64-bit entity hashing and hex formatting.

## API
- `generateEtag(entity, weak, out): usize`
- `generateStatEtag(size_bytes, mtime_ms, out): usize`
- `parseEtag(raw): ParsedEtag`
- `compareStrong(a, b): bool` & `compareWeak(a, b): bool`
