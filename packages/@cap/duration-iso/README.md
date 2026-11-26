# `@cap/duration-iso`

Zero-heap-allocation ISO 8601 duration parser, canonical formatter, and millisecond converter in pure CP.

## Features

- **Strict ISO 8601 Parsing (`parseIsoDuration`)**: Supports `P1Y2M3DT4H5M6S`, `P2W`, fractional seconds (`PT1.25S`), and signed durations (`-PT30M`); rejects empty `"P"`, `"PT"`, `"P1DT"`, and duplicate/out-of-order components.
- **Canonical Formatting (`formatIsoDuration`)**: Emits shortest valid ISO 8601 string, trimming trailing fractional zeros and emitting `"PT0S"` for zero duration.
- **Millisecond Conversion (`toApproxMillis`)**: Converts using `1Y = 365d`, `1M = 30d`, `1W = 7d`.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { IsoDuration, parseIsoDuration, formatIsoDuration, toApproxMillis } from "@cap/duration-iso";
```

## License

MIT
