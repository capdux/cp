# `cap:http_keep_alive` — HTTP `Keep-Alive` Header & Connection Persistence

Zero-allocation `Keep-Alive: timeout=5, max=100` parameter parser/formatter and RFC 9112 §9.3 HTTP/1.0 & HTTP/1.1 connection persistence evaluator.

## Exports

- `KeepAliveParams` — `parse(header)`, `format(out)` (`timeout_sec: u64?`, `max_requests: u64?`).
- `shouldKeepAlive(http_minor_version: u8, connection_header: ([]const u8)?): bool`.
