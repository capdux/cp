# `@cap/nom-parse`

Zero-allocation streaming parser combinator primitives for CP (clean-room MIT implementation inspired by Rust's `nom` v7).

## Features

- **Zero Heap Allocation**: All combinators return lightweight slice views (`ParseRes { ok, rest, matched }`, `IntRes`, `UintRes`).
- **Cross-Package Reuse**: `tagNoCase` delegates directly to `@cap/unicase` (`eqAscii`).
- **Byte & Tag Combinators**: `tag`, `tagNoCase`, `take`, `takeUntil`, `isNot`, `isA`.
- **Character Class Combinators**: `digit0`/`digit1`, `hexDigit1`, `alpha0`/`alpha1`, `alphanumeric0`/`alphanumeric1`, `space0`/`space1`, `multispace0`/`multispace1`, `lineEnding`, `notLineEnding`.
- **Sequence Combinators**: `delimitedTag`, `precededTag`, `terminatedTag`, `separatedPairTag`.
- **Overflow-Checked Numeric Parsers**: `parseU64`, `parseI64` (handles `i64::MIN`), `parseHexU64` (with optional `0x`/`0X` prefix).

## Usage

```typescript
import {
    tagNoCase,
    space1,
    delimitedTag,
    separatedPairTag,
    parseU64,
    parseI64,
    parseHexU64,
} from "@cap/nom-parse";

const r1 = tagNoCase("GET /index.html HTTP/1.1", "get");
const r2 = delimitedTag("(hello world) rest", "(", ")");
const kv = separatedPairTag("Content-Type: application/json", ": ");
const n = parseHexU64("0xCAFE_rest");
```

## License

MIT
