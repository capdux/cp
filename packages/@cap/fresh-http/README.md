# `@cap/fresh-http`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fresh` ([https://github.com/jshttp/fresh](https://github.com/jshttp/fresh))（原库名 `fresh` → CP 包名 `@cap/fresh-http`）

Zero-heap RFC 7232/9110 HTTP conditional request freshness evaluator in pure CP (clean-room MIT counterpart to npm `fresh` v0.5).

## Features

- **100% Zero Heap Allocation**: Evaluates `If-None-Match`, `If-Modified-Since`, `Cache-Control`, `ETag`, and `Last-Modified` purely on string slices.
- **Cross-Package Reuse**: Reuses `containsIgnoreCase` from `@cap/unicase` for case-insensitive `Cache-Control: no-cache` detection.
- **RFC 7232 ETag Matching**: Both `etagWeakMatch` (ignoring `W/`) and `etagStrongMatch` (rejecting weak validators), plus quoted-comma-safe multi-ETag list parsing and `*` wildcard support.
- **IMF-fixdate / RFC 1123 Parser**: `parseHttpDate` converts standard HTTP dates (e.g. `"Wed, 21 Oct 2015 07:28:00 GMT"`) to Unix timestamps with full calendar & leap-year validation.

## Dependencies

- `@cap/unicase` (`1.0.0`)

## Quick Start

```cp
import { isFresh, parseHttpDate, etagWeakMatch } from "@cap/fresh-http";

const fresh = isFresh(
    "W/\"v1\", \"v2\"",
    "Wed, 21 Oct 2015 07:28:00 GMT",
    "max-age=60",
    "\"v1\"",
    "Wed, 21 Oct 2015 07:00:00 GMT",
); // true -> respond 304 Not Modified
```

## API

- `isFresh(if_none_match, if_modified_since, cache_control, etag, last_modified): bool`
- `etagWeakMatch(a: string, b: string): bool`
- `etagStrongMatch(a: string, b: string): bool`
- `parseHttpDate(date_str: string): i64?`
- `hasNoCacheDirective(cache_control: string): bool`

## License

MIT
