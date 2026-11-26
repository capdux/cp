# @cap/accept-header

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:accept-header` ([https://github.com/djc/accept-header](https://github.com/djc/accept-header))

Clean-room MIT, zero-heap RFC 7231 HTTP `Accept` header parser and media-type negotiator for CP — modeled on crates.io `accept-header`, reusing `@cap/negotiator` and `@cap/unicase`.

## Features

- **Zero heap allocation**: fixed 16-slot `AcceptHeader` value type with zero dynamic allocations.
- **RFC 7231 quality & specificity ranking**: parses `;q=0.x` weights via `@cap/negotiator`, filters `q=0`, and stably sorts by descending `quality_milli` then descending `specificity` (`3` = `type/sub`, `2` = `type/*`, `1` = `*/*`).
- **Case-insensitive MIME matching**: uses `@cap/unicase` `eqAscii` for RFC-compliant case-insensitive type/subtype matching.

## API

### Types
- `AcceptEntry { media_range: string, quality_milli: u16, specificity: u8 }`
- `AcceptHeader`:
  - `AcceptHeader.parse(header: string): AcceptHeader`
  - `len(): usize`
  - `accepts(mime: string): bool`
  - `bestMatch(available: []string): string?`

### Functions
- `negotiateMediaType(header: string, available: []string): string?`

## Usage

```typescript
import { AcceptHeader, negotiateMediaType } from "@cap/accept-header";

let avail: [2]string = ["text/html", "application/json"];
let best = negotiateMediaType("text/html;q=0.8, application/json;q=0.95", avail[0..2]);
// -> "application/json"
```

## License

MIT
