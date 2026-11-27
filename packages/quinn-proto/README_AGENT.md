# quinn-proto — AI Agent Quick Ref

Pure CP zero-allocation RFC 9000 QUIC variable-length integer codec and Long Header parser (quinn-proto parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    QuicVarint,
    QuicLongHeader,
    encodedLen,
    encodeVarint,
    decodeVarint,
    qvInvalidHeader,
    parseLongHeader,
} from "quinn-proto";
```

## Core Types & Constants

```cp
class QuicVarint { val: u64, bytes_read: usize, ok: bool }
class QuicLongHeader { is_long: bool, packet_type: u8, version: u32, dcid_len: u8, /* +3 fields */ }
```

## API Signatures

```cp
fn encodedLen(val: u64): usize;
fn encodeVarint(val: u64, out: []u8): usize;
fn decodeVarint(buf: []u8): QuicVarint;
fn qvInvalidHeader(): QuicLongHeader;
fn parseLongHeader(pkt: []u8): QuicLongHeader;
```

## Minimal Usage

```cp
import { encodedLen } from "quinn-proto";

pub fn main() {
    let result = encodedLen(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"quinn-proto"`; install via `cap add quinn-proto`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
