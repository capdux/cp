# `@cap/uuid-v7`

Zero-heap-allocation RFC 9562 UUIDv7 constructor, 36-byte formatter, parser, timestamp extractor, and comparator in pure CP.

## Features

- **`fromParts`**: Encodes 48-bit big-endian Unix ms timestamp + 10 random bytes, enforcing version `7` and RFC variant `10`.
- **`timestampMs` / `version` / `isRfcVariant`**: Zero-copy bitfield inspection.
- **`formatHyphenated` / `parse` / `isValidUuidV7`**: Supports 36-byte canonical hyphenated and 32-byte compact hex forms.
- **`compare`**: Chronological byte-order comparator.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    UuidV7,
    fromParts,
    timestampMs,
    version,
    isRfcVariant,
    formatHyphenated,
    parse,
    isValidUuidV7,
    compare,
} from "@cap/uuid-v7";
```

## License

MIT
