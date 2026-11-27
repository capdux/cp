# const-oid — AI Agent Quick Ref

Pure CP zero-allocation ITU-T X.690 ASN.1 Object Identifier (OID) parser, encoder, and comparator (parity with RustCrypto const-oid) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    OidParseResult,
    ObjectIdentifier,
    coidZeroArcs,
    coidZeroDer,
    coidEncodeBase128U64,
    coidEncodeArcsToDer,
    coidWriteU32Dec,
} from "const-oid";
```

## Core Types & Constants

```cp
class OidParseResult { ok: bool, oid: ObjectIdentifier }
class ObjectIdentifier { arcs: [16]u32, len: usize, der_buf: [80]u8, der_len: usize } // methods: empty(...) +11
```

## API Signatures

```cp
fn coidZeroArcs(): [16]u32;
fn coidZeroDer(): [80]u8;
fn coidEncodeBase128U64(val: u64, out: []u8, start_pos: usize): usize;
fn coidEncodeArcsToDer(arcs: [16]u32, arc_len: usize, der_out: []u8): usize;
fn coidWriteU32Dec(val: u32, out: []u8, start_pos: usize): usize;
```

## Minimal Usage

```cp
import { coidZeroArcs } from "const-oid";

pub fn main() {
    let result = coidZeroArcs();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"const-oid"`; install via `cap add const-oid`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
