# `@cap/path-to-regexp`

Zero-heap-allocation URL route pattern compiler, parameter extractor, and reverse path builder in pure CP.

## Features

- **Route Syntax**: Static literals, named `:param`, optional `:param?`, and multi-segment `*wildcard`.
- **Zero-Copy Extraction**: `RoutePattern.matchPath` returns exact byte ranges (`ParamSlice`) into the original `pattern` and `path`.
- **Reverse Path Builder**: `buildPath` substitutes parameter slices into a route pattern into a caller buffer.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI (`MAX_SEGMENTS = 16`, `MAX_PARAMS = 8`).

## API

```cp
import {
    MAX_SEGMENTS,
    MAX_PARAMS,
    ParamSlice,
    MatchResult,
    RoutePattern,
    buildPath,
} from "@cap/path-to-regexp";
```

## License

MIT
