# `@cap/urn-parse`

Zero-heap-allocation RFC 8141 Uniform Resource Name (URN) parser, equivalence comparator, and canonical formatter in pure CP.

## Features

- **`parseUrn`**: Zero-copy parsing of `urn:<nid>:<nss>[?+r][?=q][#f]` with strict RFC 8141 NID validation.
- **`urnEquivalent`**: RFC 8141 §3.1 equivalence comparison (case-insensitive `nid`, exact `nss`, ignoring r/q/f components).
- **`formatUrn`**: Canonical lowercase-scheme & lowercase-NID serialization into caller buffers.
- **`isValidNid`**: Validates 2..32 character NID grammar.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { Urn, isValidNid, parseUrn, urnEquivalent, formatUrn } from "@cap/urn-parse";
```

## License

MIT
