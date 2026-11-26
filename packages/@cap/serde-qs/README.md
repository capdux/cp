# @cap/serde-qs

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:serde_qs` ([https://github.com/samscott89/serde_qs](https://github.com/samscott89/serde_qs))（原库名 `serde_qs` → CP 包名 `@cap/serde-qs`）

Zero-allocation nested bracket query string parser & serializer for CP — clean-room MIT parity with crates.io `serde_qs` v0.13.

Composes [`@cap/percent-encoding`](../percent-encoding) for WHATWG/RFC percent-encoding and form-style (`+` -> space) decoding with zero code duplication.

## Installation

```json
{
  "dependencies": {
    "@cap/serde-qs": "1.0.0"
  }
}
```

## Quick Start

```cp
import {
    QsPair,
    parseKeyPath,
    parseQs,
    getQsValueDecoded,
    serializeQs,
} from "@cap/serde-qs";

const path = parseKeyPath("user[address][city]");
// path.depth == 3, path.segments[0..3] == ["user", "address", "city"]

let pairs: [8]QsPair = undefined;
const count = parseQs("?user[name]=Alice+Smith&items[0]=rust&tags[]=cp", pairs[0..]);

let valBuf: [128]u8 = undefined;
const lenOpt = getQsValueDecoded("?user[name]=Alice+Smith", "user[name]", valBuf[0..]);
// valBuf[0..len] == "Alice Smith"
```

## API Reference

### Types

- `QsPair { key: string, value: string }` — Raw key-value pair sliced directly from the query string.
- `QsPath { segments: [8]string, depth: usize }` — Fixed-capacity parsed bracket path (up to 8 nesting levels).

### `parseKeyPath(key: string): QsPath`

Splits a nested bracket key into segments without heap allocation:
- `"user[address][city]"` -> `["user", "address", "city"]` (`depth = 3`)
- `"items[0]"` -> `["items", "0"]` (`depth = 2`)
- `"tags[]"` -> `["tags", ""]` (`depth = 2`)
- `"plain"` -> `["plain"]` (`depth = 1`)

### `parseQs(query: string, out_pairs: []QsPair): usize`

Automatically skips a leading `?`, splits on `&`, and writes raw `QsPair` entries into `out_pairs`. Empty segments are skipped; keys without `=` receive an empty value `""`.

### `getQsValueDecoded(query: string, target_key: string, out: []u8): usize?`

Finds the first entry matching `target_key` (matching both raw and percent-decoded keys) and writes the decoded value (with `+` converted to space and `%XX` decoded via `@cap/percent-encoding`) into `out`. Returns `null` if the key is not found.

### `serializeQs(pairs: []const QsPair, out: []u8): usize`

Percent-encodes keys and values via `@cap/percent-encoding` and joins entries with `&`. Preserves structural `[` and `]` brackets in keys for clean `serde_qs` round-tripping.

## License

MIT
