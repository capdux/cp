# tonic@1.0.0 (Agent Quickref)

> Zero-allocation gRPC over HTTP/2 length-prefixed frame codec, 17 status codes, and grpc-timeout parser/formatter in pure CP | License: MIT
> Import: `import { ... } from "tonic";` | Full types: `src/index.d.cp`

## Functions
- `export fn statusName(code: u8): string;`
- `export fn isRetryableStatus(code: u8): bool;`
- `export fn encodeFrame(compressed: bool, payload: &[]u8, out: []u8): usize;`
- `export fn decodeHeader(data: &[]u8, out: &mut GrpcFrameHeader): i32;`
- `export fn isSpace(b: u8): bool;`
- `export fn parseTimeoutMicros(header: string): i64;`
- `export fn formatTimeoutMillis(ms: u64, out: []u8): usize;`
