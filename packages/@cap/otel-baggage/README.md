# `@cap/otel-baggage`

Zero-heap-allocation W3C Baggage (OpenTelemetry) HTTP header parser, percent-codec, and serializer in pure CP.

## Features

- **`parseBaggage` & `countMembers`**: Zero-copy parsing of comma-separated `key=value;properties` members.
- **`getDecodedValue`**: Last-write-wins key lookup with RFC 3986 percent-decoding.
- **`formatBaggage`**: Serializes members with automatic percent-encoding for `,`, `;`, `=`, `%`, whitespace, and non-ASCII bytes while preserving `;properties`.
- **`isValidKey`**: Strict RFC 7230 `token` validation.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    BaggageMember,
    parseBaggage,
    getDecodedValue,
    countMembers,
    formatBaggage,
    isValidKey,
} from "@cap/otel-baggage";
```

## License

MIT
