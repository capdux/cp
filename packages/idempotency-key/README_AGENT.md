# idempotency-key@1.0.0 (Agent Quickref)

> Zero-allocation IETF Idempotency-Key HTTP header parser, validator, RFC 8941 formatter, and request fingerprint state evaluator for CP. | License: MIT
> Import: `import { ... } from "idempotency-key";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidKey(key: string): bool;`
- `export fn parseKeyHeader(header: string, out: []u8): usize;`
- `export fn formatKeyHeader(key: string, out: []u8): usize;`
- `export fn isUuidV4Like(key: string): bool;`
- `export fn fingerprintRequest(method: string, path: string, body: &[]u8): u64;`
- `export fn evaluateState(exists: bool, in_flight: bool, stored_fp: u64, req_fp: u64): i32;`
