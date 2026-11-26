# `@cap/parse-ms`

Zero-heap millisecond breakdown (`days`, `hours`, `minutes`, `seconds`, `milliseconds`, `microseconds`, `nanoseconds`) and reconstruction in pure CP (clean-room counterpart to npm `parse-ms` & `to-milliseconds`).

## Features

- **100% Zero Heap Allocation**: Pure value-type `ParsedMs` struct with unsigned division for Debug-safe arithmetic.
- **Integer & Sub-Millisecond Parsing**: `parseMs(ms: i64)` and `parseMsF64(ms: f64)` with truncation-toward-zero negative semantics.
- **Exact Reconstruction**: `toMilliseconds(p: ParsedMs): i64`.

## Quick Start

```cp
import { parseMs, parseMsF64, toMilliseconds } from "@cap/parse-ms";

const p = parseMs(1337000001);
// { days: 15, hours: 11, minutes: 23, seconds: 20, milliseconds: 1, microseconds: 0, nanoseconds: 0 }
const total = toMilliseconds(p); // 1337000001
```

## License

MIT
