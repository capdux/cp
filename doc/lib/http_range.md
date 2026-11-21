# `cap:http_range` — RFC 9110 §14.1.2 HTTP `Range` Request Header Parser

Zero-allocation RFC 9110 §14.1.2 `Range: bytes=...` request header parser, multi-range iterator, and byte-range resolver (bounded `start-end`, open-ended `start-`, and suffix `-N` ranges).

## Exports

- `RangeSpecKind` (`bounded` | `open_end` | `suffix`)
- `ByteRangeSpec` — `resolve(entity_len: u64): ResolvedByteRange?` (clamps end to `entity_len - 1`, rejects unsatisfiable HTTP 416 ranges).
- `ResolvedByteRange` — `start`, `end`, `total_len`, `length(): u64`.
- `ByteRangeIter` — `init(header): ByteRangeIter?`, `next(*_): ByteRangeSpec?`, `hasError(): bool`.
- `resolveSingleByteRange(header, entity_len): ResolvedByteRange?` — Fast path for single-range requests.
