# `@cap/range-parser`

Zero-heap-allocation RFC 9110 HTTP `Range` header parser and `Content-Range` formatter in pure CP.

## Features

- **Full Byte Range Support**: Closed (`bytes=0-499`), open-ended (`bytes=500-`), suffix (`bytes=-200`), and multi-range (`bytes=0-99, 100-199`).
- **Automatic Clamping & Combining**: Clamps `end` to `size - 1`, and optionally sorts & merges overlapping/adjacent ranges (`combine: true`).
- **RFC 9110 Status Codes**: Distinguishes `RANGE_OK (0)`, `RANGE_UNSATISFIABLE (-1 / HTTP 416)`, and `RANGE_MALFORMED (-2)`.
- **Response Formatting**: `formatContentRange` (`bytes start-end/total`) and `formatUnsatisfiableRange` (`bytes */total`).
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI (`MAX_RANGES = 16`).

## API

```cp
import {
    MAX_RANGES,
    RANGE_OK,
    RANGE_UNSATISFIABLE,
    RANGE_MALFORMED,
    ByteRange,
    ParseRangeResult,
    parseRange,
    formatContentRange,
    formatUnsatisfiableRange,
} from "@cap/range-parser";
```

## License

MIT
