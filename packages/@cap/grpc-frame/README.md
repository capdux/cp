# `@cap/grpc-frame`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tonic` ([https://github.com/hyperium/tonic](https://github.com/hyperium/tonic)) / `npm:@grpc/grpc-js` ([https://github.com/grpc/grpc-node](https://github.com/grpc/grpc-node))（原库名 `tonic` → CP 包名 `@cap/grpc-frame`）

Zero-heap-allocation gRPC over HTTP/2 Length-Prefixed Message frame codec, 17 canonical `grpc-status` codes, and `grpc-timeout` header parser/formatter in pure CP.

## Features

- **5-Byte Framing**: `encodeFrame` and `decodeHeader` (`[compressed_flag, len_be32, payload]`) with streaming `GRPC_INCOMPLETE` and `GRPC_INVALID` detection.
- **17 Canonical Status Codes**: `STATUS_OK (0)` .. `STATUS_UNAUTHENTICATED (16)`, `statusName`, and `isRetryableStatus`.
- **gRPC Timeout**: `parseTimeoutMicros` supporting all 6 units (`H`, `M`, `S`, `m`, `u`, `n`) and `formatTimeoutMillis` respecting the 8-digit limit.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    STATUS_OK,
    STATUS_UNAVAILABLE,
    GRPC_OK,
    GRPC_INCOMPLETE,
    GRPC_INVALID,
    GrpcFrameHeader,
    statusName,
    isRetryableStatus,
    encodeFrame,
    decodeHeader,
    parseTimeoutMicros,
    formatTimeoutMillis,
} from "@cap/grpc-frame";
```

## License

MIT
