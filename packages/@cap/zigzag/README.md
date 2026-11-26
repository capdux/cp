# `@cap/zigzag`

Zero-heap-allocation 32-bit & 64-bit ZigZag bijection and ZigZag-Varint streaming codec (Protobuf / Apache Avro / Parquet / CBOR) in pure CP.

## Features

- **Overflow-Free Bijection**: `encodeI32`/`decodeI32` and `encodeI64`/`decodeI64` covering full `MIN..MAX` ranges via branchless `@bitCast` arithmetic.
- **ZigZag-Varint Streaming**: `encodeZigzagVarint` (1..10 bytes) and `decodeZigzagVarint` with strict truncation & 10-byte overflow guards.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    VarintResult,
    encodeI32,
    decodeI32,
    encodeI64,
    decodeI64,
    encodeZigzagVarint,
    decodeZigzagVarint,
} from "@cap/zigzag";
```

## License

MIT
