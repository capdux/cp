# `@cap/purl`

Zero-heap-allocation Package URL (PURL) parser, validator, qualifier lookup, and canonical formatter in pure CP.

## Features

- **`parsePurl`**: Parses `pkg:type/namespace/name@version?qualifiers#subpath` with zero-copy slicing.
- **`isValidType`**: Enforces PURL `type` grammar (ASCII letter start, `[A-Za-z0-9.+-]`).
- **`getQualifier`**: Case-insensitive zero-copy lookup over `?k1=v1&k2=v2`.
- **`formatPurl`**: Serializes canonical lowercase-type PURL strings into caller buffers.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { Purl, parsePurl, isValidType, getQualifier, formatPurl } from "@cap/purl";
```

## License

MIT
